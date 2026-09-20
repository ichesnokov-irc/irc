cmake_minimum_required(VERSION 3.16)
project(irc_tests CXX)

include(CTest)

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(Catch2)

add_executable(irc_tests
    main.cpp
    basic_set_test.cpp
)

target_include_directories(irc_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include")

target_compile_features(irc_tests PRIVATE cxx_std_20)
set_target_properties(irc_tests PROPERTIES 
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(irc_tests PRIVATE Catch2::Catch2)

if(MSVC)
    target_compile_options(irc_tests PRIVATE /W4 /WX)
else()
    target_compile_options(irc_tests PRIVATE -Wall -Wextra -Werror -Wpedantic)
endif()

include(Catch)
catch_discover_tests(irc_tests)
