/*
 * Which rows a page carries, checked at every boundary that can be wrong.
 *
 * Every check here is a `static_assert`, so this file failing to compile is the
 * test failing, exactly as in `test_panel.cc`. Paging is arithmetic on three
 * integers, and arithmetic settled at compile time cannot be wrong at run time.
 *
 * There is a second reason for checking it this hard. Nothing in the product
 * pages today, because no list is long enough, so a mistake here would sit
 * unseen until the first real list arrives and then look like a fault in that
 * list rather than in this.
 */

#include <cstdio>
#include <initializer_list>

#include "caliper/paging.h"

namespace {

using cal::Page;
using cal::PageOf;

constexpr bool Covers(Page page, int first, int last) { return page.first == first && page.last == last; }

// A list that fits keeps every row of it. Nothing turns a page, so no row is
// given up to the one that would.
static_assert(Covers(PageOf(4, 0, 4), 0, 4));
static_assert(Covers(PageOf(4, 0, 3), 0, 3));
static_assert(Covers(PageOf(4, 0, 1), 0, 1));

// One row more than fits is where paging starts, and where a page gives up a
// row to the one that turns it. Four fit, so three are shown and the fourth
// place carries the way onward.
static_assert(Covers(PageOf(4, 0, 5), 0, 3));
static_assert(Covers(PageOf(4, 1, 5), 3, 5));

// Two full pages. The last reaches the total rather than stopping short of it.
static_assert(Covers(PageOf(4, 0, 6), 0, 3));
static_assert(Covers(PageOf(4, 1, 6), 3, 6));

// A middle page stops short of the total on both sides.
static_assert(Covers(PageOf(4, 1, 10), 3, 6));
static_assert(Covers(PageOf(4, 3, 10), 9, 10));

// A page past the last gives the last, because a caller holding a stale number
// after a list shrank has no better answer and should not be handed a range
// that reads backwards.
static_assert(Covers(PageOf(4, 9, 5), 3, 5));
static_assert(Covers(PageOf(4, -1, 5), 0, 3));

// Nothing to show, and a screen with no room for a row. Both are answers a
// caller can act on rather than reasons to crash.
static_assert(Covers(PageOf(4, 0, 0), 0, 0));
static_assert(Covers(PageOf(0, 0, 5), 0, 0));

// A screen with room for exactly one row cannot give that row up, or every
// page would be empty and no page would ever be turned.
static_assert(Covers(PageOf(1, 0, 5), 0, 5));

}  // namespace

int main() {
   for (const int total : {3, 5, 6, 10}) {
      std::printf("four fit, %2d rows:", total);
      for (int page = 0; page < 4; ++page) {
         const Page shows = PageOf(4, page, total);
         std::printf("  [%d,%d)", shows.first, shows.last);
      }
      std::printf("\n");
   }
   return 0;
}
