#ifndef CALIPER_PANEL_H_
#define CALIPER_PANEL_H_

#include "caliper/tokens.h"
#include "caliper/units.h"

namespace cal {

/**
 * The slice of a list one page of it shows.
 *
 * A list this size or smaller never produces one that stops short of the
 * total, so a caller that never checks for more pages is not wrong until a
 * list actually grows past what one page holds.
 */
struct Page {
   /// The first row this page shows.
   int first;

   /// One past the last row this page shows. Equal to the list's own total
   /// on the last page, and short of it on every other one.
   int last;

   friend constexpr bool operator==(Page, Page) = default;
};

/**
 * The panel everything is computed for.
 *
 * Two things come out of it and they do not scale alike. How wide a column is
 * follows from the width in pixels. How large a touch target has to be follows
 * from a finger, and a finger is the same size on every panel, so it follows
 * from the density instead.
 *
 * Converting the second one with a factor taken from the width is wrong by a
 * wide margin. A fingertip is 66 points on the five inch panel this project
 * measures and 60 on a seven inch one, whilst a factor from the width would say
 * 84. That is forty per cent out, on every single key.
 */
struct Panel {
   /// How many pixels across the visible area is.
   Point width;

   /// How many pixels down.
   Point height;

   /// How many pixels fall on one millimetre of glass.
   float points_per_mm;

   /**
    * Converts a physical size into pixels on this panel.
    *
    * Written as a call so that the panel is named at the point of conversion:
    * `panel(9.0_mm)` reads as the question it answers, and there is no way to
    * ask it without saying which panel.
    *
    * @param size The size on the glass.
    * @returns The size in whole pixels.
    */
   constexpr Point operator()(Millimeter size) const { return Point{Rounded(size.value * points_per_mm)}; }

   /**
    * The type size a text needs to stay legible from a given distance.
    *
    * A capital appears to the eye under an angle, and legibility follows that
    * angle rather than the size on the glass. From the distance and the angle
    * comes the cap height in millimetres, from the density its height in
    * pixels, and from the proportions of the typeface the type size.
    *
    * @param distance How far away the text is meant to be read from.
    * @returns The type size in whole pixels.
    */
   constexpr Point TypeSize(Reading distance) const {
      const float cap_height_mm = token::kCapAngleMrad / 1000.0f * distance.millimetres;
      return Point{Rounded(cap_height_mm * points_per_mm / token::kCapRatio)};
   }

   /**
    * The smallest a control carrying text may be, in pixels.
    *
    * A fixed width truncates a longer word the moment the interface carries a
    * second language, so a control sizes to its label plus its padding and
    * never below what a finger needs. The width of the label itself is
    * measured by whoever has the typeface and is passed in, because this file
    * carries no dependency on a graphics library.
    *
    * @param label_width How wide the label came out, measured in the typeface
    *                    it is set in.
    * @param padding The space between the label and each edge of the control.
    * @param floor The smallest the control may be regardless of its label,
    *              which is a fingertip unless the design says otherwise.
    * @returns The width to give the control.
    */
   constexpr Point MinimumWidth(Point label_width, Millimeter padding = token::kInset,
                                Millimeter floor = token::kMinimum) const {
      const std::int32_t around_the_label = label_width.value + 2 * (*this)(padding).value;
      const std::int32_t needed_by_a_finger = (*this)(floor).value;
      return Point{around_the_label > needed_by_a_finger ? around_the_label : needed_by_a_finger};
   }

   /**
    * How far a line of text has to move up to look centred.
    *
    * A text box is taller than the letters in it: it reaches from the top of
    * the ascenders to the bottom of the descenders, and a word without a
    * descender therefore sits above the middle of its own box whilst a word
    * with one sits below. Centring the box centres neither.
    *
    * What the eye reads as the middle is the middle of the capitals, so the
    * line moves up by half the descender. On a button reading "Fertig" beside
    * one reading "Zurück" the difference is plain, and it is the reason this
    * exists.
    *
    * @param type_size The size the text is set in.
    * @returns How many points to raise it by.
    */
   constexpr Point OpticalOffset(Point type_size) const {
      return Point{Rounded(type_size.value * token::kDescenderRatio / 2.0f)};
   }

