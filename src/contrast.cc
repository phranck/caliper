#include "caliper/contrast.h"

#include <cmath>

#include "caliper/tokens.h"

namespace cal {
namespace {

/// How far a dimmed accent is taken towards the ground. Rather more than half
/// way, which is where the design's own dimmed accent sits. No single share
/// reproduces that one exactly, because it was picked by eye rather than
/// derived, and at this share its three channels come back within a few points.
constexpr float kTowardsGround = 0.55f;

/// How far the fill under an action is taken towards the ground. Enough that
/// white reaches the floor above on every one of the sixteen accents: the one
/// that needs the most is the yellow, at this share exactly, and it is the
/// share every other one is given too so that one rule answers for all of them.
///
/// Taking the fill down rather than choosing an ink per colour is what lets the
/// word on an action always be white. An ink that changes with the colour under
/// it is read as two different controls.
constexpr float kTowardsAction = 0.32f;

/// Where the two halves of the transfer curve meet, and the constants of each.
/// They are the ones the standard states and are not adjustable.
constexpr float kCurveKnee = 0.04045f;
constexpr float kBelowKnee = 12.92f;
constexpr float kOffset = 0.055f;
constexpr float kScale = 1.055f;
constexpr float kExponent = 2.4f;

/// What the ratio of two luminances is offset by, so that black against black
/// comes out as one rather than as a division by nothing.
constexpr float kFloor = 0.05f;

/// One channel of a colour, on the scale light is measured in.
float Linear(std::uint32_t channel) {
   const float share = static_cast<float>(channel) / 255.0f;
   return share <= kCurveKnee ? share / kBelowKnee : std::pow((share + kOffset) / kScale, kExponent);
}

}  // namespace

float Luminance(std::uint32_t colour) {
   return 0.2126f * Linear((colour >> 16) & 0xFF) + 0.7152f * Linear((colour >> 8) & 0xFF) +
          0.0722f * Linear(colour & 0xFF);
}

float Contrast(std::uint32_t one, std::uint32_t other) {
   const float first = Luminance(one) + kFloor;
   const float second = Luminance(other) + kFloor;
   return first > second ? first / second : second / first;
}

std::uint32_t InkOn(std::uint32_t colour) {
   return Contrast(0xffffff, colour) >= kLargeTextContrast ? 0xffffff : token::kBg;
}

std::uint32_t ActionFill(std::uint32_t colour) {
   std::uint32_t taken = 0;
   for (int shift = 16; shift >= 0; shift -= 8) {
      const float here = static_cast<float>((colour >> shift) & 0xFF);
      const float there = static_cast<float>((token::kBg >> shift) & 0xFF);
      taken |= static_cast<std::uint32_t>(here + (there - here) * kTowardsAction + 0.5f) << shift;
   }
   return taken;
}

std::uint32_t DimmedAccent(std::uint32_t colour) {
   std::uint32_t dimmed = 0;
   for (int shift = 16; shift >= 0; shift -= 8) {
      const float here = static_cast<float>((colour >> shift) & 0xFF);
      const float there = static_cast<float>((token::kBg >> shift) & 0xFF);
      dimmed |= static_cast<std::uint32_t>(here + (there - here) * kTowardsGround + 0.5f) << shift;
   }
   return dimmed;
}

}  // namespace cal
