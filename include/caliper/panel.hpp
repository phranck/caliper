#pragma once

#include "caliper/tokens.hpp"
#include "caliper/units.hpp"

namespace cal {

/**
 * The panel everything is computed for.
 *
 * Two things come out of it and they do not scale alike. How wide a column is
 * follows from the width in pixels. How large a touch target has to be follows
 * from a finger, and a finger is the same size on every panel, so it follows
 * from the density instead.
 *
 * Converting the second one with a factor taken from the width is wrong by a
 * wide margin. A fingertip is 66 points on the five inch panel this project
 * measures and 60 on a seven inch one, whilst a factor from the width would say
 * 84. That is forty per cent out, on every single key.
 */
struct Panel {
    /// How many pixels across the visible area is.
    Point width;

    /// How many pixels down.
    Point height;

    /// How many pixels fall on one millimetre of glass.
    float points_per_mm;

    /**
     * Converts a physical size into pixels on this panel.
     *
     * Written as a call so that the panel is named at the point of conversion:
     * `panel(9.0_mm)` reads as the question it answers, and there is no way to
     * ask it without saying which panel.
     *
     * @param size The size on the glass.
     * @returns The size in whole pixels.
     */
    constexpr Point operator()(Millimeter size) const
    {
        return Point{rounded(size.value * points_per_mm)};
    }

    /**
     * The type size a text needs to stay legible from a given distance.
     *
     * A capital appears to the eye under an angle, and legibility follows that
     * angle rather than the size on the glass. From the distance and the angle
     * comes the cap height in millimetres, from the density its height in
     * pixels, and from the proportions of the typeface the type size.
     *
     * @param distance How far away the text is meant to be read from.
     * @returns The type size in whole pixels.
     */
    constexpr Point type_size(Reading distance) const
    {
        const float cap_height_mm = token::cap_angle_mrad / 1000.0f * distance.millimetres;
        return Point{rounded(cap_height_mm * points_per_mm / token::cap_ratio)};
    }

    /**
     * The smallest a control carrying text may be, in pixels.
     *
     * A fixed width truncates a longer word the moment the interface carries a
     * second language, so a control sizes to its label plus its padding and
     * never below what a finger needs. The width of the label itself is
     * measured by whoever has the typeface and is passed in, because this file
     * carries no dependency on a graphics library.
     *
     * @param label_width How wide the label came out, measured in the typeface
     *                    it is set in.
     * @param padding The space between the label and each edge of the control.
     * @param floor The smallest the control may be regardless of its label,
     *              which is a fingertip unless the design says otherwise.
     * @returns The width to give the control.
     */
    constexpr Point minimum_width(Point label_width,
                                  Millimeter padding = token::inset,
                                  Millimeter floor = token::minimum) const
    {
        const std::int32_t around_the_label = label_width.value + 2 * (*this)(padding).value;
        const std::int32_t needed_by_a_finger = (*this)(floor).value;
        return Point{around_the_label > needed_by_a_finger ? around_the_label
                                                           : needed_by_a_finger};
    }

    /**
     * How far a line of text has to move up to look centred.
     *
     * A text box is taller than the letters in it: it reaches from the top of
     * the ascenders to the bottom of the descenders, and a word without a
     * descender therefore sits above the middle of its own box whilst a word
     * with one sits below. Centring the box centres neither.
     *
     * What the eye reads as the middle is the middle of the capitals, so the
     * line moves up by half the descender. On a button reading "Fertig" beside
     * one reading "Zurück" the difference is plain, and it is the reason this
     * exists.
     *
     * @param type_size The size the text is set in.
     * @returns How many points to raise it by.
     */
    constexpr Point optical_offset(Point type_size) const
    {
        return Point{rounded(type_size.value * token::descender_ratio / 2.0f)};
    }

    /**
     * A corner inside another, in pixels.
     *
     * The rule itself lives with the values in `tokens.hpp`; this is the same
     * rule with the panel applied, so a caller drawing a nested surface never
     * handles the two corners as separate numbers.
     *
     * @param outer The corner around it.
     * @param gap The space between the two, the standard inset by default.
     * @returns The inner corner in whole pixels.
     */
    constexpr Point inner_radius(Millimeter outer, Millimeter gap = token::inset) const
    {
        return (*this)(token::inner_radius(outer, gap));
    }
};

/// The panel this project measures, and the one every figure in its papers
/// describes.
inline constexpr Panel jc8048w500{.width = 800_pt, .height = 480_pt,
                                  .points_per_mm = 7.35f};

// The figure a fingertip comes out at on this panel, which the whole interface
// is built on. This is not an example: when it stops holding, the library does
// not build.
static_assert(jc8048w500(token::fingertip) == 66_pt);
static_assert(jc8048w500(token::thumb) == 88_pt);

// The four grades of the type scale, which the drawings of the interface use to
// the point.
static_assert(jc8048w500.type_size(token::title) == 60_pt);
static_assert(jc8048w500.type_size(token::heading) == 36_pt);
static_assert(jc8048w500.type_size(token::body) == 26_pt);
static_assert(jc8048w500.type_size(token::small) == 22_pt);

// The spacings, in the pixels the drawings are placed on.
static_assert(jc8048w500(token::edge) == 16_pt);
static_assert(jc8048w500(token::inset) == 20_pt);
static_assert(jc8048w500(token::line_gap) == 12_pt);

// The corners, outer and nested.
static_assert(jc8048w500(token::radius_panel) == 32_pt);
static_assert(jc8048w500(token::radius_tile) == 20_pt);
static_assert(jc8048w500.inner_radius(token::radius_panel) == 12_pt);

}  // namespace cal
