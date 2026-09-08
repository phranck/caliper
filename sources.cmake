# What the library is built from, named once for every build that uses it.
#
# The ESP-IDF component reads this, and so does a build on a desk. Two lists
# would be two answers to the same question, and the one that is not the build
# somebody is running is the one that goes stale.
set(CALIPER_ROOT "${CMAKE_CURRENT_LIST_DIR}")

set(CALIPER_SOURCES
    "${CALIPER_ROOT}/src/contrast.cc"
    "${CALIPER_ROOT}/src/shapes.cc"
    "${CALIPER_ROOT}/src/squircles.cc"
    "${CALIPER_ROOT}/src/theme.cc"
    "${CALIPER_ROOT}/src/components.cc"
    "${CALIPER_ROOT}/src/inspect.cc")
