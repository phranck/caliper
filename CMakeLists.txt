# Caliper as an ESP-IDF component.
#
# The headers carry the arithmetic and are usable on their own, which is what
# lets the tests run on a machine with no board. The sources here are the part
# that touches the graphics library, so they only build where it exists.

include("${CMAKE_CURRENT_LIST_DIR}/sources.cmake")

idf_component_register(SRCS ${CALIPER_SOURCES}
                       INCLUDE_DIRS "include"
                       REQUIRES lvgl log)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_20)
