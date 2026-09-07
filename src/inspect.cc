#include "caliper/inspect.h"

#include <cinttypes>

#include "caliper/tokens.h"
#include "esp_log.h"
#include "lvgl.h"

namespace cal {
namespace {

constexpr char kTag[] = "caliper";

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
Element describe(lv_obj_t* object) {
   Element element;

   lv_area_t area;
   lv_obj_get_coords(object, &area);

   element.left = Point{area.x1};
   element.top = Point{area.y1};
   element.width = Point{lv_area_get_width(&area)};
   element.height = Point{lv_area_get_height(&area)};

   // Clickable is set on almost everything the library creates, so it says
   // nothing about whether a finger is meant to land there. Two things do: what
   // the object is, and whether anything is listening. A button is a button
   // before a handler is attached to it, and a plain container with a handler
   // is a target even though it looks like scenery.
   const bool a_control = lv_obj_check_type(object, &lv_button_class) || lv_obj_check_type(object, &lv_slider_class) ||
                          lv_obj_check_type(object, &lv_switch_class) || lv_obj_check_type(object, &lv_checkbox_class);
   element.touchable =
       a_control || (lv_obj_has_flag(object, LV_OBJ_FLAG_CLICKABLE) && lv_obj_get_event_count(object) > 0);

   element.radius = Point{static_cast<std::int32_t>(lv_obj_get_style_radius(object, LV_PART_MAIN))};

   element.draws = lv_obj_get_style_bg_opa(object, LV_PART_MAIN) != LV_OPA_TRANSP ||
                   lv_obj_get_style_border_width(object, LV_PART_MAIN) > 0;

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

   // Anything else gets the name of what it is, so a finding can be placed at
   // all. An empty name in a log is a finding nobody can act on.
   if (element.name[0] == '\0') {
      if (lv_obj_check_type(object, &lv_button_class)) {
         element.name = "button";
      } else if (lv_obj_get_child_count(object) > 0) {
         element.name = "container";
      } else {
         element.name = "spacer";
      }
   }

   return element;
}

/// Where one walk puts what it found, since the walk cannot carry state through
/// the library's own iteration.
struct Walk {
   Screen* screen;
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
void visit(lv_obj_t* object, const Bands& bands, bool scrolled) {
   const Element element = describe(object);

   // The band check is left out twice over: inside a scrolling container,
   // where content below the fold is the point, and for the bands themselves,
   // which reach outside the content area by definition. Everything else is
   // checked wherever it stands, which is how a control in a footer gets
   // measured at all.
   const bool banded = walk.screen->InABand(object);
   const bool exempt = scrolled || banded;
   const Bands applicable = exempt ? Bands{} : bands;

   // A band spans the panel, so it has no margin of its own to hold. What
   // stands inside one holds the bar's margin, which is smaller than the
   // screen's on purpose. Everything else holds the screen's.
   Element measured = element;
   if (walk.screen->IsABand(object) || object == walk.screen->root()) {
      measured.margin = Point{0};
   } else if (banded) {
      measured.margin = walk.screen->panel()(token::kStatusEdge);
      measured.margin_vertical = false;
   } else {
      measured.margin = walk.screen->panel()(token::kEdge);
   }

   walk.findings += Check(measured, walk.screen->panel(), applicable, walk.report);

   const bool scrolls = scrolled || lv_obj_has_flag(object, LV_OBJ_FLAG_SCROLLABLE);

   const std::uint32_t children = lv_obj_get_child_count(object);
   for (std::uint32_t index = 0; index < children; ++index) {
      visit(lv_obj_get_child(object, index), bands, scrolls);
   }
}

/// Writes a finding the way somebody reading a serial log wants it.
void log_finding(const Finding& finding) {
   ESP_LOGE(kTag, "%s: \"%s\" is %" PRId32 ", needs %" PRId32 " (%" PRId32 ",%" PRId32 " %" PRId32 "x%" PRId32 ")",
            NameOf(finding.rule), finding.name, finding.actual, finding.required, finding.left.value, finding.top.value,
            finding.width.value, finding.height.value);
}

}  // namespace

int Inspect(Screen& screen, Report report) {
   // The layout has to have run, or every coordinate read below is the one
   // from before it did.
   lv_obj_update_layout(lv_screen_active());

   const Bands bands{screen.ContentTop(), screen.ContentBottom()};

   walk = Walk{&screen, report, 0};

   // The whole screen, not only the content. A control in a footer is a
   // control, and leaving the bands out meant an undersized one in there was
   // never looked at.
   // The screen itself is the panel and holds no margin to anything.
   const std::uint32_t children = lv_obj_get_child_count(screen.root());
   for (std::uint32_t index = 0; index < children; ++index) {
      visit(lv_obj_get_child(screen.root(), index), bands, false);
   }

   return walk.findings;
}

int InspectAndLog(Screen& screen) {
   const int findings = Inspect(screen, log_finding);

   if (findings == 0) {
      ESP_LOGI(kTag, "screen holds");
   } else {
      ESP_LOGE(kTag, "%d finding%s on this screen", findings, findings == 1 ? "" : "s");
   }

   return findings;
}

}  // namespace cal
