#ifndef CALIPER_COMPONENTS_H_
#define CALIPER_COMPONENTS_H_

#include <initializer_list>

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
 * What the sidebar carries.
 *
 * The lists are held rather than copied, so this is written where it is passed
 * and never kept. That is what it is for: it names the parts of the sidebar at
 * the one place the sidebar is built.
 */
struct Sidebar {
   /// One symbol per area, in the order they stand.
   std::initializer_list<const lv_image_dsc_t*> icons;

   /// One name per area, under its symbol, in the same order. An empty list
   /// leaves the symbols standing alone.
   std::initializer_list<const char*> names = {};

   /// Which area is open, counted from nought.
   int active = 0;

   /// The wordmark in the header at the top, or nullptr for none. It belongs to
   /// the device rather than to what the device is showing, and the sidebar is
   /// the part of the screen that is the device.
   const lv_image_dsc_t* mark = nullptr;

   /// What the mark is filled with, or nullptr for the surface's own ink. Where
   /// it is given, the mark is taken as a stencil and this is shown through it,
   /// which is how the same spectrum reaches the mark here that it has on the
   /// splash.
   const lv_grad_dsc_t* mark_fill = nullptr;
};

/**
 * What the player controller carries.
 *
 * The keys are named rather than counted out of a list, because which symbol
 * belongs where is a fact about this component and a position in a list says it
 * less clearly than the name does.
 */
struct PlayerController {
   /// The artwork of what is playing, or nullptr for none. It is what a person
   /// recognises first, so it stands at the leading edge and the words follow.
   const lv_image_dsc_t* cover = nullptr;

   /// What is playing.
   const char* title = nullptr;

   /// Where it is playing.
   const char* second = nullptr;

   /// Whether it is running, which decides which of the two middle symbols the
   /// middle key carries.
   bool playing = false;

   const lv_image_dsc_t* back = nullptr;
   const lv_image_dsc_t* pause = nullptr;
   const lv_image_dsc_t* play = nullptr;
   const lv_image_dsc_t* forward = nullptr;
   const lv_image_dsc_t* volume = nullptr;
};

/**
 * What the keyboard carries.
 *
 * The symbols are passed in rather than held here, because the collection they
 * come from belongs to the product and not to the library.
 */
struct Keyboard {
   /// The field it types into. It has to be there: a keyboard with nowhere to
   /// put what is typed is a row of decorations.
   Object field = nullptr;

   /// What the key at the end says, or nullptr for none. It is a button rather
   /// than a key, because it ends the task instead of adding a character.
   const char* confirm = nullptr;

   /// Between small and capital letters, on the letters.
   const lv_image_dsc_t* shift = nullptr;

   /// One layer back, which is what the same key does on the other two.
   const lv_image_dsc_t* back = nullptr;

   /// Rubs out what stands before the caret, and what stands after it.
   const lv_image_dsc_t* backspace = nullptr;
   const lv_image_dsc_t* forward_delete = nullptr;
};

/**
 * What a card says.
 *
 * A message stands over the screen it belongs to rather than in place of it.
 * Where it asks something, the answer is on the screen underneath, and taking
 * that away to ask about it leaves somebody answering from memory.
 */
struct Card {
   /// What kind of message this is. It decides the colour of the symbol beside
   /// the title, and nothing else: a person should know which kind they are
   /// looking at before reading the sentence.
   enum class Kind {
      /// Something worth knowing, in the accent.
      kInfo,
      /// Something went wrong, in the colour of a warning.
      kTrouble,
   };

   /// The title, which says what happened in as few words as it takes.
   const char* title = nullptr;

   /// The message under it, which says the rest.
   const char* message = nullptr;

   /// The symbol beside the title, or nullptr for none. Which symbol belongs to
   /// which kind is the product's business, so it is passed in.
   const lv_image_dsc_t* symbol = nullptr;

   /// Which kind it is.
   Kind kind = Kind::kInfo;
};

/**
 * The way back, at the leading edge of a header.
 *
 * It names the screen it goes to rather than saying "back", because a person
 * two levels down wants to know where the step lands. The symbol before it is
 * what makes it a way back at all: text that leads somewhere and text that
 * reports something look alike, and a way back nobody recognises is worse than
 * none, because they stop looking for one.
 *
 * A screen with nothing above it leaves it empty, and the header then carries
 * only what stands at the other end.
 */
