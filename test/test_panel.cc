/*
 * The arithmetic, checked against three panels of different densities.
 *
 * Every check here is a `static_assert`, so this file failing to compile is the
 * test failing. That is not a trick: the whole point of the library is that a
 * measurement is settled before the device is switched on, and a check that
 * runs at the same moment as the arithmetic it checks is the closest a test can
 * get to the thing itself.
 *
 * The `main` below prints the same figures, so a person can read what the
 * compiler already agreed with.
 */

#include <cstdio>

#include "caliper/panel.h"

namespace {

using namespace cal;

/// The panel this project measures. Its figures are the ones the papers carry.
constexpr Panel five_inch = kJc8048w500;

/// A seven inch panel, computed rather than measured: no such panel was on the
/// desk. It is here because it is the case that breaks the tempting shortcut.
constexpr Panel seven_inch{.width = 1024_pt, .height = 600_pt, .points_per_mm = 6.68f};

/// A five inch panel at 720 by 1280, which is what the boards in PAP-HW-004
/// carry. Same glass, 2.4 times the pixels.
constexpr Panel five_inch_hd{.width = 720_pt, .height = 1280_pt, .points_per_mm = 294.0f / 25.4f};

// A fingertip is nine millimetres wherever it is, and comes out at a different
// number of pixels on each of the three.
static_assert(five_inch(token::kFingertip) == 66_pt);
static_assert(seven_inch(token::kFingertip) == 60_pt);
static_assert(five_inch_hd(token::kFingertip) == 104_pt);

static_assert(five_inch(token::kThumb) == 88_pt);
static_assert(seven_inch(token::kThumb) == 80_pt);
static_assert(five_inch_hd(token::kThumb) == 139_pt);

// This is the mistake the library exists to prevent. Scaling the fingertip of
// the five inch panel by the ratio of the widths gives 84 points on the seven
// inch one, where the answer is 60. Forty per cent out, on every key.
constexpr Point scaled_by_width{Rounded(66.0f * 1024.0f / 800.0f)};
static_assert(scaled_by_width == 84_pt);
static_assert(scaled_by_width != seven_inch(token::kFingertip));

// A line of text is raised by half a descender to look centred, which is three
// points at body size on this panel.
static_assert(five_inch.OpticalOffset(26_pt) == 3_pt);
static_assert(five_inch.OpticalOffset(60_pt) == 6_pt);

// The type scale holds its angle, so the grades follow the density too.
static_assert(five_inch.TypeSize(token::kBody) == 26_pt);
static_assert(seven_inch.TypeSize(token::kBody) == 24_pt);
static_assert(five_inch_hd.TypeSize(token::kBody) == 41_pt);

static_assert(five_inch.TypeSize(token::kTitle) == 60_pt);
static_assert(seven_inch.TypeSize(token::kTitle) == 54_pt);
static_assert(five_inch_hd.TypeSize(token::kTitle) == 94_pt);

// A corner inside another, computed from the one around it.
static_assert(five_inch.InnerRadius(token::kRadiusPanel) == 8_pt);
static_assert(seven_inch.InnerRadius(token::kRadiusPanel) == 7_pt);

// A control carrying text is its label plus its padding, and never under the
// floor a control has to reach. A short label is therefore held up by it.
static_assert(five_inch.MinimumWidth(20_pt) == 60_pt);
static_assert(five_inch.MinimumWidth(200_pt) == 240_pt);
static_assert(seven_inch.MinimumWidth(20_pt) == 56_pt);

// A typeface gives back whatever height a line happens to need, and 43 is
// what a heading measures on the five inch panel. Rounded up to the grid, so
// whatever is stacked beneath it lands on an even point instead of an odd one.
static_assert(five_inch.RoundedUpToGrid(43_pt) == 44_pt);
static_assert(five_inch.RoundedUpToGrid(88_pt) == 88_pt);

// The units keep each other out. None of the following compiles, and that is
// the deliverable:
//   Point wrong = token::fingertip;      // a millimetre is not a pixel
//   five_inch(66_pt);                    // pixels do not need converting
//   five_inch.type_size(token::body.value);  // a float is not a distance

/**
 * Prints what the compiler has already checked.
 *
 * @param panel The panel to report on.
 * @param name What to call it in the output.
 */
void report(const Panel& panel, const char* name) {
   std::printf("%-14s %4d x %-4d  %5.2f pt/mm   fingertip %3d   body %2d   edge %2d\n", name, panel.width.value,
               panel.height.value, static_cast<double>(panel.points_per_mm), panel(token::kFingertip).value,
               panel.TypeSize(token::kBody).value, panel(token::kEdge).value);
}

}  // namespace

int main() {
   report(five_inch, "5 inch");
   report(seven_inch, "7 inch");
   report(five_inch_hd, "5 inch HD");

   std::printf(
       "\nthe same fingertip scaled by width instead: %d points on the seven "
       "inch panel, where the answer is %d\n",
       scaled_by_width.value, seven_inch(token::kFingertip).value);
   return 0;
}
