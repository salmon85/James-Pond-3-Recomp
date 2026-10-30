cmake_minimum_required(VERSION 3.16)
project(JamesPond3Recomp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Shared static-recompilation framework. The engine is vendored under engine/
# so this repository is self-contained; set GENESIS_RECOMP_ROOT to use another
# segagenesisrecomp checkout.
set(GENESIS_RECOMP_ROOT "" CACHE PATH "Optional segagenesisrecomp checkout")
if(GENESIS_RECOMP_ROOT)
    set(RECOMP_ROOT "${GENESIS_RECOMP_ROOT}")
else()
    set(RECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/engine")
endif()
set(RUNNER_ROOT "${RECOMP_ROOT}/runner")

if(NOT EXISTS "${RECOMP_ROOT}/runner/glue.c")
    message(FATAL_ERROR "segagenesisrecomp checkout not found at ${RECOMP_ROOT}")
endif()

# SDL2 is the only host dependency besides a C/C++ toolchain and CMake.
if(WIN32)
    list(APPEND CMAKE_PREFIX_PATH
        "${RUNNER_ROOT}/external/SDL2/SDL2-2.28.5/cmake")
endif()
find_package(SDL2 REQUIRED)

# SDL2 headers are included as <SDL2/SDL.h>; Homebrew points the include dir
# at .../include/SDL2, so add its parent as well.
set(SDL2_PARENT_INCLUDE_DIRS "")
foreach(_inc IN LISTS SDL2_INCLUDE_DIRS)
    get_filename_component(_inc_name "${_inc}" NAME)
    if(_inc_name STREQUAL "SDL2")
        get_filename_component(_inc_parent "${_inc}" DIRECTORY)
        list(APPEND SDL2_PARENT_INCLUDE_DIRS "${_inc_parent}")
    endif()
endforeach()

include("${RECOMP_ROOT}/cmake/GenesisRecompGenerated.cmake")
genesisrecomp_add_generated_sources(
    JP3_GENERATED_SOURCES jp3 "${CMAKE_CURRENT_SOURCE_DIR}/game" jp3.md)

genesisrecomp_runner_sources(JP3_RUNNER_SOURCES TRACE ON)

add_executable(JamesPond3Recomp
    ${JP3_RUNNER_SOURCES}
    ${JP3_GENERATED_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/game/jp3_spec.c"
)

add_dependencies(JamesPond3Recomp ${JP3_GENERATED_SOURCES_TARGET})
target_compile_definitions(JamesPond3Recomp PRIVATE
    INCLUDE_GENERATED_CODE=1
    ENABLE_RECOMPILED_CODE=1
    OWN_BACKEND=1
    DEFAULT_DEBUG_PORT=4382
    GENESIS_GAME_VERSION="${PROJECT_VERSION}"
)
option(JP3_TRACE_D9BA "Temporary bring-up tracing for the $D9BA state byte" OFF)
if(JP3_TRACE_D9BA)
    target_compile_definitions(JamesPond3Recomp PRIVATE GENESIS_TRACE_D9BA=1)
    # Fixed load address makes __builtin_return_address() resolvable with addr2line.
    target_link_options(JamesPond3Recomp PRIVATE -no-pie)
endif()
# Keep symbols available for source-level bring-up debugging.
target_compile_options(JamesPond3Recomp PRIVATE -g)

target_include_directories(JamesPond3Recomp PRIVATE
    "${RUNNER_ROOT}"
    "${RUNNER_ROOT}/include"
    "${RUNNER_ROOT}/video"
    "${RUNNER_ROOT}/external/ymfm/src"
    "${RUNNER_ROOT}/external/superzazu"
    "${RUNNER_ROOT}/external/clowncommon"
    "${RECOMP_ROOT}/recompiler/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/game"
    ${SDL2_PARENT_INCLUDE_DIRS}
)
target_link_libraries(JamesPond3Recomp PRIVATE SDL2::SDL2main SDL2::SDL2)
if(WIN32)
    target_link_libraries(JamesPond3Recomp PRIVATE ws2_32 comdlg32)
endif()
genesisrecomp_runner_target(JamesPond3Recomp)

# Stage the ROM next to the executable so the runner finds it with no args.
add_custom_command(TARGET JamesPond3Recomp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/game/jp3.md"
        "$<TARGET_FILE_DIR:JamesPond3Recomp>/jp3.md"
    COMMENT "Staging James Pond 3 ROM")
