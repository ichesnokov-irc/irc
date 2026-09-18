cmake_minimum_required(VERSION 3.16)

add_executable(irc_tests test.cpp)

target_compile_features(irc_tests PRIVATE cxx_std_20)
set_target_properties(irc_tests PROPERTIES 
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(irc_tests PRIVATE irc)

if(MSVC)
    target_compile_options(irc_tests PRIVATE /W4 /WX)
else()
    target_compile_options(irc_tests PRIVATE -Wall -Wextra -Werror -Wpedantic)
endif()

add_test(NAME RunIrcTests COMMAND irc_tests)
