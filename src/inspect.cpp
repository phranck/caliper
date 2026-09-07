#include "caliper/inspect.hpp"

#include <cinttypes>

#include "esp_log.h"
#include "lvgl.h"

#include "caliper/tokens.hpp"

namespace cal {
namespace {

constexpr char TAG[] = "caliper";

/**
 * Reads one object's geometry as the layout actually produced it.
 *
 * The coordinates a layout computes are only there once it has run, so this is
 * called after the screen is laid out rather than while it is being built.
 * Anything read before then is the geometry somebody intended.
 *
 * @param object The object to describe.
 * @returns Its description, in the terms the checks work in.
 */
Element describe(lv_obj_t *object)
{
    Element element;

    lv_area_t area;
    lv_obj_get_coords(object, &area);

    element.left = Point{area.x1};
    element.top = Point{area.y1};
    element.width = Point{lv_area_get_width(&area)};
    element.height = Point{lv_area_get_height(&area)};

    element.touchable = lv_obj_has_flag(object, LV_OBJ_FLAG_CLICKABLE);

    element.radius = Point{static_cast<std::int32_t>(
        lv_obj_get_style_radius(object, LV_PART_MAIN))};

    // A label states its own width once it has been laid out, and that width is
    // what has to fit rather than an estimate from the character count.
    if (lv_obj_check_type(object, &lv_label_class)) {
        element.text_width = element.width;
        element.name = lv_label_get_text(object);
        element.is_content = true;
    }

    if (lv_obj_check_type(object, &lv_image_class)) {
        element.name = "symbol";
        element.is_content = true;
    }

    return element;
}

/// Where one walk puts what it found, since the walk cannot carry state through
/// the library's own iteration.
struct Walk {
    Screen *screen;
    Report report;
    int findings;
};

Walk walk;

/**
 * Checks one object and everything under it.
 *
 * @param object The object.
 * @param bands Where the content area begins and ends.
 * @param scrolled Whether an enclosing container scrolls. Inside one, content
 *                 below the fold is the point rather than a fault, so the band
 *                 check is left out for it and the other five still run.
 */
void visit(lv_obj_t *object, const Bands &bands, bool scrolled)
{
    const Element element = describe(object);
    const Bands applicable = scrolled ? Bands{} : bands;
    walk.findings += check(element, walk.screen->panel(), applicable, walk.report);

    const bool scrolls = scrolled ||
                         lv_obj_has_flag(object, LV_OBJ_FLAG_SCROLLABLE);

    const std::uint32_t children = lv_obj_get_child_count(object);
    for (std::uint32_t index = 0; index < children; ++index) {
        visit(lv_obj_get_child(object, index), bands, scrolls);
    }
}

/// Writes a finding the way somebody reading a serial log wants it.
void log_finding(const Finding &finding)
{
    ESP_LOGE(TAG, "%s: \"%s\" is %" PRId32 ", needs %" PRId32, name_of(finding.rule),
             finding.name, finding.actual, finding.required);
}

}  // namespace

int inspect(Screen &screen, Report report)
{
    // The layout has to have run, or every coordinate read below is the one
    // from before it did.
    lv_obj_update_layout(lv_screen_active());

    const Bands bands{screen.content_top(), screen.content_bottom()};

    walk = Walk{&screen, report, 0};

    // The bands themselves reach outside the content area by definition, so the
    // walk starts at the content and not at the screen. What sits in a band is
    // checked against the margins and the grid all the same, one level down.
    visit(screen.content(), bands, false);

    return walk.findings;
}

int inspect_and_log(Screen &screen)
{
    const int findings = inspect(screen, log_finding);

    if (findings == 0) {
        ESP_LOGI(TAG, "screen holds");
    } else {
        ESP_LOGE(TAG, "%d finding%s on this screen", findings,
                 findings == 1 ? "" : "s");
    }

    return findings;
}

}  // namespace cal
