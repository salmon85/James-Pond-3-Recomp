# Point CMake at the Zig toolchain bundled beside this file.
#
# Zig provides the compiler driver (`zig cc` / `zig c++`) and the archiver
# (`zig ar` / `zig ranlib`); the builder payload contains no other compiler.
# The host ABI is the default target on both Windows and Linux.

if(CMAKE_HOST_WIN32)
    set(_zig "${CMAKE_CURRENT_LIST_DIR}/zig/zig.exe")
else()
    set(_zig "${CMAKE_CURRENT_LIST_DIR}/zig/zig")
endif()

set(CMAKE_C_COMPILER "${_zig}")
set(CMAKE_C_COMPILER_ARG1 cc)
set(CMAKE_CXX_COMPILER "${_zig}")
set(CMAKE_CXX_COMPILER_ARG1 c++)

set(CMAKE_AR "${_zig}")
set(CMAKE_RANLIB "${_zig}")

# The generated runner embeds __DATE__/__TIME__; Zig's clang promotes that
# warning to an error under the project's -Werror.
set(CMAKE_C_FLAGS_INIT "-Wno-date-time")
set(CMAKE_CXX_FLAGS_INIT "-Wno-date-time")
