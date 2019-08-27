cmake_minimum_required(VERSION 3.10)

project(ipc
        VERSION 0.1
        DESCRIPTION "ipc"
        LANGUAGES CXX)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)
set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer -O0")
set(
  CMAKE_EXE_LINKER_FLAGS_DEBUG
  "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# find_package(Doxygen REQUIRED dot)

include_directories(include extern/include)
link_directories(build)

set(sources src/daemon.cpp)

add_executable(ipc ${sources})
target_compile_features(ipc PUBLIC cxx_std_17)
target_compile_options(ipc PRIVATE -pedantic -Wall -Wextra)
target_link_libraries(ipc common pthread)

add_executable(client src/client.cpp)
target_compile_features(client PUBLIC cxx_std_17)
target_compile_options(client PRIVATE -pedantic -Wall -Wextra)
target_link_libraries(client common pthread)

# add_subdirectory(test)

# doxygen_add_docs(docs src include)