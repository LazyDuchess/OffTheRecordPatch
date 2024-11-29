cmake_minimum_required(VERSION 3.10)

project(OffTheRecordPatch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)

FetchContent_Declare(
  MinHook
  GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
  GIT_TAG        master
)

FetchContent_MakeAvailable(MinHook)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h" "include/*/*.h")

add_library(OffTheRecordPatch SHARED ${SOURCES} ${HEADERS})

target_include_directories(OffTheRecordPatch PRIVATE include minhook/include)

target_link_libraries(OffTheRecordPatch PRIVATE minhook)

set_target_properties(OffTheRecordPatch PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    SUFFIX ".asi"
)

target_compile_definitions(OffTheRecordPatch PRIVATE OFFTHERECORDPATCH_EXPORTS)
