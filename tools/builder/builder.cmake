# builder.cmake — turn the user's own ROM into a native James Pond 3 build.
#
# This script is the shared brain of the Windows and Linux builders. The
# launchers only collect the ROM path and start this file:
#
#   cmake -DROM=<rom> -DOUT_DIR=<dir> -DPAYLOAD=<builder dir> -P builder.cmake
#
# PAYLOAD holds this script, source.tar.gz, toolchain/ and sdl2/. Everything is
# built in a scratch directory so the payload can be a read-only AppImage
# mount, and only the finished game is copied to OUT_DIR.

cmake_minimum_required(VERSION 3.21)

foreach(_var ROM OUT_DIR PAYLOAD)
    if(NOT DEFINED ${_var} OR "${${_var}}" STREQUAL "")
        message(FATAL_ERROR "builder: missing -D${_var}=...")
    endif()
endforeach()
get_filename_component(ROM "${ROM}" ABSOLUTE)
get_filename_component(OUT_DIR "${OUT_DIR}" ABSOLUTE)
get_filename_component(PAYLOAD "${PAYLOAD}" ABSOLUTE)

set(_cmake "${CMAKE_COMMAND}")
if(CMAKE_HOST_WIN32)
    set(_ninja "${PAYLOAD}/toolchain/ninja.exe")
    set(_exe_suffix ".exe")
    if(DEFINED ENV{TEMP})
        set(_work "$ENV{TEMP}/jp3-builder")
    else()
        set(_work "$ENV{LOCALAPPDATA}/jp3-builder")
    endif()
else()
    set(_ninja "${PAYLOAD}/toolchain/ninja")
    set(_exe_suffix "")
    if(DEFINED ENV{XDG_CACHE_HOME} AND NOT "$ENV{XDG_CACHE_HOME}" STREQUAL "")
        set(_work "$ENV{XDG_CACHE_HOME}/jp3-builder")
    else()
        set(_work "$ENV{HOME}/.cache/jp3-builder")
    endif()
endif()

# ---------------------------------------------------------------------------
# The ROM is the only user input. Check it before spending minutes compiling.
# ---------------------------------------------------------------------------
set(_rom_sha "8f52e27945b96d6c37fda28c44d39d65d805cc0b58332f9b4a2eb26dff540d29")
if(NOT EXISTS "${ROM}" OR IS_DIRECTORY "${ROM}")
    message(FATAL_ERROR "ROM not found: ${ROM}")
endif()
file(SIZE "${ROM}" _rom_size)
file(SHA256 "${ROM}" _rom_hash)
if(NOT _rom_size EQUAL 2097152 OR NOT _rom_hash STREQUAL _rom_sha)
    message(FATAL_ERROR
        "This is not the James Pond 3 (USA, Europe) ROM.\n"
        "  got:      ${_rom_size} bytes, sha256 ${_rom_hash}\n"
        "  expected: 2097152 bytes, sha256 ${_rom_sha}")
endif()
message(STATUS "ROM verified: James Pond 3 (USA, Europe)")

# ---------------------------------------------------------------------------
# Fresh scratch tree: toolchain and SDL2 paths differ on every AppImage run,
# so stale CMake caches are worse than a few minutes of recompilation.
# ---------------------------------------------------------------------------
message(STATUS "Working directory: ${_work}")
file(REMOVE_RECURSE "${_work}")
file(MAKE_DIRECTORY "${_work}/src")
file(ARCHIVE_EXTRACT INPUT "${PAYLOAD}/source.tar.gz" DESTINATION "${_work}/src")
file(COPY_FILE "${ROM}" "${_work}/src/game/jp3.md")

include(ProcessorCount)
ProcessorCount(_jobs)
if(_jobs EQUAL 0)
    set(_jobs 4)
endif()

# ---------------------------------------------------------------------------
# Configure and build the game. The bundled toolchain file points CMake and
# the archiver at the bundled Zig, so no compiler needs to be installed.
# ---------------------------------------------------------------------------
set(_configure_args
    -S "${_work}/src" -B "${_work}/build" -G Ninja
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_MAKE_PROGRAM=${_ninja}
    -DCMAKE_TOOLCHAIN_FILE=${PAYLOAD}/toolchain/zig-toolchain.cmake
    -DSDL2_DIR=${PAYLOAD}/sdl2/lib/cmake/SDL2
)
message(STATUS "Configuring...")
execute_process(COMMAND "${_cmake}" ${_configure_args}
                RESULT_VARIABLE _result)
if(NOT _result EQUAL 0)
    message(FATAL_ERROR "Configure failed (${_result}).")
endif()

message(STATUS "Compiling (${_jobs} jobs). This takes a few minutes...")
execute_process(COMMAND "${_cmake}" --build "${_work}/build"
                        --parallel ${_jobs}
                RESULT_VARIABLE _result)
if(NOT _result EQUAL 0)
    message(FATAL_ERROR "Build failed (${_result}).")
endif()

set(_built "${_work}/build/JamesPond3Recomp${_exe_suffix}")
if(NOT EXISTS "${_built}")
    message(FATAL_ERROR "Build finished but ${_built} is missing.")
endif()

# ---------------------------------------------------------------------------
# Deliver only what the game needs at run time.
# ---------------------------------------------------------------------------
file(MAKE_DIRECTORY "${OUT_DIR}")
file(COPY_FILE "${_built}" "${OUT_DIR}/JamesPond3Recomp${_exe_suffix}")
file(COPY_FILE "${ROM}" "${OUT_DIR}/jp3.md")
if(CMAKE_HOST_WIN32)
    file(COPY_FILE "${PAYLOAD}/sdl2/bin/SDL2.dll" "${OUT_DIR}/SDL2.dll")
endif()
file(WRITE "${OUT_DIR}/README.txt"
"James Pond 3: Operation Starfish — native build\n\
\n\
Run JamesPond3Recomp${_exe_suffix}. The game reads jp3.md from this folder.\n\
\n\
Built from your own ROM by the James Pond 3 recompilation builder.\n\
Source and license: https://github.com/salmon85/James-Pond-3-Recomp\n")

message(STATUS "Done: ${OUT_DIR}/JamesPond3Recomp${_exe_suffix}")
