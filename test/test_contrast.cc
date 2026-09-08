/*
 * Which ink stands on which ground.
 *
 * This is the one piece of arithmetic in the library whose mistakes do not look
 * like mistakes. A wrong measurement puts something in the wrong place and
 * somebody sees it; a wrong ink is simply dim, and dim reads as a design
 * decision until the panel is held at arm's length in a bright room.
 *
 * The figures below are the ones the design states about itself, so this is
 * also the check that the rule and the drawing still agree.
 */

#include <cstdio>

#include "caliper/contrast.h"
#include "caliper/tokens.h"

namespace {

using namespace cal;

/// How close two ratios have to be to count as the same figure. They are
/// written to two places wherever anyone quotes them.
constexpr float kCloseEnough = 0.005f;

bool Near(float value, float expected) {
   const float apart = value > expected ? value - expected : expected - value;
   return apart < kCloseEnough;
}

int failures = 0;

void Expect(bool held, const char* what) {
   if (!held) {
      std::printf("contrast: %s\n", what);
      failures += 1;
   }
}

}  // namespace

int main() {
   // What the design says about its own accent, in its own words: white on it
   // stands at 3.03 to one, and it against the ground at 6.23. The second of
   // those said 6.25 until this test was written, which is how long a figure in
   // a comment survives a change to the colour it was measured against.
   Expect(Near(Contrast(0xffffff, token::kAccent), 3.03f), "white on the accent is not 3.03");
   Expect(Near(Contrast(token::kAccent, token::kBg), 6.23f), "the accent on the ground is not 6.23");

   // And therefore white stands on it, which is what the design draws. It sits
   // a hundredth above the floor, so of everything this rule is ever asked
   // about, this is the case that would turn over first.
   Expect(InkOn(token::kAccent) == 0xffffff, "the accent does not carry white");

   // A light accent cannot. Yellow at full strength gives white 1.07 to one,
   // which is no contrast at all, and takes the ground's own colour instead.
   Expect(InkOn(0xffcc00) == token::kBg, "a light accent still carries white");
   Expect(InkOn(0xffffff) == token::kBg, "white carries white");

   // A dark one keeps it.
   Expect(InkOn(0x000000) == 0xffffff, "black does not carry white");

   // Whatever the ink turns out to be, it holds the floor a large word needs.
   // That is the whole point of choosing it rather than stating it.
   constexpr std::uint32_t candidates[] = {0x009ce9, 0xff2d55, 0xff9500, 0xffcc00, 0x34c759,
                                           0x00c7be, 0x007aff, 0xaf52de, 0x000000, 0xffffff};
   for (std::uint32_t colour : candidates) {
      Expect(Contrast(InkOn(colour), colour) >= kLargeTextContrast, "an ink falls under the floor");
   }

   // The dimmed accent is darker than the accent and lighter than the ground,
   // which is what makes it read as the same colour turned down rather than as
   // a second colour or as the ground itself.
   const std::uint32_t dimmed = DimmedAccent(token::kAccent);
   Expect(Luminance(dimmed) < Luminance(token::kAccent), "the dimmed accent is not darker");
   Expect(Luminance(dimmed) > Luminance(token::kBg), "the dimmed accent is not lighter than the ground");

   if (failures == 0) {
      std::printf("contrast: every ink holds\n");
   }
   return failures;
}
