// Generated from tokens/caliper.toml by tools/generate_tokens.py.
// Do not edit: the next build overwrites this file.

#ifndef CALIPER_TOKENS_H_
#define CALIPER_TOKENS_H_

#include <cstdint>

#include "caliper/units.h"

namespace cal::token {

// Spacings. Millimetres, because a margin answers to an eye and a finger
// rather than to a pixel count.
inline constexpr Millimeter kEdge{2.18f};
inline constexpr Millimeter kGroup{3.27f};
inline constexpr Millimeter kInset{2.72f};
inline constexpr Millimeter kLineGap{1.63f};
inline constexpr Millimeter kFloating{1.09f};
inline constexpr Millimeter kKeyboardGap{1.09f};
inline constexpr Millimeter kPlayerInset{1.09f};
inline constexpr Millimeter kStatusEdge{1.09f};
inline constexpr Millimeter kSliderLabel{2.18f};
inline constexpr Millimeter kSubTextGap{0.82f};
inline constexpr Millimeter kStatusGap{2.18f};

// The heights that decide where the content area begins and ends:
// StatusBar and Header above it, and the room reserved below it.
inline constexpr Millimeter kStatusBar{4.35f};
inline constexpr Millimeter kHeader{9.25f};
inline constexpr Millimeter kFooter{10.88f};
inline constexpr Millimeter kButton{6.53f};
inline constexpr Millimeter kCard{68.0f};
inline constexpr Millimeter kCardSymbol{5.44f};
inline constexpr Millimeter kContentUnavailableSymbol{7.07f};
inline constexpr Millimeter kKeyboardKey{7.1f};
inline constexpr Millimeter kKeyboardKeyHigh{6.8f};
inline constexpr Millimeter kKeyboardEdge{1.36f};
inline constexpr Millimeter kKeyboardLayer{11.4f};
inline constexpr Millimeter kKeyboardSymbols{11.4f};
inline constexpr Millimeter kKeyboardConfirm{20.9f};
inline constexpr Millimeter kKeyboardEscape{12.0f};
inline constexpr Millimeter kKeyboardChar{2.18f};
inline constexpr Millimeter kKeyboardShifted{1.4f};
inline constexpr Millimeter kKeyboardStack{0.27f};
inline constexpr Millimeter kKeyboardMark{2.2f};
inline constexpr Millimeter kKeyboardCorner{0.54f};
inline constexpr Millimeter kMediaKey{6.8f};
inline constexpr Millimeter kTrack{2.18f};
inline constexpr Millimeter kSliderKnob{5.99f};
inline constexpr Millimeter kSidebar{11.97f};
inline constexpr Millimeter kSidebarItem{10.88f};

// What a hand needs. The floor every touch target is measured against.
inline constexpr Millimeter kFingertip{9.0f};
inline constexpr Millimeter kThumb{12.0f};
inline constexpr Millimeter kMinimum{6.53f};

// The type scale as a rule: every grade is a cap height seen under one
// angle from the distance it is meant to be read at.
inline constexpr float kCapAngleMrad = 6.1f;
inline constexpr float kCapRatio = 0.7f;
inline constexpr float kXHeightRatio = 0.506f;
inline constexpr float kDescenderRatio = 0.2f;
inline constexpr Reading kTitle{935.0f};
inline constexpr Reading kHeading{560.0f};
inline constexpr Reading kBody{405.0f};
inline constexpr Reading kSmall{345.0f};
inline constexpr Reading kStatus{265.0f};
inline constexpr Reading kCaption{203.0f};
inline constexpr Reading kKeyLabel{405.0f};

// Only the outer corners. Anything nested is computed by inner_radius
// below, because two numbers drift into a pinched corner.
inline constexpr Millimeter kRadiusCard{3.81f};
inline constexpr Millimeter kRadiusArtwork{1.63f};
inline constexpr float kSquircle = 3.2f;
inline constexpr float kSquircleArtwork = 8.0f;

/// A corner inside another is the one around it less the space between
/// them. Stated as a rule rather than as a second number, so the two
/// cannot fall out of step when either moves.
constexpr Millimeter InnerRadius(Millimeter outer, Millimeter gap = kInset) {
   return Millimeter{outer.value - gap.value};
}

// The corner inside the card's own, so a button matches whatever surface
// it stands on rather than carrying a second figure that can drift from it.
inline constexpr Millimeter kRadiusButton = InnerRadius(kRadiusCard, kInset);

// The grid every edge lands on, in whole pixels.
inline constexpr Point kGrid{2};

// The panels. Width and height are the pixels a panel has and the
// density is what turns a millimetre above into a number of them.
// `Panel` itself is built from these in `panel.h`, which cannot be
// done here because the type lives there and includes this.
inline constexpr Point kPanelJc8048w500Width{800};
inline constexpr Point kPanelJc8048w500Height{480};
inline constexpr float kPanelJc8048w500PointsPerMm = 7.35f;

// The palette. A colour has no measurement, so these pass through as they
// were written.
inline constexpr std::uint32_t kBg = 0x111111;
inline constexpr std::uint32_t kSurface = 0x1b1b1b;
inline constexpr std::uint32_t kRaised = 0x262626;
inline constexpr std::uint32_t kKeyTray = 0x1b1b1b;
inline constexpr std::uint32_t kKeyFiller = 0x282828;
inline constexpr std::uint32_t kKeyQuiet = 0x3a3a3a;
inline constexpr std::uint32_t kKey = 0x626262;
inline constexpr std::uint32_t kKeyInk = 0xececec;
inline constexpr std::uint32_t kKeyShifted = 0xaaaaaa;
inline constexpr std::uint32_t kKeyMark = 0xc0c0c0;
inline constexpr std::uint32_t kInk = 0xececec;
inline constexpr std::uint32_t kStatusInk = 0xf0f0f0;
inline constexpr std::uint32_t kMuted = 0x939393;
inline constexpr std::uint32_t kFaint = 0x757575;
inline constexpr std::uint32_t kAccent = 0x009ce9;
inline constexpr std::uint32_t kLine = 0x353535;
inline constexpr std::uint32_t kOverlay = 0x323232;
inline constexpr std::uint32_t kDanger = 0xf85149;
inline constexpr std::uint32_t kDangerInk = 0xffffff;

}  // namespace cal::token

#endif  // CALIPER_TOKENS_H_
