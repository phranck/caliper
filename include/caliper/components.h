#ifndef CALIPER_COMPONENTS_H_
#define CALIPER_COMPONENTS_H_

#include "caliper/panel.h"
#include "lvgl.h"

namespace cal {

/// An object of the graphics library. A component is one, so this header knows
/// about the library; the core with the units and the checks does not, which is
/// what lets those run on a machine with no board.
using Object = lv_obj_t*;

/**
 * What the status bar carries. Everything in it is optional, and what is left
 * out simply does not appear.
 */
struct StatusBar {
   /// What stands at the left end. The right end is for the state of the
   /// device, so this is where anything about the screen itself goes.
   const char* leading = nullptr;

   /// The time, right aligned, which is where a person looks for it.
   const char* clock = nullptr;

   /// The state of the network, as a symbol, or nullptr for none.
   const lv_image_dsc_t* network = nullptr;

   /// The battery, as a symbol.
   const lv_image_dsc_t* battery = nullptr;

   /// How full it is, shown beside the symbol. Negative leaves the figure out
   /// and shows the symbol alone, which is what the design makes switchable.
   int charge = -1;
};

/**
 * A screen, which is the frame the components compute from.
 *
 * It carries the panel and the three bands, and it hands out the area that is
 * left between them. Nothing built on it states a coordinate: what a screen
 * says is what stands on it and in what order.
 */
class Screen {
  public:
   /**
    * Takes over the active screen of the display and prepares it.
    *
    * @param panel The panel being drawn on.
    */
   explicit Screen(const Panel& panel);

   /// The status bar along the very top, on every screen.
   ///
   /// Named like a variable rather than in the style of a function, which the
   /// guide allows for an accessor. `StatusBar` is taken by what it is given,
   /// and the two cannot both have that name. `panel`, `root` and `typography`
   /// keep theirs for the same reason.
   Object status_bar(const StatusBar& status = StatusBar{});

   /**
    * The header under it, which says what one is looking at.
    *
    * @param title What is being looked at.
    * @param trailing What stands on the right, or nullptr for nothing.
    */
   Object Header(const char* title, const char* trailing = nullptr);

   /// The footer along the bottom, for whatever the screen offers there.
   Object Footer();

   /// The area between the bands, which is what everything else goes into.
   Object Content();

   /// The panel this screen is built for.
   const Panel& panel() const { return panel_; }

   /// The first row of pixels the content may use, and the first it may not.
   Point ContentTop() const;
   Point ContentBottom() const;

   /**
    * How many rows of a given height are visible at once.
    *
    * A list scrolls, so this is not a limit on what may be put into it. It
    * says what somebody sees without moving anything, which is what decides
    * whether the important row is one of them.
    *
    * @param row_height How tall one row is.
    * @returns How many are visible, gaps between them included.
    */
   int RowsVisible(Point row_height) const;

   /// The height a row of a list takes, which is the height of the header.
   Point RowHeight() const;

   /// The screen everything on it hangs from, for a pass that walks it.
   Object root() const { return root_; }

   /**
    * Whether an object is one of the bands or sits in one.
    *
    * A band reaches outside the content area by definition, so the check that
    * keeps content clear of the bands cannot be applied to the bands
    * themselves. Every other check still is.
    *
    * @param object The object to ask about.
    * @returns True when the band check does not apply to it.
    */
   bool InABand(Object object) const;

   /// Whether an object is one of the bands itself, which spans the panel and
   /// therefore holds no margin.
   bool IsABand(Object object) const;

