cmake_minimum_required(VERSION 3.16)
project(shtRipper)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "-static")
set(BUILD_ARCH "-m64")

set(CMAKE_CXX_FLAGS_RELEASE "-Os")


SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${BUILD_ARCH}")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${BUILD_ARCH}")



include_directories(${CMAKE_SOURCE_DIR} include)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(shtRipper src/main.cpp version.h src/magn_file.cpp src/compress.cpp)
add_dependencies(shtRipper auto_increment_version)


ADD_CUSTOM_TARGET(
        auto_increment_version
        ${CMAKE_COMMAND}
        -D VERSION_FILE=${CMAKE_SOURCE_DIR}/version.h
        -P ${CMAKE_SOURCE_DIR}/AutoVersion.cmake
)