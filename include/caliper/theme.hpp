#pragma once

#include "lvgl.h"

#include "caliper/panel.hpp"

namespace cal {

/**
 * The typefaces a screen is set in, one per grade of the scale.
 *
 * They belong to the product and not to this library: a library that ships a
 * typeface has decided what the product looks like. Caliper says how large a
 * grade is and the product says in what.
 */
struct Typography {
    /// Ordinary text, and what anything unspecified falls back to.
    const lv_font_t *body = nullptr;

    /// A heading, larger, for what one is looking at.
    const lv_font_t *heading = nullptr;

    /// The smallest grade, for a value beside a name.
    const lv_font_t *small = nullptr;

    /// The same size as body in a heavier cut, for the one or two words a
    /// control carries. A label on a button is read at a glance rather than
    /// line by line, and the heavier cut is what makes that work at this size.
    const lv_font_t *strong = nullptr;

    /// What the status bar is set in, at its own grade. It may be a different
    /// face: the bar carries figures and single words at the very top of the
    /// screen, where a condensed face is tight and a normal one reads at a
    /// glance. Null falls back to `small`.
    const lv_font_t *status = nullptr;

};

/**
 * Registers Caliper's look with the graphics library, for one display.
 *
 * It is a theme rather than a second styling world beside the library's own.
 * Anything taken straight from LVGL is styled by it too, so a widget nobody
 * wrapped still belongs to the same design instead of standing beside it.
 *
 * The values come from the generated header, so the device and the documents
 * that describe it cannot carry different ones.
 *
 * @param display The display to style.
 * @param panel The panel it draws on, which turns the physical values into
 *              points.
 * @param fonts The typefaces to set text in.
 */
void install_theme(lv_display_t *display, const Panel &panel, const Typography &fonts);

/// The typefaces the theme was installed with, for a component that sets text.
const Typography &typography();

}  // namespace cal
