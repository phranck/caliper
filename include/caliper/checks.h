#ifndef CALIPER_CHECKS_H_
#define CALIPER_CHECKS_H_

#include "caliper/panel.h"
#include "caliper/tokens.h"
#include "caliper/units.h"

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
   /// Something that takes a touch and does nothing with it.
   Deaf,
};

/**
 * One element of a screen, as much of it as the checks need.
 *
 * Everything is in points, because by this stage the panel has been applied and
 * what is left is what the glass will show.
 */
struct Element {
   /// What to call it in a finding. Points at a literal that outlives the check.
   const char* name = "";

   /// Where it sits and how large it is.
   Point left{0};
   Point top{0};
   Point width{0};
   Point height{0};

   /// Whether a finger is meant to land on it.
   bool touchable = false;

   /// How large the area a finger can hit is, where that differs from the box
   /// the element is drawn in. Zero means the two are the same, which is the
   /// ordinary case. A slider is the one that differs: its bar is narrow on
   /// purpose, and what may be hit reaches well beyond it.
   Point touch_width{0};
   Point touch_height{0};

   /// How wide its text came out, measured in the typeface it is set in, or
   /// zero where it carries none.
   Point text_width{0};

   /// The padding between its own edge and that text.
   Point text_padding{0};

   /// Whether the element is content inside a surface rather than a surface
   /// itself, so a line of text or a symbol. Where such a thing sits is a
   /// matter of alignment, and alignment lands where it lands: a symbol of 30
   /// points centred in a row of 68 sits at 19 whatever the grid says.
   ///
   /// The grid exists so that two surfaces side by side are not half a point
   /// apart, and it therefore applies to surfaces.
   bool is_content = false;

   /// Its corner, and the corner of whatever encloses it. Both zero where
   /// neither has one.
   Point radius{0};
   Point outer_radius{0};

   /// The space between this element's edge and the enclosing one, which is
   /// what the two corners differ by.
   Point outer_gap{0};

   /// How close to the edge of the panel this may come. It is the screen's
   /// margin for anything on a screen and the bar's own for anything in the
   /// status bar, which is deliberately narrower because the bar is 32 points
   /// tall where the screen is 480. Zero means the element spans the panel by
   /// design, which is what a band does.
   Point margin{0};

   /// Whether the margin applies downwards as well as sideways. Inside a band
   /// it does not: a band is 32 points tall, and a line of text in it cannot
   /// hold 16 above and below. What frames it there is the band.
   bool margin_vertical = true;

   /// Whether the element draws anything at all. One that does not, so a
   /// container holding others or a spacer, cannot be seen to sit half a point
   /// beside its neighbour, and the grid is there to prevent exactly that.
   bool draws = true;

   /// Whether the element can be pressed, holds nothing, and has nothing
   /// listening to it.
   ///
   /// It is a hole in the screen: the library hit-tests front to back and stops
   /// at the first thing that can be pressed, so such an element swallows the
   /// touch meant for whatever lies under it. Nothing about it looks wrong, and
   /// the screen it stands on measures perfectly.
   ///
   /// Holding nothing is what makes it safe to assert. A container with content
   /// in it may be scrolling, which is a thing to do with a press, so it is
   /// left alone.
   bool swallows_touches = false;
};

/**
 * What one check found, with both figures so nobody has to measure again.
 */
struct Finding {
   Rule rule;
   const char* name;

   /// What it is.
   std::int32_t actual;

   /// What it has to be.
   std::int32_t required;

   /// Where it stands and how large it is. A name says what kind of thing it
   /// is and every container is called the same, so without this a finding on
   /// a screen with forty objects on it cannot be traced back to one of them.
   Point left{0};
   Point top{0};
   Point width{0};
   Point height{0};
};

/// What a caller does with a finding. Returning is the only option: the checks
/// allocate nothing and throw nothing, so reporting belongs to the caller.
using Report = void (*)(const Finding&);

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
 * Runs all seven checks over one element and reports what does not hold.
 *
 * @param element The element to check.
 * @param panel The panel it is built for, which decides what a finger needs.
 * @param bands Where the content area begins and ends.
 * @param report Called once per finding.
 * @returns How many findings there were, so a caller can stop a build on it.
 */
constexpr int Check(const Element& element, const Panel& panel, const Bands& bands, Report report) {
   int findings = 0;

   const auto found = [&](Rule rule, std::int32_t actual, std::int32_t required) {
      findings += 1;
      if (report != nullptr) {
         report(
             Finding{rule, element.name, actual, required, element.left, element.top, element.width, element.height});
      }
   };

   const std::int32_t right = element.left.value + element.width.value;
   const std::int32_t bottom = element.top.value + element.height.value;

   // Something that can be pressed and answers nothing. It reads as scenery and
   // behaves as a lid: the press stops there instead of reaching the control
   // underneath, and neither the drawing nor any other measurement shows it.
   // A slider whose own handle could not be grabbed is what this is here for.
   if (element.swallows_touches) {
      found(Rule::Deaf, 1, 0);
   }

   // A touch target smaller than a fingertip is one the finger misses, and it
   // looks perfectly reasonable on a screen at four times the size. What counts
   // is what can be hit rather than what is drawn, and the two differ wherever
   // a control carries an area reaching past its own edges.
   if (element.touchable) {
      const std::int32_t needed = panel(token::kMinimum).value;
      const std::int32_t across = element.touch_width.value > 0 ? element.touch_width.value : element.width.value;
      const std::int32_t down = element.touch_height.value > 0 ? element.touch_height.value : element.height.value;
      if (across < needed) {
         found(Rule::TouchTarget, across, needed);
      }
      if (down < needed) {
         found(Rule::TouchTarget, down, needed);
      }
   }

   // Content against the edge of the panel reads as content against the case.
   const std::int32_t margin = element.margin.value;
   if (margin > 0 && element.left.value < margin) {
      found(Rule::Margin, element.left.value, margin);
   }
   if (margin > 0 && element.margin_vertical && element.top.value < margin) {
      found(Rule::Margin, element.top.value, margin);
   }
   if (margin > 0 && panel.width.value - right < margin) {
      found(Rule::Margin, panel.width.value - right, margin);
   }
   if (margin > 0 && element.margin_vertical && panel.height.value - bottom < margin) {
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
   // sit side by side. Content and anything invisible are exempt, for the
   // reasons at `is_content` and `draws`.
   const std::int32_t step = token::kGrid.value;
   if (!element.is_content && element.draws) {
      // The two edges, and not the size. Where a surface starts is a
      // decision of the design; how wide it comes out is often the layout
      // filling what is left or a label deciding for it, and neither of those
      // can land on a grid. What one sees as a misalignment is two edges
      // beside each other, which is what this measures.
      const std::int32_t edges[2] = {element.left.value, element.top.value};
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
constexpr const char* NameOf(Rule rule) {
   switch (rule) {
      case Rule::TouchTarget:
         return "touch target";
      case Rule::Margin:
         return "margin";
      case Rule::Fit:
         return "fit";
      case Rule::Corner:
         return "corner";
      case Rule::Grid:
         return "grid";
      case Rule::Band:
         return "band";
      case Rule::Deaf:
         return "swallows a touch";
   }
   return "unknown";
}

}  // namespace cal

#endif  // CALIPER_CHECKS_H_
