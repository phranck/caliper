#ifndef CALIPER_PAGING_H_
#define CALIPER_PAGING_H_

namespace cal {

/**
 * Which rows of a list a page carries.
 *
 * Apart from the screen that asks for it, and apart from the graphics library,
 * because once the number of rows that fit is known this is arithmetic on three
 * integers and nothing else. That is what lets it be checked at all: a screen
 * needs a display and a display needs a board or a window, whilst a boundary
 * needs neither.
 */

struct Page {
   /// The first row this page shows.
   int first;

   /// One past the last row this page shows. Equal to the list's own total
   /// on the last page, and short of it on every other one.
   int last;
};

/**
 * Which rows of a list belong on one page.
 *
 * The counting, apart from the screen that asks for it. Once the number of
 * rows that fit is known, this is arithmetic on three integers and nothing
 * else, which is what lets it be checked without a graphics library. Every
 * boundary that can be wrong lives here.
 *
 * A page gives one of its rows up to the one that turns the page, but only
 * where turning it is needed at all: a list that already fits keeps every row.
 *
 * @param visible How many rows fit on the screen at once, at least one.
 * @param shown Which page, counted from nought. A number past the last page
 *              gives the last page rather than a range that reads backwards,
 *              because a caller holding a stale one after a list shrank has
 *              no better answer and should not be handed nonsense.
 * @param total How many rows there are in all.
 * @returns The first row and one past the last.
 */
constexpr Page PageOf(int visible, int shown, int total) {
   if (visible < 1 || total < 1) {
      return Page{0, 0};
   }

   const int per_page = total > visible ? visible - 1 : visible;
   if (per_page < 1) {
      return Page{0, total};
   }

   const int last_page = (total - 1) / per_page;
   const int page = shown < 0 ? 0 : (shown > last_page ? last_page : shown);

   const int first = page * per_page;
   const int last = first + per_page < total ? first + per_page : total;
   return Page{first, last};
}

}  // namespace cal

#endif  // CALIPER_PAGING_H_