   /**
    * A corner inside another, in pixels.
    *
    * The rule itself lives with the values in `tokens.h`; this is the same
    * rule with the panel applied, so a caller drawing a nested surface never
    * handles the two corners as separate numbers.
    *
    * @param outer The corner around it.
    * @param gap The space between the two, the standard inset by default.
    * @returns The inner corner in whole pixels.
    */
   constexpr Point InnerRadius(Millimeter outer, Millimeter gap = token::kInset) const {
      return (*this)(token::InnerRadius(outer, gap));
   }

   /**
    * Rounds a measured length up to the grid.
    *
    * A typeface or a word gives back whatever height or width it happens to
    * need, and that figure lands on the grid only by chance. Rounding it up
    * keeps whatever is stacked or placed against it on the grid too, without
    * ever giving the content itself less room than it measured.
    *
    * @param measured The length as the content actually came out, in pixels.
    * @returns The same length, or the next grid step above it.
    */
   constexpr Point RoundedUpToGrid(Point measured) const {
      const std::int32_t remainder = measured.value % token::kGrid.value;
      return remainder == 0 ? measured : Point{measured.value + token::kGrid.value - remainder};
   }

   /**
    * Which rows of a list belong on the page currently showing.
    *
    * A page holds one row fewer than fit whenever there is a next one to
    * turn to, because the row that turns the page stands in the list with
    * the rest rather than floating outside it. The last page carries every
    * row that is left instead, since nothing follows it.
    *
    * A page asked for beyond the last is clamped to the last one rather than
    * left to run `first` past `total`, so a caller holding a stale page
    * number across a list that shrank still lands on real rows.
    *
    * @param visible How many rows fit on one page, from `Screen::RowsVisible`.
    * @param shown Which page is showing, counted from nought.
    * @param total How many rows there are in all.
    * @returns The slice `shown` covers, or the last page's slice where
    *          `shown` reaches past it. `last` reaches `total` on the last
    *          page and stops one short of it everywhere else.
    */
   constexpr Page Paginate(int visible, int shown, int total) const {
      const int per_page = total > visible ? visible - 1 : visible;

      int page = shown < 0 ? 0 : shown;
      if (per_page > 0) {
         const int last_page = (total - 1) / per_page;
         if (page > last_page) {
            page = last_page;
         }
      }

      const int first = page * per_page;
      const int last = first + per_page < total ? first + per_page : total;
      return Page{first, last};
   }
};

/// The panel this project measures, and the one every figure in its papers
/// describes.
inline constexpr Panel kJc8048w500{.width = token::kPanelJc8048w500Width,
                                   .height = token::kPanelJc8048w500Height,
                                   .points_per_mm = token::kPanelJc8048w500PointsPerMm};

// The figure a fingertip comes out at on this panel, which the whole interface
// is built on. This is not an example: when it stops holding, the library does
// not build.
static_assert(kJc8048w500(token::kFingertip) == 66_pt);
static_assert(kJc8048w500(token::kThumb) == 88_pt);

// The four grades of the type scale, which the drawings of the interface use to
// the point.
static_assert(kJc8048w500.TypeSize(token::kTitle) == 60_pt);
static_assert(kJc8048w500.TypeSize(token::kHeading) == 36_pt);
static_assert(kJc8048w500.TypeSize(token::kBody) == 26_pt);
static_assert(kJc8048w500.TypeSize(token::kSmall) == 22_pt);

// The spacings, in the pixels the drawings are placed on.
static_assert(kJc8048w500(token::kEdge) == 16_pt);
static_assert(kJc8048w500(token::kInset) == 20_pt);
static_assert(kJc8048w500(token::kLineGap) == 12_pt);

// The layout heights, each under its own name rather than behind a shared
// prefix that named a mechanism only the first two of these actually are.
static_assert(kJc8048w500(token::kStatusBar) == 32_pt);
static_assert(kJc8048w500(token::kHeader) == 68_pt);
static_assert(kJc8048w500(token::kFooter) == 80_pt);
static_assert(kJc8048w500(token::kButton) == 48_pt);
static_assert(kJc8048w500(token::kCard) == 500_pt);

// The corners, outer and nested.
static_assert(kJc8048w500(token::kRadiusCard) == 28_pt);
static_assert(kJc8048w500.InnerRadius(token::kRadiusCard) == 8_pt);

}  // namespace cal

#endif  // CALIPER_PANEL_H_
