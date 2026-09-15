cmake_minimum_required(VERSION 4.4)

project(flagV
 VERSION 0.1
 LANGUAGES CXX)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_library(DecoderLib STATIC src/decode.cpp src/system.cpp src/cpu_exec.cpp)

target_include_directories(DecoderLib PUBLIC src/)

add_executable(MainApp src/main.cpp)

target_link_libraries(MainApp PRIVATE DecoderLib)

add_executable(ProcessObjdump others/processObjdump.cpp)

target_link_libraries(ProcessObjdump PRIVATE DecoderLib)

add_executable(OtherTests others/tests.cpp)
target_link_libraries(OtherTests PRIVATE DecoderLib)


if(MSVC)
 target_compile_options(MainApp PRIVATE /W4 /WX)
 target_compile_options(ProcessObjdump PRIVATE /W4 /WX)
else()
  set(MY_PROJECT_WARNINGS
    -Wall
    -Wextra
    -Wshadow
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wold-style-cast
)
 target_compile_options(MainApp PRIVATE ${MY_PROJECT_WARNINGS})
 target_compile_options(ProcessObjdump PRIVATE -Wall -Wextra -pedantic)
 target_compile_options(OtherTests PRIVATE -Wall -Wextra -pedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