struct WayBack {
   /// What the screen above is called.
   const char* text = nullptr;

   /// The symbol before it, pointing the way it goes. Which symbol that is
   /// belongs to the product.
   const lv_image_dsc_t* symbol = nullptr;
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
   /// What frame a screen stands in.
   enum class Frame {
      /// The bands the finished product carries: a status bar across the top
      /// and room at the bottom for what is playing.
      kProduct,

      /// Nothing but what the screen puts there, and everything on it centred.
      /// The onboarding stands in this: a bar reporting the state of a device
      /// that is still being set up has nothing to report, and a person setting
      /// one up is answering one question at a time rather than watching it.
      kBare,
   };

   /**
    * Takes over the active screen of the display and prepares it.
    *
    * @param panel The panel being drawn on.
    * @param frame Which of the two frames it stands in.
    */
   explicit Screen(const Panel& panel, Frame frame = Frame::kProduct);

   /// The status bar along the very top, on every screen.
   ///
   /// Named like a variable rather than in the style of a function, which the
   /// guide allows for an accessor. `StatusBar` is taken by what it is given,
   /// and the two cannot both have that name. `panel`, `root` and `typography`
   /// keep theirs for the same reason.
   Object status_bar(const StatusBar& status = StatusBar{});

   /**
    * The band under it, which says what one is looking at.
    *
    * The name stands at the trailing end, directly over the content it names.
    * The way out stands at the leading end, where a thumb reaches it and where
    * every screen of a stack puts it, so the way back is in one place however
    * deep one has gone.
    *
    * Both ends stand half a corner in, over and above the band's own edge. A
    * line of type flush against a rounded edge reads as colliding with it, and
    * what stands under this band is a tile whose corner has already pulled it
    * away from the edge.
    *
    * @param title What is being looked at.
    * @param back The way to the screen above. Empty on a screen with nothing
    *             above it.
    * @param note What the screen wants to say about itself, beside the way
    *             back, such as how many favourites there are.
    */
   Object Header(const char* title, const WayBack& back = {}, const char* note = nullptr);

   /// The way back, where the header carries one, so a caller can say where it
   /// goes. This library changes screens for nobody.
   Object back() const { return back_; }

   /**
    * The anchors along the left, one per area of the product.
    *
    * Always there once the device is set up, so that any area is one touch
    * away. On the left rather than along the bottom because this panel is short
    * and wide: the content has 300 points of height and 768 of width, so a band
    * at the bottom takes from what is scarce and one at the side does not.
    *
    * It runs the whole height of the panel and moves both the left edge of the
    * content area and the left end of the status bar. Because no screen states
    * a coordinate, every screen follows without being touched.
    *
    * Named like a variable rather than in the style of a function, for the same
    * reason `status_bar` is: `Sidebar` is taken by what it is given.
    *
    * Each item carries the number of its area as its user data, which is what a
    * caller reads to find out which one was touched.
    *
    * @param areas What it carries.
    * @returns The band, so a caller can attach events to its items.
    */
   Object sidebar(const Sidebar& areas);

   /**
    * A card laid over the screen, with what is behind it dimmed.
    *
    * It is built in three parts, which is how it is spoken about: the title
    * with the symbol for its kind, the message, and the row of buttons at the
    * foot. Its height follows what it holds rather than being stated, because
    * a stated height gives the air inside it away to whatever happens to be
    * put in.
    *
    * It fades in. The screen behind it stays where it is and is dimmed, so
    * whatever the message is about is still there to be looked at.
    *
    * Named like a variable rather than in the style of a function, for the same
    * reason `status_bar` and `sidebar` are: `Card` is taken by what it is
    * given.
    *
    * @param what The message.
    * @returns The card, so a caller may reach it.
    */
   Object card(const Card& what);

   /// The row at the foot of the card, where its buttons go. They are aligned
   /// to the right, so the one that ends the task sits where a step ends.
   Object card_footer() const { return card_footer_; }

