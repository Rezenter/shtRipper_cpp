cmake_minimum_required(VERSION 3.20)
project(shtRipper)

set(CMAKE_CXX_STANDARD 20)

#set(CMAKE_CXX_FLAGS_RELEASE "-O3")
string(REPLACE "-O2" "-Os" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
string(REPLACE "-O2" "-Os" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${BUILD_ARCH}")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${BUILD_ARCH}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${BUILD_ARCH}")

include_directories(${CMAKE_SOURCE_DIR} include)

#add_executable(shtRipper src/main.cpp version.h src/compress.cpp)
#add_dependencies(shtRipper auto_increment_version)

add_library(ripperForPython MODULE src/pythonWrapper.cpp version.h src/compress.cpp)

install(TARGETS ripperForPython
        CONFIGURATIONS Debug
        LIBRARY DESTINATION ${CMAKE_SOURCE_DIR}/distribution/)
install(TARGETS ripperForPython
        CONFIGURATIONS Release
        LIBRARY DESTINATION ${CMAKE_SOURCE_DIR}/distribution/)

ADD_CUSTOM_TARGET(
        auto_increment_version
        ${CMAKE_COMMAND}
        -D VERSION_FILE=${CMAKE_SOURCE_DIR}/version.h
        -P ${CMAKE_SOURCE_DIR}/AutoVersion.cmake
)