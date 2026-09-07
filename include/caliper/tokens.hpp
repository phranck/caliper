// Generated from tokens/caliper.toml by tools/generate_tokens.py.
// Do not edit: the next build overwrites this file.

#pragma once

#include <cstdint>

#include "caliper/units.hpp"

namespace cal::token {

// Spacings. Millimetres, because a margin answers to an eye and a finger
// rather than to a pixel count.
inline constexpr Millimeter edge{3.27f};
inline constexpr Millimeter group{3.27f};
inline constexpr Millimeter inset{2.72f};
inline constexpr Millimeter line_gap{1.63f};

// What a hand needs. The floor every touch target is measured against.
inline constexpr Millimeter fingertip{9.0f};
inline constexpr Millimeter thumb{12.0f};

// The type scale as a rule: every grade is a cap height seen under one
// angle from the distance it is meant to be read at.
inline constexpr float cap_angle_mrad = 6.1f;
inline constexpr float cap_ratio = 0.705f;
inline constexpr float x_height_ratio = 0.505f;
inline constexpr float descender_ratio = 0.24f;
inline constexpr Reading title{945.0f};
inline constexpr Reading heading{565.0f};
inline constexpr Reading body{410.0f};
inline constexpr Reading small{345.0f};

// Only the outer corners. Anything nested is computed by inner_radius
// below, because two numbers drift into a pinched corner.
inline constexpr Millimeter radius_panel{4.35f};
inline constexpr Millimeter radius_tile{2.72f};
inline constexpr Millimeter radius_artwork{1.63f};
inline constexpr float squircle = 3.2f;
inline constexpr float squircle_artwork = 8.0f;

/// A corner inside another is the one around it less the space between
/// them. Stated as a rule rather than as a second number, so the two
/// cannot fall out of step when either moves.
constexpr Millimeter inner_radius(Millimeter outer, Millimeter gap = inset)
{
    return Millimeter{outer.value - gap.value};
}

// The grid every edge lands on, in whole pixels.
inline constexpr Point grid{2};

// The palette. A colour has no measurement, so these pass through as they
// were written.
inline constexpr std::uint32_t bg = 0x17120e;
inline constexpr std::uint32_t surface = 0x221a14;
inline constexpr std::uint32_t raised = 0x30251b;
inline constexpr std::uint32_t key = 0x3d3126;
inline constexpr std::uint32_t key_edge = 0x45382b;
inline constexpr std::uint32_t ink = 0xfaf4ec;
inline constexpr std::uint32_t muted = 0xc3b1a0;
inline constexpr std::uint32_t faint = 0x8d7c6c;
inline constexpr std::uint32_t accent = 0xf0a85c;
inline constexpr std::uint32_t accent_ink = 0x2b1806;
inline constexpr std::uint32_t accent_dim = 0x6b4415;
inline constexpr std::uint32_t line = 0x3d3128;
inline constexpr std::uint32_t overlay = 0x3f3226;
inline constexpr std::uint32_t danger = 0xe8705a;
inline constexpr std::uint32_t danger_ink = 0x2b0c06;

}  // namespace cal::token
