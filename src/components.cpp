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

/**
 * Aligns a label so that its capitals sit in the middle rather than its box.
 *
 * A text box runs from the top of the ascenders to the bottom of the
 * descenders, so centring it leaves the letters sitting low. Every vertical
 * centring of text in this library goes through here, because getting it right
 * in one place and wrong in four is worse than not doing it at all.
 *
 * @param label The label to align.
 * @param panel The panel, for the arithmetic.
 * @param alignment Where in its parent the label goes, as a middle alignment.
 * @param size The type size the label is set in.
 * @param inset How far in from the edge, for a left or right alignment.
 */
void align_optically(Object label, const Panel &panel, lv_align_t alignment,
                     Point size, std::int32_t inset = 0)
{
    lv_obj_align(label, alignment, inset, -panel.optical_offset(size).value);
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

Object Screen::status_bar(const StatusBar &status)
{
    Object band = lv_obj_create(root_);
    make_plain(band);
    lv_obj_set_size(band, panel_.width.value, panel_(token::band_status_bar).value);
    lv_obj_set_pos(band, 0, 0);
    lv_obj_set_style_bg_color(band, lv_color_hex(token::surface), 0);
    lv_obj_set_style_bg_opa(band, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(band, panel_(token::status_edge).value, 0);
    status_ = band;

    // Everything sits at the right end, in the order a person reads it: what
    // the device is doing, then how full it is, then the time. Laid out from
    // the right rather than placed, so leaving one out closes the gap.
    lv_obj_set_flex_flow(band, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(band, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(band, panel_(token::status_edge).value, 0);

    const Point size = panel_.type_size(token::status);

    const auto symbol = [&](const lv_image_dsc_t *source) {
        Object image = lv_image_create(band);
        lv_image_set_src(image, source);
        lv_obj_set_style_image_recolor(image, lv_color_hex(token::muted), 0);
        lv_obj_set_style_image_recolor_opa(image, LV_OPA_COVER, 0);

        // Its own size, stated. Left to the layout an image is stretched or
        // squeezed to whatever the row has left, and a symbol that is a point
        // narrower than it is tall reads as a mistake without looking like one.
        lv_obj_set_size(image, source->header.w, source->header.h);
    };

    // Everything in the bar is one weight. What separates the time from the
    // rest is its colour, not a second cut: at 19 points a heavier face reads
    // as a different typeface rather than as emphasis.
    const auto text = [&](const char *content, std::uint32_t colour) {
        Object label = lv_label_create(band);
        lv_label_set_text(label, content);
        lv_obj_set_style_text_color(label, lv_color_hex(colour), 0);

        const lv_font_t *face = typography().status != nullptr ? typography().status
                                                               : typography().small;
        if (face != nullptr) {
            lv_obj_set_style_text_font(label, face, 0);
        }

        // Nudged like every other centred line. In a flex row the alignment is
        // the container's, so the offset goes on the object itself.
        lv_obj_set_style_translate_y(label, -panel_.optical_offset(size).value, 0);
    };

    if (status.leading != nullptr) {
        text(status.leading, token::muted);

        // A spacer that grows, so what follows sits at the right end whatever
        // stands at the left.
        spacer(band);
    }

    if (status.network != nullptr) {
        symbol(status.network);
    }

    if (status.battery != nullptr) {
        symbol(status.battery);
    }

    if (status.charge >= 0) {
        static char charge[8];
        lv_snprintf(charge, sizeof(charge), "%d %%", status.charge);
        text(charge, token::muted);
    }

    if (status.clock != nullptr) {
        text(status.clock, token::ink);
    }

    return band;
}

Object Screen::header(const char *title, const char *trailing)
{
    Object band = lv_obj_create(root_);
    make_plain(band);
    lv_obj_set_size(band, panel_.width.value, panel_(token::band_header).value);
    lv_obj_set_pos(band, 0, panel_(token::band_status_bar).value);
    lv_obj_set_style_pad_hor(band, panel_(token::edge).value, 0);
    header_ = band;

    Object label = lv_label_create(band);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_color(label, lv_color_hex(token::ink), 0);
    if (typography().heading != nullptr) {
        lv_obj_set_style_text_font(label, typography().heading, 0);
    }
    align_optically(label, panel_, LV_ALIGN_LEFT_MID, panel_.type_size(token::heading));

    if (trailing != nullptr) {
        Object right = lv_label_create(band);
        lv_label_set_text(right, trailing);
        lv_obj_set_style_text_color(right, lv_color_hex(token::muted), 0);
        if (typography().small != nullptr) {
            lv_obj_set_style_text_font(right, typography().small, 0);
        }
        align_optically(right, panel_, LV_ALIGN_RIGHT_MID, panel_.type_size(token::small));
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
    footer_ = band;
    return band;
}

bool Screen::is_a_band(Object object) const
{
    return object == status_ || object == header_ || object == footer_;
}

bool Screen::in_a_band(Object object) const
{
    for (Object walk = object; walk != nullptr; walk = lv_obj_get_parent(walk)) {
        if (walk == status_ || walk == header_ || walk == footer_) {
            return true;
        }
    }
    return false;
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

Object list(Object parent, const Panel &panel)
{
    Object group = lv_obj_create(parent);
    make_plain(group);
    lv_obj_set_width(group, lv_pct(100));
    lv_obj_set_height(group, LV_SIZE_CONTENT);

    // One surface for the whole group, with the corner around all of it.
    lv_obj_set_style_bg_color(group, lv_color_hex(token::surface), 0);
    lv_obj_set_style_bg_opa(group, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(group, panel(token::radius_tile).value, 0);
    lv_obj_set_style_clip_corner(group, true, 0);

    // The rows sit directly on each other. What parts them is a line, not a gap.
    lv_obj_set_flex_flow(group, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(group, 0, 0);
    return group;
}

Object row(Object parent, const Panel &panel, const char *name, const char *value,
           const lv_image_dsc_t *icon)
{
    // Inside a group the row carries no surface of its own: the group draws it
    // once for all of them, and a second one on top would show at the corners.
    const bool grouped = lv_obj_get_style_bg_opa(parent, LV_PART_MAIN) != LV_OPA_TRANSP;

    Object line = lv_obj_create(parent);
    make_plain(line);
    lv_obj_set_width(line, lv_pct(100));

    // A row is as tall as the header, which is what the design gives both.
    lv_obj_set_height(line, panel(token::band_header).value);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_hor(line, panel(token::inset).value, 0);

    if (!grouped) {
        lv_obj_set_style_bg_color(line, lv_color_hex(token::surface), 0);
        lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(line, panel(token::radius_tile).value, 0);
    } else if (lv_obj_get_child_count(parent) > 1) {
        // A hairline above every row but the first, starting where the text
        // starts. Drawn as a border rather than as an object, so a list of
        // thirty rows does not cost thirty more of them.
        lv_obj_set_style_border_color(line, lv_color_hex(token::line), 0);
        lv_obj_set_style_border_width(line, 1, 0);
        lv_obj_set_style_border_side(line, LV_BORDER_SIDE_TOP, 0);
        lv_obj_set_style_border_post(line, true, 0);
    }

    std::int32_t text_left = 0;

    if (icon != nullptr) {
        Object symbol = lv_image_create(line);
        lv_image_set_src(symbol, icon);

        // Tinted rather than coloured in the file. The image carries an alpha
        // channel and nothing else, so the same one serves a muted row and an
        // accented one without a second copy.
        lv_obj_set_style_image_recolor(symbol, lv_color_hex(token::muted), 0);
        lv_obj_set_style_image_recolor_opa(symbol, LV_OPA_COVER, 0);
        lv_obj_align(symbol, LV_ALIGN_LEFT_MID, 0, 0);

        // The gap between a symbol and the word it belongs to is the same
        // everywhere on these screens, and it is the design's own inset.
        text_left = icon->header.w + panel(token::inset).value;
    }

    Object label = lv_label_create(line);
    lv_label_set_text(label, name);
    lv_obj_set_style_text_color(label, lv_color_hex(token::ink), 0);
    align_optically(label, panel, LV_ALIGN_LEFT_MID, panel.type_size(token::body), text_left);

    if (value != nullptr) {
        Object right = lv_label_create(line);
        lv_label_set_text(right, value);
        lv_obj_set_style_text_color(right, lv_color_hex(token::muted), 0);
        if (typography().small != nullptr) {
            lv_obj_set_style_text_font(right, typography().small, 0);
        }
        align_optically(right, panel, LV_ALIGN_RIGHT_MID, panel.type_size(token::small));
    }

    return line;
}

namespace {

/**
 * The label carrying a row's name.
 *
 * A row holds up to three children in an order that depends on what it was
 * given, so the name is not always the first. It is always the first label,
 * because the value is added after it.
 *
 * @param row The row.
 * @returns The label, or a null object for a row that carries no name.
 */
Object name_of(Object row)
{
    for (std::uint32_t index = 0; index < lv_obj_get_child_count(row); ++index) {
        Object child = lv_obj_get_child(row, index);
        if (lv_obj_check_type(child, &lv_label_class)) {
            return child;
        }
    }
    return nullptr;
}

/**
 * Moves the mark to the row that was touched.
 *
 * The choice lives in the rows themselves rather than in a variable here, so
 * nothing has to be kept in step with the object tree and the checks can read
 * it the same way anything else on the screen is read.
 */
void on_row_touched(lv_event_t *event)
{
    Object touched = static_cast<Object>(lv_event_get_target(event));
    Object group = lv_obj_get_parent(touched);

    for (std::uint32_t index = 0; index < lv_obj_get_child_count(group); ++index) {
        Object row = lv_obj_get_child(group, index);
        mark_current(row, row == touched);
    }

    lv_obj_send_event(group, LV_EVENT_VALUE_CHANGED, nullptr);
}

}  // namespace

void mark_current(Object row, bool current)
{
    if (current) {
        lv_obj_add_state(row, LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(row, lv_color_hex(token::raised), 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    } else {
        lv_obj_remove_state(row, LV_STATE_CHECKED);

        // Back to transparent rather than to the surface colour: inside a group
        // the surface belongs to the group, and painting it again here would
        // show at the corners the group clips.
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    }

    Object name = name_of(row);
    if (name != nullptr) {
        lv_obj_set_style_text_color(name, lv_color_hex(current ? token::accent : token::ink), 0);
    }
}

void choose_one(Object group, int chosen_row)
{
    for (std::uint32_t index = 0; index < lv_obj_get_child_count(group); ++index) {
        Object row = lv_obj_get_child(group, index);

        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, on_row_touched, LV_EVENT_CLICKED, nullptr);

        // What the finger gets back before anything else happens. It is the
        // same ground the mark uses, at half strength, so pressing a row looks
        // like the beginning of choosing it rather than like a separate colour.
        lv_obj_set_style_bg_color(row, lv_color_hex(token::raised), LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(row, LV_OPA_50, LV_STATE_PRESSED);

        mark_current(row, static_cast<int>(index) == chosen_row);
    }
}

int chosen(Object group)
{
    for (std::uint32_t index = 0; index < lv_obj_get_child_count(group); ++index) {
        if (lv_obj_has_state(lv_obj_get_child(group, index), LV_STATE_CHECKED)) {
            return static_cast<int>(index);
        }
    }
    return -1;
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
    lv_obj_set_style_radius(control, panel(token::radius_button).value, 0);
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
    if (typography().strong != nullptr) {
        lv_obj_set_style_text_font(text, typography().strong, 0);
    }
    lv_obj_set_style_text_color(
        text, lv_color_hex(accent ? token::accent_ink : token::ink), 0);

    align_optically(text, panel, LV_ALIGN_CENTER, panel.type_size(token::body));

    return control;
}

Object centred_block(Object parent, const Panel &panel)
{
    Object block = lv_obj_create(parent);
    make_plain(block);
    lv_obj_set_width(block, lv_pct(100));

    // As tall as what goes into it, which is what lets it be centred at all:
    // a block of a stated height would be centred as that height rather than
    // as its contents.
    lv_obj_set_height(block, LV_SIZE_CONTENT);
    lv_obj_center(block);

    lv_obj_set_flex_flow(block, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(block, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(block, panel(token::group).value, 0);
    return block;
}

Object hero(Object parent, const Panel &panel, const lv_image_dsc_t *icon,
            const char *heading, const char *line)
{
    Object block = centred_block(parent, panel);

    if (icon != nullptr) {
        Object symbol = lv_image_create(block);
        lv_image_set_src(symbol, icon);
        lv_obj_set_size(symbol, icon->header.w, icon->header.h);
        lv_obj_set_style_image_recolor(symbol, lv_color_hex(token::accent), 0);
        lv_obj_set_style_image_recolor_opa(symbol, LV_OPA_COVER, 0);
    }

    Object title = lv_label_create(block);
    lv_label_set_text(title, heading);
    lv_obj_set_style_text_color(title, lv_color_hex(token::ink), 0);
    if (typography().heading != nullptr) {
        lv_obj_set_style_text_font(title, typography().heading, 0);
    }

    if (line != nullptr) {
        Object under = lv_label_create(block);
        lv_label_set_text(under, line);
        lv_obj_set_style_text_color(under, lv_color_hex(token::muted), 0);
    }

    return block;
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
