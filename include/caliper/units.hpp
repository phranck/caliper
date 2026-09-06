#pragma once

#include <cstdint>

/**
 * The two kinds of measurement an interface on a panel is made of, as separate
 * types.
 *
 * A millimetre and a pixel are both numbers, and nothing in C stops one being
 * written where the other belongs. That mistake has already been made once on a
 * real panel in this project. As distinct types the compiler refuses it, and it
 * cannot be written at all.
 *
 * Everything here is `constexpr`, so a panel known when the firmware is built
 * costs nothing at run time; a panel established at start-up runs the same line
 * then.
 */
namespace cal {

/**
 * A pixel of the panel.
 *
 * Whole, because a pixel is. This is what every conversion ends in and the only
 * one of these types a graphics library ever sees.
 */
struct Point {
    std::int32_t value;

    friend constexpr bool operator==(Point, Point) = default;
};

/**
 * A physical size on the glass.
 *
 * It stays the same on every panel there is, which is the whole reason it
 * exists: a fingertip is nine millimetres whatever the density beneath it.
 */
struct Millimeter {
    float value;

    friend constexpr bool operator==(Millimeter, Millimeter) = default;
};

/**
 * How far away something is meant to be read from, in millimetres.
 *
 * A type size is stated this way rather than in points because the distance is
 * the question one actually has about it. What size it comes out at follows
 * from the distance, the angle a capital has to hold and the panel.
 */
struct Reading {
    float millimetres;

    friend constexpr bool operator==(Reading, Reading) = default;
};

/// A size on the glass, written as `9.0_mm`.
constexpr Millimeter operator""_mm(long double size)
{
    return Millimeter{static_cast<float>(size)};
}

/// A whole number of pixels, written as `800_pt`.
constexpr Point operator""_pt(unsigned long long size)
{
    return Point{static_cast<std::int32_t>(size)};
}

/// A reading distance, written as `400.0_read`.
constexpr Reading operator""_read(long double distance)
{
    return Reading{static_cast<float>(distance)};
}

/**
 * Rounds to the nearest whole number, halves away from zero.
 *
 * The standard library's own rounding is not usable at compile time in this
 * standard, and the rule matters beyond that: the generator that writes the
 * design values rounds the same way in Python, so a value converted here and
 * the same value converted there cannot land on different pixels. Python's own
 * `round` breaks ties towards even, which would differ at exactly one half.
 *
 * @param value The number to round.
 * @returns The nearest whole number.
 */
constexpr std::int32_t rounded(float value)
{
    return static_cast<std::int32_t>(value < 0.0f ? value - 0.5f : value + 0.5f);
}

}  // namespace cal
