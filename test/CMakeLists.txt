cmake_minimum_required(VERSION 3.16)

project(caliper_tests CXX)

# The core carries no dependency on ESP-IDF or on LVGL, which is what lets the
# arithmetic be checked here, on a machine, without a board attached.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(test_panel test_panel.cpp)

target_include_directories(test_panel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# The same settings the device is built with, so anything that would fail there
# fails here first.
target_compile_options(test_panel PRIVATE
    -Wall -Wextra -Werror -fno-exceptions -fno-rtti)

enable_testing()
add_test(NAME panel COMMAND test_panel)
