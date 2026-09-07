/*
 * The six checks, each made to fail once on purpose.
 *
 * A check that has never been red proves nothing, so every rule here appears
 * twice: an element that holds it and an element that breaks it. The counts are
 * `static_assert`ed, which means a rule that stops firing breaks the build
 * rather than going quiet.
 */

#include <cstdio>

#include "caliper/checks.hpp"

namespace {

using namespace cal;

constexpr Panel panel = jc8048w500;

/// The bands of a screen of this design: a status bar of 32 and a header of 68
/// above, a footer of 112 below.
constexpr Bands bands{.content_top = Point{100}, .content_bottom = Point{368}};

/// Counts findings without reporting them, which is what a compile-time check
/// can do.
constexpr int findings(const Element &element)
{
    return check(element, panel, bands, nullptr);
}

/// An element that holds every rule: inside the margins, inside the bands, on
/// the grid, large enough for a finger, its text fitting, its corner concentric.
constexpr Element sound{.name = "sound",
                        .left = Point{24},
                        .top = Point{100},
                        .width = Point{240},
                        .height = Point{68},
                        .touchable = true,
                        .text_width = Point{160},
                        .text_padding = Point{20},
                        .radius = Point{12},
                        .outer_radius = Point{32},
                        .outer_gap = Point{20}};

static_assert(findings(sound) == 0);

// A touch target under a fingertip, which is 66 points on this panel.
constexpr Element small_target = [] {
    Element element = sound;
    element.name = "Fertig";
    element.height = Point{48};
    return element;
}();
static_assert(findings(small_target) == 1);

// Against the edge of the panel, where the margin is 24.
constexpr Element at_the_edge = [] {
    Element element = sound;
    element.left = Point{8};
    return element;
}();
static_assert(findings(at_the_edge) == 1);

// A label wider than the surface carrying it, which is what a second language
// does to a control sized for the first.
constexpr Element overflowing = [] {
    Element element = sound;
    element.text_width = Point{220};
    return element;
}();
static_assert(findings(overflowing) == 1);

// A corner that is not the outer one less the inset, which is the pinched
// corner one sees and cannot name.
constexpr Element pinched = [] {
    Element element = sound;
    element.radius = Point{16};
    return element;
}();
static_assert(findings(pinched) == 1);

// An edge between two points of the grid.
constexpr Element off_grid = [] {
    Element element = sound;
    element.left = Point{25};
    return element;
}();
static_assert(findings(off_grid) == 1);

// Content reaching under the footer.
constexpr Element under_the_band = [] {
    Element element = sound;
    element.height = Point{300};
    return element;
}();
static_assert(findings(under_the_band) == 1);

// One element can break several rules at once, and each is reported. This one
// breaks four rules across five findings: too short for a finger, past the left
// margin, its text twice the room it has, and two of its four edges between
// points of the grid.
constexpr Element hopeless = [] {
    Element element = sound;
    element.left = Point{7};
    element.height = Point{47};
    element.text_width = Point{500};
    return element;
}();
static_assert(findings(hopeless) == 5);

/// Prints a finding the way the device would.
void print(const Finding &finding)
{
    std::printf("caliper: %s, \"%s\" is %d, needs %d\n",
                name_of(finding.rule), finding.name,
                finding.actual, finding.required);
}

/**
 * Runs one element through the checks and prints what it found.
 *
 * @param element The element.
 * @param what What the element is meant to demonstrate.
 */
void run(const Element &element, const char *what)
{
    std::printf("%-22s ", what);
    const int count = check(element, panel, bands, nullptr);
    if (count == 0) {
        std::printf("clean\n");
        return;
    }
    std::printf("%d finding%s\n", count, count == 1 ? "" : "s");
    check(element, panel, bands, print);
}

}  // namespace

int main()
{
    run(sound, "sound");
    run(small_target, "too small for a finger");
    run(at_the_edge, "against the edge");
    run(overflowing, "text too wide");
    run(pinched, "pinched corner");
    run(off_grid, "off the grid");
    run(under_the_band, "under the footer");
    run(hopeless, "several at once");
    return 0;
}
