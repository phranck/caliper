#pragma once

#include "caliper/panel.hpp"

struct _lv_display_t;
struct _lv_font_t;

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
    const _lv_font_t *body = nullptr;

    /// A heading, larger, for what one is looking at.
    const _lv_font_t *heading = nullptr;

    /// The smallest grade, for a value beside a name.
    const _lv_font_t *small = nullptr;
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
void install_theme(_lv_display_t *display, const Panel &panel, const Typography &fonts);

/// The typefaces the theme was installed with, for a component that sets text.
const Typography &typography();

}  // namespace cal