  private:
   Panel panel_;
   Object root_ = nullptr;
   Object content_ = nullptr;
   Object status_ = nullptr;
   Object header_ = nullptr;
   Object footer_ = nullptr;
};

/**
 * A group of rows, drawn as one surface.
 *
 * The rows inside it share a single background with one corner around the whole
 * group, and a hairline parts each row from the next. The line starts where the
 * text starts rather than at the edge of the surface, which is what makes a
 * list read as a column of entries instead of a stack of separate things.
 *
 * A screen may carry several groups, and the space between two of them is what
 * says they are separate.
 *
 * @param parent What it goes into, normally the content area.
 * @param panel The panel, for the measurements.
 * @returns The group, to put rows into.
 */
Object List(Object parent, const Panel& panel);

/**
 * A row of a list: a name on the left, a value on the right.
 *
 * It takes the full width of whatever it is put into and the height of a row
 * from the design, which is the same height the header takes. Put into a
 * `list`, it carries no surface of its own and is parted from the next by a
 * hairline; put anywhere else, it draws its own.
 *
 * @param parent What it goes into.
 * @param panel The panel, for the measurements.
 * @param name What the row is about.
 * @param value What it says, or nullptr for nothing.
 * @param icon A symbol at the left end, or nullptr for none. It carries an
 *             alpha channel only and is tinted here, so one file serves every
 *             colour it is ever drawn in.
 * @returns The row, so a caller can attach an event to it.
 */
Object Row(Object parent, const Panel& panel, const char* name, const char* value = nullptr,
           const lv_image_dsc_t* icon = nullptr);

/**
 * Marks a row as the one that is chosen, or takes the mark away again.
 *
 * A marked row raises its ground and sets its name in the accent colour, which
 * is the same pair everything on these screens uses to say "this one". It is
 * safe to call on a row that is already in the state asked for.
 *
 * @param row The row.
 * @param current Whether it is the chosen one.
 */
void MarkCurrent(Object row, bool current);

/**
 * Turns a group of rows into a list where touching one chooses it.
 *
 * Exactly one row is marked at any time, and touching another moves the mark.
 * The group then sends `LV_EVENT_VALUE_CHANGED`, so a caller attaches an
 * ordinary event handler to the group and asks `chosen` which row it is. That
 * keeps the choice in the object tree rather than in a variable beside it,
 * which is what lets the checks see it.
 *
 * Every row also gains a pressed appearance. Without one a finger gets no
 * answer until the screen changes, and on a panel that redraws in tens of
 * milliseconds that reads as a device that did not notice.
 *
 * @param group The group, with its rows already in it.
 * @param chosen Which row starts marked, counted from nought, or -1 for none.
 */
void ChooseOne(Object group, int chosen = 0);

/**
 * Which row of a group is the chosen one.
 *
 * @param group A group that was passed to `choose_one`.
 * @returns The row's place in the group, counted from nought, or -1 for none.
 */
int Chosen(Object group);

/**
 * A card: a surface with its own corner, holding whatever is put into it.
 *
 * @param parent What it goes into.
 * @param panel The panel, for the measurements.
 * @param width How wide, in points, because a card is placed by a layout rather
 *              than filling its parent.
 * @param height How tall.
 * @returns The card.
 */
Object Card(Object parent, const Panel& panel, Point width, Point height);

/**
 * A button, which is never narrower than its label plus its padding and never
 * smaller than a finger needs.
 *
 * A fixed width truncates a longer word the moment the interface carries a
 * second language, which is why the width is a minimum and not a setting.
 *
 * @param parent What it goes into.
 * @param panel The panel, for the measurements.
 * @param label What it says.
 * @param accent Whether it carries the accent colour, which marks the one
 *               action a screen is about.
 * @returns The button.
 */
Object Button(Object parent, const Panel& panel, const char* label, bool accent = false);

/**
 * A stack of things standing in the middle of what is left of a screen.
 *
 * It counts the heights and the gaps together and places the stack as a whole.
 * Set by hand such a stack sits too low almost every time, because the eye
 * counts the lines and forgets the gaps between them.
 *
 * This is what a spacer cannot do. A single object in the middle of a surface
 * is an alignment, but four of them stacked need two nested containers, because
 * a flex layout works in one direction, and that costs six objects for what an
 * alignment does without one.
 *
 * @param parent What it goes into, normally the content area.
 * @param panel The panel, for the measurements.
 * @returns The block, to put things into. They stack downwards.
 */
Object CentredBlock(Object parent, const Panel& panel);

/**
 * What a screen says when it has one thing to say.
 *
 * A symbol, a heading, a line under it and one control: the shape of a welcome,
 * an empty state or a question. Everything is optional except the heading,
 * because without that there is nothing being said.
 *
 * @param parent What it goes into.
 * @param panel The panel, for the measurements.
 * @param icon The symbol above it, or nullptr.
 * @param heading What it says.
 * @param line The line under it, or nullptr.
 * @returns The block, so a caller can put a control at the bottom of it.
 */
Object Hero(Object parent, const Panel& panel, const lv_image_dsc_t* icon, const char* heading,
            const char* line = nullptr);

/**
 * Empty space between two things.
 *
 * With a size it is exactly that large. Without one it grows and pushes
 * whatever stands left and right of it to the ends, and it takes its direction
 * from the container rather than from itself: in a row it grows sideways, in a
 * column downwards.
 *
 * It is not for centring. A single object in the middle of a surface is an
 * alignment, which is a property of the object rather than an object beside it.
 *
 * @param parent What it goes into.
 * @param size How large, or zero to grow.
 * @returns The spacer.
 */
Object Spacer(Object parent, Point size = Point{0});

}  // namespace cal

#endif  // CALIPER_COMPONENTS_H_
