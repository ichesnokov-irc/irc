cmake_minimum_required(VERSION 3.16)
project(irc VERSION 1.0.0 LANGUAGES CXX)

add_library(irc INTERFACE)
add_library(irc::irc ALIAS irc)

target_include_directories(irc INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    add_subdirectory(tests)
endif()