cmake_minimum_required(VERSION 3.17)

project(TestTask)

set(SOURCES src/main.cpp)

set(HEADERS inc/ivfs.h inc/utils.h inc/vfs.h)

add_executable(app ${SOURCES} ${HEADERS})

set_target_properties(app PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_include_directories(app PRIVATE inc)