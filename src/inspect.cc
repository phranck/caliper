#include "caliper/inspect.h"

#include <cinttypes>

#include "caliper/tokens.h"
#include "lvgl.h"

// The findings go wherever the build has a log. On a board that is the serial
// line the device already writes to, and on a desk it is the terminal the
// window was started from. Asked of the compiler rather than switched by a
// build flag, because the answer is simply whether the header is there.
#if __has_include("esp_log.h")
#include "esp_log.h"
#define CAL_REPORT_ERROR(...) ESP_LOGE(kTag, __VA_ARGS__)
#define CAL_REPORT_INFO(...) ESP_LOGI(kTag, __VA_ARGS__)
#else
#include <cstdio>
#define CAL_REPORT_ERROR(...)    \
   do {                          \
      std::printf("%s: ", kTag); \
      std::printf(__VA_ARGS__);  \
      std::printf("\n");         \
      std::fflush(stdout);       \
   } while (false)
#define CAL_REPORT_INFO(...) CAL_REPORT_ERROR(__VA_ARGS__)
#endif

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

   // Pressable, empty, and nothing listening. The library stops at the first
   // thing under the finger that can be pressed, so this one takes the touch
   // meant for whatever lies beneath and drops it.
   //
   // Empty is what makes this safe to assert. The scrollable flag cannot tell a
   // container apart from a decoration, because `lv_obj_create` sets that flag
   // and the clickable one on everything it makes, so a test against it exempts
   // the whole screen. Something holding nothing and with nothing listening has
   // no way of doing anything with a press; a container holding content might
   // be scrolling, and is left alone.
   //
   // A control by type is let through as well, because it answers before
   // anybody attaches a handler to it.
   element.swallows_touches = !a_control && lv_obj_has_flag(object, LV_OBJ_FLAG_CLICKABLE) &&
                              lv_obj_get_event_count(object) == 0 && lv_obj_get_child_count(object) == 0;

   // What a finger can hit is not always what is drawn. A control may carry an
   // area reaching beyond its own edges, and a slider is the case that needs
   // it: its bar is deliberately narrow, because what is dragged is the handle
   // and a bar as tall as a key reads as the control itself. Measured against
   // the drawn box, every such control fails a check that is asking the right
   // question about the wrong rectangle.
   if (element.touchable) {
      lv_area_t reachable;
      lv_obj_get_click_area(object, &reachable);
      element.touch_width = Point{lv_area_get_width(&reachable)};
      element.touch_height = Point{lv_area_get_height(&reachable)};
   }

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
 * @param area Where the content begins and ends.
 * @param scrolled Whether an enclosing container scrolls. Inside one, content
 *                 below the fold is the point rather than a fault, so the content
 *                 check is left out for it and the downward half of the margin
 *                 with it. The other five still run, and so do the sides.
 */
void visit(lv_obj_t* object, const ContentArea& area, bool scrolled) {
   const Element element = describe(object);

   // The content check is left out twice over: inside a scrolling container,
   // where content below the fold is the point, and for the things that stand
   // outside the content by definition, such as the status bar and the player. Everything else is
   // checked wherever it stands, which is how a control in a footer gets
   // measured at all.
   const bool outside = walk.screen->StandsOutsideContent(object);
   const bool exempt = scrolled || outside;
   const ContentArea applicable = exempt ? ContentArea{} : area;

   // Something that stands outside the content spans the panel, so it has no
   // margin of its own to hold. What stands inside one holds the bar's margin,
   // which is smaller than the screen's on purpose. Everything else holds the screen's.
   Element measured = element;
   if (walk.screen->IsOutsideContent(object) || object == walk.screen->root()) {
      measured.margin = Point{0};
   } else if (outside) {
      measured.margin = walk.screen->panel()(token::kStatusEdge);
      measured.margin_vertical = false;
   } else {
      measured.margin = walk.screen->panel()(token::kEdge);

      // Inside something that scrolls, only the sides hold a margin. A row
      // below the fold is not against the case, because it is not on the glass
      // at all, and what holds the distance there is the bottom edge of the
      // scrolling area, which is measured in its own right. Without this, any
      // list longer than the screen reports one finding per row of the part
      // nobody can see.
      measured.margin_vertical = !scrolled;
   }

   walk.findings += Check(measured, walk.screen->panel(), applicable, walk.report);

   const bool scrolls = scrolled || lv_obj_has_flag(object, LV_OBJ_FLAG_SCROLLABLE);

   const std::uint32_t children = lv_obj_get_child_count(object);
   for (std::uint32_t index = 0; index < children; ++index) {
      visit(lv_obj_get_child(object, index), area, scrolls);
   }
}

/// Writes a finding the way somebody reading a serial log wants it.
void log_finding(const Finding& finding) {
   // One rule has nothing to compare. Where a thing swallows a touch there is
   // no figure that should have been larger, only a thing that should not be
   // there, so printing "is 1, needs 0" would be noise dressed as a
   // measurement.
   if (finding.rule == Rule::Deaf) {
      CAL_REPORT_ERROR("%s: \"%s\" (%" PRId32 ",%" PRId32 " %" PRId32 "x%" PRId32 ")", NameOf(finding.rule),
                       finding.name, finding.left.value, finding.top.value, finding.width.value, finding.height.value);
      return;
   }

   CAL_REPORT_ERROR("%s: \"%s\" is %" PRId32 ", needs %" PRId32 " (%" PRId32 ",%" PRId32 " %" PRId32 "x%" PRId32 ")",
                    NameOf(finding.rule), finding.name, finding.actual, finding.required, finding.left.value,
                    finding.top.value, finding.width.value, finding.height.value);
}

}  // namespace

/// What every `InspectAndLog` has found since the tally was cleared. Held here
/// rather than handed back, because the screens are built by functions that
/// return nothing and the figure would be lost on the way out.
int found_so_far = 0;

int FindingsSoFar() { return found_so_far; }

void ClearFindings() { found_so_far = 0; }

int Inspect(Screen& screen, Report report) {
   // The layout has to have run, or every coordinate read below is the one
   // from before it did.
   lv_obj_update_layout(lv_screen_active());

   const ContentArea area{screen.ContentTop(), screen.ContentBottom()};

   walk = Walk{&screen, report, 0};

   // The whole screen, not only the content. A control in a footer is a
   // control, and leaving those out meant an undersized one in there was
   // never looked at.
   // The screen itself is the panel and holds no margin to anything.
   const std::uint32_t children = lv_obj_get_child_count(screen.root());
   for (std::uint32_t index = 0; index < children; ++index) {
      visit(lv_obj_get_child(screen.root(), index), area, false);
   }

   return walk.findings;
}

int InspectAndLog(Screen& screen) {
   const int findings = Inspect(screen, log_finding);
   found_so_far += findings;

   if (findings == 0) {
      CAL_REPORT_INFO("screen holds");
   } else {
      CAL_REPORT_ERROR("%d finding%s on this screen", findings, findings == 1 ? "" : "s");
   }

   return findings;
}

}  // namespace cal
