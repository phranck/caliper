#include "caliper/components.hpp"

#include "lvgl.h"

#include "caliper/theme.hpp"
#include "caliper/tokens.hpp"

namespace cal {
namespace {

/**
 * Strips a container of everything that would otherwise show: no background of
 * its own, no border, no padding, no scrolling.
 *
 * A container is a place to put things, and every one of those defaults draws
 * something the design did not ask for.
 */
void make_plain(Object object)
{
    lv_obj_set_style_bg_opa(object, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(object, 0, 0);
    lv_obj_set_style_pad_all(object, 0, 0);
    lv_obj_set_style_radius(object, 0, 0);
    lv_obj_remove_flag(object, LV_OBJ_FLAG_SCROLLABLE);
}

}  // namespace

Screen::Screen(const Panel &panel) : panel_(panel)
{
    root_ = lv_screen_active();
    make_plain(root_);
    lv_obj_set_style_bg_color(root_, lv_color_hex(token::bg), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
}

Point Screen::content_top() const
{
    return Point{panel_(token::band_status_bar).value + panel_(token::band_header).value};
}

Point Screen::content_bottom() const
{
    return Point{panel_.height.value - panel_(token::band_footer).value};
}

Point Screen::row_height() const
{
    return panel_(token::band_header);
}

int Screen::rows_visible(Point row_height) const
{
    const std::int32_t room = content_bottom().value - content_top().value;
    const std::int32_t gap = panel_(token::line_gap).value;

    if (row_height.value <= 0) {
        return 0;
    }

    // One row needs its own height, every further one needs a gap as well.
    int count = 0;
    std::int32_t used = 0;
    while (used + row_height.value <= room) {
        used += row_height.value + gap;
        count += 1;
    }
    return count;
}

Object Screen::status_bar()
{
    Object band = lv_obj_create(root_);
    make_plain(band);
    lv_obj_set_size(band, panel_.width.value, panel_(token::band_status_bar).value);
    lv_obj_set_pos(band, 0, 0);
    lv_obj_set_style_bg_color(band, lv_color_hex(token::surface), 0);
    lv_obj_set_style_bg_opa(band, LV_OPA_COVER, 0);
    return band;
}

Object Screen::header(const char *title, const char *trailing)
{
    Object band = lv_obj_create(root_);
    make_plain(band);
    lv_obj_set_size(band, panel_.width.value, panel_(token::band_header).value);
    lv_obj_set_pos(band, 0, panel_(token::band_status_bar).value);
    lv_obj_set_style_pad_hor(band, panel_(token::edge).value, 0);

    Object label = lv_label_create(band);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_color(label, lv_color_hex(token::ink), 0);
    if (typography().heading != nullptr) {
        lv_obj_set_style_text_font(label, typography().heading, 0);
    }
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

    if (trailing != nullptr) {
        Object right = lv_label_create(band);
        lv_label_set_text(right, trailing);
        lv_obj_set_style_text_color(right, lv_color_hex(token::muted), 0);
        if (typography().small != nullptr) {
            lv_obj_set_style_text_font(right, typography().small, 0);
        }
        lv_obj_align(right, LV_ALIGN_RIGHT_MID, 0, 0);
    }

    return band;
}

Object Screen::footer()
{
    Object band = lv_obj_create(root_);
    make_plain(band);
    lv_obj_set_size(band, panel_.width.value, panel_(token::band_footer).value);
    lv_obj_set_pos(band, 0, content_bottom().value);
    lv_obj_set_style_bg_color(band, lv_color_hex(token::surface), 0);
    lv_obj_set_style_bg_opa(band, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(band, panel_(token::edge).value, 0);
    return band;
}

Object Screen::content()
{
    if (content_ != nullptr) {
        return content_;
    }

    content_ = lv_obj_create(root_);
    make_plain(content_);
    lv_obj_set_pos(content_, panel_(token::edge).value, content_top().value);
    lv_obj_set_size(content_,
                    panel_.width.value - 2 * panel_(token::edge).value,
                    content_bottom().value - content_top().value);

    // Whatever goes in stacks downwards with the design's own gap between two
    // things that belong together, so nothing states a position.
    lv_obj_set_flex_flow(content_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content_, panel_(token::line_gap).value, 0);

    // A list scrolls. It costs frames, because a moving surface is the most
    // expensive thing this panel does, and it is what a person expects from a
    // list all the same. The content area is therefore the one container on a
    // screen that scrolls, and it does so vertically only: sideways movement
    // belongs to changing the screen, not to reading one.
    lv_obj_add_flag(content_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(content_, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content_, LV_SCROLLBAR_MODE_AUTO);
    return content_;
}

Object row(Object parent, const Panel &panel, const char *name, const char *value)
{
    Object line = lv_obj_create(parent);
    make_plain(line);
    lv_obj_set_width(line, lv_pct(100));

    // A row is as tall as the header, which is what the design gives both.
    lv_obj_set_height(line, panel(token::band_header).value);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(line, lv_color_hex(token::surface), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(line, panel(token::radius_tile).value, 0);
    lv_obj_set_style_pad_hor(line, panel(token::inset).value, 0);

    Object label = lv_label_create(line);
    lv_label_set_text(label, name);
    lv_obj_set_style_text_color(label, lv_color_hex(token::ink), 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

    if (value != nullptr) {
        Object right = lv_label_create(line);
        lv_label_set_text(right, value);
        lv_obj_set_style_text_color(right, lv_color_hex(token::muted), 0);
        if (typography().small != nullptr) {
            lv_obj_set_style_text_font(right, typography().small, 0);
        }
        lv_obj_align(right, LV_ALIGN_RIGHT_MID, 0, 0);
    }

    return line;
}

Object card(Object parent, const Panel &panel, Point width, Point height)
{
    Object surface = lv_obj_create(parent);
    make_plain(surface);
    lv_obj_set_size(surface, width.value, height.value);
    lv_obj_set_style_bg_color(surface, lv_color_hex(token::surface), 0);
    lv_obj_set_style_bg_opa(surface, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(surface, panel(token::radius_panel).value, 0);
    lv_obj_set_style_pad_all(surface, panel(token::inset).value, 0);
    return surface;
}

Object button(Object parent, const Panel &panel, const char *label, bool accent)
{
    Object control = lv_button_create(parent);
    lv_obj_set_style_radius(control, panel(token::radius_tile).value, 0);
    lv_obj_set_style_pad_hor(control, panel(token::inset).value, 0);

    // A minimum rather than a size, so a longer word in another language makes
    // the button wider instead of being cut off inside it.
    lv_obj_set_width(control, LV_SIZE_CONTENT);
    lv_obj_set_style_min_width(control, panel(token::minimum).value, 0);

    lv_obj_set_height(control, panel(token::band_button).value);

    lv_obj_set_style_bg_color(
        control, lv_color_hex(accent ? token::accent : token::raised), 0);

    Object text = lv_label_create(control);
    lv_label_set_text(text, label);
    lv_obj_set_style_text_color(
        text, lv_color_hex(accent ? token::accent_ink : token::ink), 0);
    lv_obj_center(text);

    return control;
}

Object spacer(Object parent, Point size)
{
    Object gap = lv_obj_create(parent);
    make_plain(gap);

    if (size.value > 0) {
        lv_obj_set_size(gap, size.value, size.value);
        return gap;
    }

    // Growing rather than sized. Which way it grows is the container's business,
    // so both directions are set to grow and the one that has no room stays put.
    lv_obj_set_flex_grow(gap, 1);
    lv_obj_set_size(gap, 0, 0);
    return gap;
}

}  // namespace cal
