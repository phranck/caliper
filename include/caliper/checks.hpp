#pragma once

#include "caliper/panel.hpp"
#include "caliper/tokens.hpp"
#include "caliper/units.hpp"

/**
 * The six things about a screen that can be settled mechanically.
 *
 * None of them is checked by a graphics toolkit, and none is visible on a
 * screen at four times the size. Whether a surface can still be hit and a line
 * can still be read shows itself on the device, once the device is on a desk
 * and the mistake is expensive.
 *
 * The checks work on a description of an element rather than on a graphics
 * object, so they run on a machine with no board and no graphics library
 * attached. Filling that description from a real object tree is a separate,
 * thin piece of code that only exists where the library does.
 */
namespace cal {

/// What a finding is about.
enum class Rule {
    /// A touch target under the floor a control has to reach.
    TouchTarget,
    /// Something closer to the edge of the panel than the margin.
    Margin,
    /// Text wider than the surface carrying it.
    Fit,
    /// A corner inside another that is not concentric with it.
    Corner,
    /// An edge between two points of the grid.
    Grid,
    /// Content reaching under a band.
    Band,
};

/**
 * One element of a screen, as much of it as the checks need.
 *
 * Everything is in points, because by this stage the panel has been applied and
 * what is left is what the glass will show.
 */
struct Element {
    /// What to call it in a finding. Points at a literal that outlives the check.
    const char *name = "";

    /// Where it sits and how large it is.
    Point left{0};
    Point top{0};
    Point width{0};
    Point height{0};

    /// Whether a finger is meant to land on it.
    bool touchable = false;

    /// How wide its text came out, measured in the typeface it is set in, or
    /// zero where it carries none.
    Point text_width{0};

    /// The padding between its own edge and that text.
    Point text_padding{0};

    /// Whether the element is the text itself rather than a surface carrying
    /// it. Where a line of text sits inside its surface is a matter of
    /// alignment, so the grid does not apply to it: the grid exists so that two
    /// surfaces side by side are not half a point apart.
    bool is_text = false;

    /// Its corner, and the corner of whatever encloses it. Both zero where
    /// neither has one.
    Point radius{0};
    Point outer_radius{0};

    /// The space between this element's edge and the enclosing one, which is
    /// what the two corners differ by.
    Point outer_gap{0};
};

/**
 * What one check found, with both figures so nobody has to measure again.
 */
struct Finding {
    Rule rule;
    const char *name;

    /// What it is.
    std::int32_t actual;

    /// What it has to be.
    std::int32_t required;
};

/// What a caller does with a finding. Returning is the only option: the checks
/// allocate nothing and throw nothing, so reporting belongs to the caller.
using Report = void (*)(const Finding &);

/**
 * The bands a screen carries, so content can be told apart from what covers it.
 *
 * A band takes its height from what it holds, so these are results rather than
 * settings, and they are passed in rather than assumed.
 */
struct Bands {
    /// The first row of pixels the content may use.
    Point content_top{0};

    /// The first row it may not, which is where the footer begins.
    Point content_bottom{0};
};

/**
 * Runs all six checks over one element and reports what does not hold.
 *
 * @param element The element to check.
 * @param panel The panel it is built for, which decides what a finger needs.
 * @param bands Where the content area begins and ends.
 * @param report Called once per finding.
 * @returns How many findings there were, so a caller can stop a build on it.
 */
constexpr int check(const Element &element, const Panel &panel, const Bands &bands,
                    Report report)
{
    int findings = 0;

    const auto found = [&](Rule rule, std::int32_t actual, std::int32_t required) {
        findings += 1;
        if (report != nullptr) {
            report(Finding{rule, element.name, actual, required});
        }
    };

    const std::int32_t right = element.left.value + element.width.value;
    const std::int32_t bottom = element.top.value + element.height.value;

    // A touch target smaller than a fingertip is one the finger misses, and it
    // looks perfectly reasonable on a screen at four times the size.
    if (element.touchable) {
        const std::int32_t needed = panel(token::minimum).value;
        if (element.width.value < needed) {
            found(Rule::TouchTarget, element.width.value, needed);
        }
        if (element.height.value < needed) {
            found(Rule::TouchTarget, element.height.value, needed);
        }
    }

    // Content against the edge of the panel reads as content against the case.
    const std::int32_t margin = panel(token::edge).value;
    if (element.left.value < margin) {
        found(Rule::Margin, element.left.value, margin);
    }
    if (element.top.value < margin) {
        found(Rule::Margin, element.top.value, margin);
    }
    if (panel.width.value - right < margin) {
        found(Rule::Margin, panel.width.value - right, margin);
    }
    if (panel.height.value - bottom < margin) {
        found(Rule::Margin, panel.height.value - bottom, margin);
    }

    // A word that only overflows in the second language is one nobody sees.
    if (element.text_width.value > 0) {
        const std::int32_t room = element.width.value - 2 * element.text_padding.value;
        if (element.text_width.value > room) {
            found(Rule::Fit, element.text_width.value, room);
        }
    }

    // A corner inside another has to be the outer one less the space between
    // them. Anything else is the pinched corner one sees and cannot name.
    if (element.outer_radius.value > 0) {
        const std::int32_t concentric = element.outer_radius.value - element.outer_gap.value;
        if (element.radius.value != concentric) {
            found(Rule::Corner, element.radius.value, concentric);
        }
    }

    // Half a point is invisible alone and plain to see the moment two surfaces
    // sit side by side. Text is exempt, for the reason at `is_text`.
    const std::int32_t step = token::grid.value;
    if (!element.is_text) {

        // A plain array rather than a braced list, which would pull in a
        // standard header for nothing but the loop.
        const std::int32_t edges[4] = {element.left.value, element.top.value,
                                       element.width.value, element.height.value};
        for (std::int32_t edge : edges) {
            if (edge % step != 0) {
                found(Rule::Grid, edge, edge - edge % step);
            }
        }
    }

    // A row hidden behind the footer is a row that was drawn and paid for.
    if (bands.content_bottom.value > 0) {
        if (element.top.value < bands.content_top.value) {
            found(Rule::Band, element.top.value, bands.content_top.value);
        }
        if (bottom > bands.content_bottom.value) {
            found(Rule::Band, bottom, bands.content_bottom.value);
        }
    }

    return findings;
}

/**
 * The name of a rule, for a message a person reads.
 *
 * @param rule The rule.
 * @returns Its name, which outlives the call.
 */
constexpr const char *name_of(Rule rule)
{
    switch (rule) {
    case Rule::TouchTarget: return "touch target";
    case Rule::Margin: return "margin";
    case Rule::Fit: return "fit";
    case Rule::Corner: return "corner";
    case Rule::Grid: return "grid";
    case Rule::Band: return "band";
    }
    return "unknown";
}

}  // namespace cal
