cmake_minimum_required(VERSION 3.16)

project(caliper_tests CXX)

# The core carries no dependency on ESP-IDF or on LVGL, which is what lets the
# arithmetic be checked here, on a machine, without a board attached.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The same settings the device is built with, so anything that would fail there
# fails here first.
# Most of these are arithmetic in a header and need nothing beside them. The
# contrast is not: its curve is in a source file, because the transfer function
# it uses is not something a constant expression can compute.
function(caliper_test name)
    set(sources ${name}.cc ${ARGN})
    add_executable(${name} ${sources})
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    target_compile_options(${name} PRIVATE
        -Wall -Wextra -Werror -fno-exceptions -fno-rtti)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

enable_testing()

caliper_test(test_panel)
caliper_test(test_checks)
caliper_test(test_contrast ../src/contrast.cc)