   /// Where a card puts what its message alone cannot say, such as a set of
   /// answers to choose between.
   ///
   /// Between the message and the footer, so the buttons stay at the foot
   /// whatever a caller adds. A card is a flex column, and anything put into
   /// the card itself after it was built lands below everything already in it,
   /// which is under the button that closes it.
   Object card_content() const { return card_content_; }

   /// The card itself, where one stands.
   Object card() const { return card_; }

   /// Takes the card away again, and the dimming with it.
   void DismissCard();

   /// What the status bar says the time is, so a caller can change it whilst
   /// the screen stands. The bar is built once with the screen and the time
   /// goes on running: setting it in place rather than building the screen
   /// again matters here more than it would elsewhere, because this panel
   /// redraws the whole frame for any change at all.
   Object clock() const { return clock_; }

   /// The symbol the status bar shows for the network, for the same reason.
   Object signal() const { return signal_; }

   /// The wordmark, wherever the screen has put it, or nullptr where it carries
   /// none. A caller that sets its colours moving needs the object rather than
   /// the band around it.
   Object mark() const { return mark_; }

   /**
    * The transport controls, floating over the bottom of the content.
    *
    * Only there when something is playing anywhere in the system. It has to
    * stay put: one reaches for the pause key whilst music is playing, without
    * looking, and something that scrolls away is no use for that. It is what is
    * left of the footer, which this design does not otherwise have.
    *
    * It is drawn as a pill standing clear of the sidebar, the right edge and
    * the bottom edge by the same distance, so it reads as lying over the screen
    * rather than as another band fixed to it.
    *
    * Named like a variable rather than in the style of a function, for the same
    * reason `status_bar` and `sidebar` are: `PlayerController` is taken by what
    * it is given.
    *
    * @param player What is playing and which keys it offers.
    * @returns The pill, so a caller can reach what stands in it.
    */
   Object player_controller(const PlayerController& player);

   /**
    * The keyboard along the bottom, in three layers.
    *
    * Row two is inset by half a key against row one, and row three carries a
    * modifier at each end. The two modifiers are different widths on purpose,
    * and that difference is what keeps the letters of row three out of row
    * two's columns: on a keyboard no two rows line up, and the eye finds a key
    * by its offset against the row above.
    *
    * The key at the bottom left walks through the layers in a ring, so letters,
    * figures, symbols and back to letters. A ring rather than a pair, because
    * that is how a telephone does it and the hand already knows the way.
    *
    * Built from placed keys rather than from the library's own button matrix.
    * A matrix divides a row into equal parts and pads them alike, and this
    * keyboard has three rows with different gap counts and two modifiers at
    * stated widths, none of which a matrix can hold.
    *
    * A screen carries one keyboard, which is why the layer it is showing is
    * kept here rather than on the object.
    *
    * @param keys What it types into and what its modifiers look like.
    * @returns The band, which sends `LV_EVENT_READY` when the key at the end is
    *          touched.
    */
   Object keyboard(const Keyboard& keys);

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

   /**
    * Says that an object is a piece a band is drawn from rather than something
    * standing in one.
    *
    * A band is sometimes laid in parts: the sidebar's surface is two pieces
    * with a gap for the open item, and the player's pill is two ends and a
    * middle. Each piece reaches the edge of what it belongs to and holds no
    * margin of its own, which is what a band does and what a thing inside one
    * may not. Marked on the object rather than kept in a list, so a band may be
    * laid in as many pieces as its shape needs.
    *
    * @param object The piece.
    */
   static void MarkAsBandPart(Object object);

