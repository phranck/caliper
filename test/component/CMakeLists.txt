cmake_minimum_required(VERSION 3.16)

# The smallest project that builds Caliper as what it is: an ESP-IDF component
# against LVGL. The checks under test/ measure the arithmetic on a machine with
# no board, and that half can be green whilst the half that draws does not
# compile at all. This is the one that catches that.
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../..")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(caliper_component)
