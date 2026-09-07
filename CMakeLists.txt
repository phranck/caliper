# Caliper as an ESP-IDF component.
#
# The headers carry the arithmetic and are usable on their own, which is what
# lets the tests run on a machine with no board. The sources here are the part
# that touches the graphics library, so they only build where it exists.

idf_component_register(SRCS "src/theme.cc" "src/components.cc" "src/inspect.cc"
                       INCLUDE_DIRS "include"
                       REQUIRES lvgl log)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_20)