  private:
   Panel panel_;
   Frame frame_ = Frame::kProduct;
   Object root_ = nullptr;
   Object content_ = nullptr;
   Object sidebar_ = nullptr;
   Object player_ = nullptr;
   Object status_ = nullptr;
   Object mark_ = nullptr;
   Object clock_ = nullptr;
   Object signal_ = nullptr;
   Object card_ = nullptr;
   Object card_footer_ = nullptr;
   Object card_content_ = nullptr;
   Object dimming_ = nullptr;
   Object header_ = nullptr;
   Object back_ = nullptr;
   Object keyboard_ = nullptr;
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
 * @param across How much of the width it takes, as a percentage. A list of two
 *               or three short answers looks lost across the whole measure, and
 *               a narrower one is centred in the place it was given.
 * @returns The group, to put rows into.
 */
Object List(Object parent, const Panel& panel, int across = 100);

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
 * @param marks Symbols at the right end, in the order they are written, left
 *              to right. They say something about the row rather than naming
 *              it: whether it is locked, how strong a signal is, that it leads
 *              somewhere. They stand outside the value, which is text, and the
 *              value gives way to them.
 * @returns The row, so a caller can attach an event to it.
 */
Object Row(Object parent, const Panel& panel, const char* name, const char* value = nullptr,
           const lv_image_dsc_t* icon = nullptr, std::initializer_list<const lv_image_dsc_t*> marks = {});

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
 * Makes a row one that leads away, rather than one that is chosen.
 *
 * It becomes touchable and answers a finger with the same pressed ground a
 * choosable row does, and that is all: nothing stays marked afterwards, because
 * the screen it leads to is the answer. A settings list is the case for it,
 * where every row goes somewhere and none of them is the current one.
 *
 * What happens next is the caller's, so the handler is theirs to attach.
 *
 * @param row The row.
 */
void LeadsAway(Object row);

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
 * A heading over whatever follows it.
 *
 * What the header band carries on a screen of the product, for a screen that
 * has no bands and puts its heading in with the rest.
 *
 * @param parent What it goes into.
 * @param panel The panel, for the measurements.
 * @param words What it says.
 * @returns The heading.
 */
Object Heading(Object parent, const Panel& panel, const char* words);

/**
 * How much a button insists.
 *
 * Three, because a screen has three kinds of answer on it and colour is how a
 * person tells them apart before reading a word. Anything finer belongs in what
 * the button says rather than in how it is filled.
 */
enum class Emphasis {
   /// One answer among several. Most buttons are this.
   kPlain,

   /// The one action the screen is about.
   kAccent,

