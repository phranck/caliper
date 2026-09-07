#pragma once

#include "lvgl.h"

#include "caliper/panel.hpp"

namespace cal {

/// An object of the graphics library. A component is one, so this header knows
/// about the library; the core with the units and the checks does not, which is
/// what lets those run on a machine with no board.
using Object = lv_obj_t *;

/**
 * What the status bar carries. Everything in it is optional, and what is left
 * out simply does not appear.
 */
struct StatusBar {
    /// What stands at the left end. The right end is for the state of the
    /// device, so this is where anything about the screen itself goes.
    const char *leading = nullptr;

    /// The time, right aligned, which is where a person looks for it.
    const char *clock = nullptr;

    /// The state of the network, as a symbol, or nullptr for none.
    const lv_image_dsc_t *network = nullptr;

    /// The battery, as a symbol.
    const lv_image_dsc_t *battery = nullptr;

    /// How full it is, shown beside the symbol. Negative leaves the figure out
    /// and shows the symbol alone, which is what the design makes switchable.
    int charge = -1;
};

/**
 * A screen, which is the frame the components compute from.
 *
 * It carries the panel and the three bands, and it hands out the area that is
 * left between them. Nothing built on it states a coordinate: what a screen
 * says is what stands on it and in what order.
 */
class Screen {
public:
    /**
     * Takes over the active screen of the display and prepares it.
     *
     * @param panel The panel being drawn on.
     */
    explicit Screen(const Panel &panel);

    /// The status bar along the very top, on every screen.
    Object status_bar(const StatusBar &status = StatusBar{});

    /**
     * The header under it, which says what one is looking at.
     *
     * @param title What is being looked at.
     * @param trailing What stands on the right, or nullptr for nothing.
     */
    Object header(const char *title, const char *trailing = nullptr);

    /// The footer along the bottom, for whatever the screen offers there.
    Object footer();

    /// The area between the bands, which is what everything else goes into.
    Object content();

    /// The panel this screen is built for.
    const Panel &panel() const { return panel_; }

    /// The first row of pixels the content may use, and the first it may not.
    Point content_top() const;
    Point content_bottom() const;

    /**
     * How many rows of a given height are visible at once.
     *
     * A list scrolls, so this is not a limit on what may be put into it. It
     * says what somebody sees without moving anything, which is what decides
     * whether the important row is one of them.
     *
     * @param row_height How tall one row is.
     * @returns How many are visible, gaps between them included.
     */
    int rows_visible(Point row_height) const;

    /// The height a row of a list takes, which is the height of the header.
    Point row_height() const;

    /// The screen everything on it hangs from, for a pass that walks it.
    Object root() const { return root_; }

    /**
     * Whether an object is one of the bands or sits in one.
     *
     * A band reaches outside the content area by definition, so the check that
     * keeps content clear of the bands cannot be applied to the bands
     * themselves. Every other check still is.
     *
     * @param object The object to ask about.
     * @returns True when the band check does not apply to it.
     */
    bool in_a_band(Object object) const;

    /// Whether an object is one of the bands itself, which spans the panel and
    /// therefore holds no margin.
    bool is_a_band(Object object) const;

private:
    Panel panel_;
    Object root_ = nullptr;
    Object content_ = nullptr;
    Object status_ = nullptr;
    Object header_ = nullptr;
    Object footer_ = nullptr;
};

/**
 * A group of rows, drawn as one surface.
 *
 * The rows inside it share a single background with one corner around the whole
 * group, and a hairline parts each row from the next. The line starts where the
 * text starts rather than at the edge of the surface, which is what makes a
 * list read as a column of entries instead of a stack of separate things.
 *
 * A screen may carry several groups, and the space between two of them is what
 * says they are separate.
 *
 * @param parent What it goes into, normally the content area.
 * @param panel The panel, for the measurements.
 * @returns The group, to put rows into.
 */
Object list(Object parent, const Panel &panel);

/**
 * A row of a list: a name on the left, a value on the right.
 *
 * It takes the full width of whatever it is put into and the height of a row
 * from the design, which is the same height the header takes. Put into a
 * `list`, it carries no surface of its own and is parted from the next by a
 * hairline; put anywhere else, it draws its own.
 *
 * @param parent What it goes into.
 * @param panel The panel, for the measurements.
 * @param name What the row is about.
 * @param value What it says, or nullptr for nothing.
 * @param icon A symbol at the left end, or nullptr for none. It carries an
 *             alpha channel only and is tinted here, so one file serves every
 *             colour it is ever drawn in.
 * @returns The row, so a caller can attach an event to it.
 */
Object row(Object parent, const Panel &panel, const char *name,
           const char *value = nullptr, const lv_image_dsc_t *icon = nullptr);

/**
 * A card: a surface with its own corner, holding whatever is put into it.
 *
 * @param parent What it goes into.
 * @param panel The panel, for the measurements.
 * @param width How wide, in points, because a card is placed by a layout rather
 *              than filling its parent.
 * @param height How tall.
 * @returns The card.
 */
Object card(Object parent, const Panel &panel, Point width, Point height);

/**
 * A button, which is never narrower than its label plus its padding and never
 * smaller than a finger needs.
 *
 * A fixed width truncates a longer word the moment the interface carries a
 * second language, which is why the width is a minimum and not a setting.
 *
 * @param parent What it goes into.
 * @param panel The panel, for the measurements.
 * @param label What it says.
 * @param accent Whether it carries the accent colour, which marks the one
 *               action a screen is about.
 * @returns The button.
 */
Object button(Object parent, const Panel &panel, const char *label,
              bool accent = false);

/**
 * Empty space between two things.
 *
 * With a size it is exactly that large. Without one it grows and pushes
 * whatever stands left and right of it to the ends, and it takes its direction
 * from the container rather than from itself: in a row it grows sideways, in a
 * column downwards.
 *
 * It is not for centring. A single object in the middle of a surface is an
 * alignment, which is a property of the object rather than an object beside it.
 *
 * @param parent What it goes into.
 * @param size How large, or zero to grow.
 * @returns The spacer.
 */
Object spacer(Object parent, Point size = Point{0});

}  // namespace cal
