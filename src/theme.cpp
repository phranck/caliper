#include "caliper/theme.hpp"

#include "lvgl.h"

#include "caliper/tokens.hpp"

namespace cal {
namespace {

/// The styles the theme applies. They live for as long as the program does,
/// because a style handed to LVGL is read at every draw and freeing it would
/// leave the objects pointing at nothing.
lv_style_t style_screen;
lv_style_t style_surface;
lv_style_t style_text;
lv_style_t style_button;
bool prepared = false;
Typography fonts;

/// Turns a colour from the generated values into what the library expects.
lv_color_t colour(std::uint32_t value)
{
    return lv_color_hex(value);
}

/**
 * Fills in the styles once, for the panel in question.
 */
void prepare(const Panel &panel)
{
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, colour(token::bg));
    lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
    lv_style_set_text_color(&style_screen, colour(token::ink));
    if (fonts.body != nullptr) {
        lv_style_set_text_font(&style_screen, fonts.body);
    }
    lv_style_set_pad_all(&style_screen, 0);
    lv_style_set_border_width(&style_screen, 0);

    lv_style_init(&style_surface);
    lv_style_set_bg_color(&style_surface, colour(token::surface));
    lv_style_set_bg_opa(&style_surface, LV_OPA_COVER);
    lv_style_set_border_width(&style_surface, 0);
    lv_style_set_radius(&style_surface, panel(token::radius_tile).value);
    lv_style_set_pad_all(&style_surface, panel(token::inset).value);

    lv_style_init(&style_text);
    lv_style_set_text_color(&style_text, colour(token::ink));
    if (fonts.body != nullptr) {
        lv_style_set_text_font(&style_text, fonts.body);
    }

    lv_style_init(&style_button);
    lv_style_set_bg_color(&style_button, colour(token::raised));
    lv_style_set_bg_opa(&style_button, LV_OPA_COVER);
    lv_style_set_text_color(&style_button, colour(token::ink));
    lv_style_set_border_width(&style_button, 0);
    lv_style_set_radius(&style_button, panel(token::radius_tile).value);
    lv_style_set_pad_hor(&style_button, panel(token::inset).value);

    // Never under the floor a control has to reach, whatever the label. The
    // check would catch a button that came out smaller, and setting it here
    // means it cannot. It is a minimum and not the height: how tall a button is
    // drawn is the component's business, and it sets that itself.
    lv_style_set_min_height(&style_button, panel(token::minimum).value);
    lv_style_set_min_width(&style_button, panel(token::minimum).value);

    prepared = true;
}

/**
 * Applies the styles to whatever the library is about to show.
 */
void apply(lv_theme_t *theme, lv_obj_t *object)
{
    (void)theme;

    if (lv_obj_get_parent(object) == nullptr) {
        lv_obj_add_style(object, &style_screen, 0);
        return;
    }

    if (lv_obj_check_type(object, &lv_button_class)) {
        lv_obj_add_style(object, &style_button, 0);
        return;
    }

    if (lv_obj_check_type(object, &lv_label_class)) {
        lv_obj_add_style(object, &style_text, 0);
        return;
    }

    if (lv_obj_check_type(object, &lv_obj_class)) {
        lv_obj_add_style(object, &style_surface, 0);
    }
}

}  // namespace

void install_theme(_lv_display_t *display, const Panel &panel, const Typography &typefaces)
{
    fonts = typefaces;

    if (!prepared) {
        prepare(panel);
    }

    // The theme sits on top of whatever the library already had, so a widget
    // nobody wrapped is styled too rather than standing beside the design.
    // The structure itself is private to the library, so it is reached through
    // its own calls rather than assigned to.
    lv_theme_t *theme = lv_display_get_theme(display);
    if (theme != nullptr) {
        lv_theme_set_apply_cb(theme, apply);
    }
}

const Typography &typography()
{
    return fonts;
}

}  // namespace cal