   /// An action that deletes something. It carries the warning colour rather
   /// than the accent, so that the one answer nobody can take back does not
   /// look like the one they are being led towards.
   kWarning,
};

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
 * @param emphasis How much it insists.
 * @returns The button.
 */
Object Button(Object parent, const Panel& panel, const char* label, Emphasis emphasis = Emphasis::kPlain);

/**
 * A button that carries a symbol and no words.
 *
 * Drawn as a squircle, so its size is one a stencil was made for. A size
 * without one falls back to a fully rounded rectangle, and beside a real
 * squircle that is visible.
 *
 * @param parent What it goes into.
 * @param panel The panel, for the measurements.
 * @param symbol What it carries, centred.
 * @param size How wide and tall, which are the same.
 * @param accent Whether it carries the accent colour.
 * @returns The key, so a caller can attach an event to it.
 */
Object IconButton(Object parent, const Panel& panel, const lv_image_dsc_t* symbol, Point size, bool accent = false);

/**
 * What is playing, as artwork with two lines beside it.
 *
 * @param parent What it goes into.
 * @param panel The panel, for the measurements.
 * @param cover The artwork, or nullptr for none.
 * @param title What is playing.
 * @param second Where it is playing.
 * @returns The group.
 */
/**
 * The field a keyboard types into.
 *
 * A secret field carries a key at its right end that shows what was typed and
 * hides it again. Somebody typing a long word into a panel on a wall has no
 * other way of finding out which character went wrong, and the alternative is
 * clearing the field and starting over.
 *
 * @param parent What it goes into.
 * @param panel The panel, for the measurements.
 * @param secret Whether what is typed is shown as dots. A network's word is,
 *               and a name is not.
 * @param reveal The symbol that shows what is hidden, or nullptr for no key.
 * @param conceal The symbol that hides it again.
 * @returns The field.
 */
Object TextField(Object parent, const Panel& panel, bool secret = false, const lv_image_dsc_t* reveal = nullptr,
                 const lv_image_dsc_t* conceal = nullptr);

Object PlayingInfo(Object parent, const Panel& panel, const lv_image_dsc_t* cover, const char* title,
                   const char* second);

/**
 * A row of keys that belong to one another, evenly spaced.
 *
 * All of them are the same size. One larger key would say that the middle one
 * matters more, and it does not: skipping and volume are reached for just as
 * often as pausing.
 *
 * @param parent What it goes into.
 * @param panel The panel, for the measurements.
 * @param symbols One per key, in the order they stand.
 * @param accent Which of them carries the accent colour, counted from nought,
 *               or negative for none.
 * @returns The group, so a caller can reach its keys.
 */
Object MediaButtons(Object parent, const Panel& panel, std::initializer_list<const lv_image_dsc_t*> symbols,
                    int accent = -1);

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
 * @param across How much of the width it takes, as a percentage. What stands in
 *               it takes the block's width, so this is what makes a heading, a
 *               list and a row of buttons line up with each other.
 * @returns The block, to put things into. They stack downwards.
 */
Object CentredBlock(Object parent, const Panel& panel, int across = 100);

/**
 * What a screen says when there is nothing on it to show.
 *
 * A symbol, a line saying what is going on, and a line saying the rest: an
 * empty list, a place in the product that is not built yet, a welcome before
 * anything has been set up. Named as SwiftUI names it, because it is the same
 * thing and a second name for it would only have to be learnt.
 *
 * The symbol is drawn at the one size the design gives this arrangement, so it
 * is not passed in and cannot be passed differently. The sub-text sits closer to
 * the text than the text does to the symbol, which is what makes the two lines
 * read as one thing.
 *
 * Anything that goes below the words is put into the block that comes back. A
 * button and a progress bar both stand there on screens of this shape, and
 * neither is a control of its own: it is the same button that stands elsewhere.
 *
 * @param parent What it goes into.
 * @param panel The panel, for the measurements.
 * @param icon The symbol above it, or nullptr for none. It has to be supplied
 *             at the size this arrangement draws, which is
 *             `panel(token::kBandContentUnavailableSymbol)`.
 * @param text What is going on.
 * @param sub_text The line under it, or nullptr for none.
 * @returns The block, so a caller can put a control at the bottom of it.
 */
Object ContentUnavailableView(Object parent, const Panel& panel, const lv_image_dsc_t* icon, const char* text,
                              const char* sub_text = nullptr);

/**
 * The filled bar every indicator of a share is drawn from.
 *
 * Narrow on purpose. Where a bar carries a handle, what a finger hits is the
 * handle and the bar only says where it stands; one as tall as a key reads as
 * though it were the control itself. Where it carries none, it reports a share
 * nobody moves, and the same shape says the same thing.
 *
 * It takes the width of whatever it is put into.
 *
 * @param parent What it goes into.
 * @param panel The panel, for the measurements.
 * @param filled How much of it is full, from 0 to 1. Anything outside that is
 *               clamped, because a share cannot be more than all of it.
 * @returns The bar, so a caller can reach the part that is full.
 */
Object Bar(Object parent, const Panel& panel, float filled);

/**
 * A labelled slider: its name at the left, its value at the right, the bar
 * under both.
 *
 * The handle is what answers a finger and is therefore the size a finger needs,
 * whilst the bar under it stays narrow. Dragging it sends
 * `LV_EVENT_VALUE_CHANGED` from the slider, and `SliderAt` says where it now
 * stands.
 *
 * The value is written by the caller rather than computed here. What a share
 * means is the product's business: the same half way along is 50 percent on one
 * screen and minus five on another.
 *
 * @param parent What it goes into.
 * @param panel The panel, for the measurements.
 * @param label What the slider stands for.
 * @param value What it says now, or nullptr for nothing.
 * @param filled Where the handle starts, from 0 to 1.
 * @returns The slider, so a caller can listen to it.
 */
Object Slider(Object parent, const Panel& panel, const char* label, const char* value, float filled);

/**
 * Where a slider's handle stands.
 *
 * @param slider A slider that came from `Slider`.
 * @returns Its position, from 0 to 1.
 */
float SliderAt(Object slider);

/**
 * Writes a slider's value afresh, without building it again.
 *
 * Dragging one changes the figure beside it at every step, and rebuilding the
 * screen for that would redraw the whole panel for a number.
 *
 * @param slider A slider that came from `Slider`.
 * @param value What it says now.
 */
void SetSliderValue(Object slider, const char* value);

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
