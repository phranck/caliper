#include "caliper/components.h"

#include <iterator>

#include "caliper/keyboards.h"
#include "caliper/shapes.h"
#include "caliper/theme.h"
#include "caliper/tokens.h"
#include "lvgl.h"

namespace cal {
namespace {

/**
 * Strips a container of everything that would otherwise show: no background of
 * its own, no border, no padding, no scrolling.
 *
 * A container is a place to put things, and every one of those defaults draws
 * something the design did not ask for.
 */
/// How long a card takes to appear. Long enough to be seen arriving, which is
/// what says it belongs to the screen it is over rather than replacing it, and
/// short enough that nobody waits for it.
constexpr std::uint32_t kCardFadeMs = 180;

/// The flag that says an object is a piece of a band. One of the four the
/// library leaves to whoever is using it.
constexpr lv_obj_flag_t kBandPart = LV_OBJ_FLAG_USER_1;

void MakePlain(Object object) {
   // Scenery, and therefore not a target. The library makes every container
   // clickable, which then reads as a control that is too small to hit;
   // anything that is meant to be touched says so itself.
   lv_obj_remove_flag(object, LV_OBJ_FLAG_CLICKABLE);

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
void AlignOptically(Object label, const Panel& panel, lv_align_t alignment, Point size, std::int32_t inset = 0) {
   lv_obj_align(label, alignment, inset, -panel.OpticalOffset(size).value);
}

}  // namespace

Screen::Screen(const Panel& panel, Frame frame) : panel_(panel), frame_(frame) {
   root_ = lv_screen_active();
   MakePlain(root_);
   lv_obj_set_style_bg_color(root_, lv_color_hex(token::kBg), 0);
   lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
}

/// What a bare screen keeps clear at each end: the room the two answers in the
/// corners take, and the air around them.
static std::int32_t BareEnd(const Panel& panel) { return 2 * panel(token::kInset).value + panel(token::kButton).value; }

Point Screen::ContentTop() const {
   // A screen carrying a keyboard keeps nothing in its bottom corners, because
   // those corners are keys. The room a bare screen holds for the answers that
   // would stand there is therefore not held at the top either, and what is
   // left goes to the one field being typed into.
   if (keyboard_ != nullptr) {
      return Point{panel_(token::kEdge).value + panel_(token::kInset).value};
   }

   // The same at the top as at the bottom. Nothing stands up there, and the
   // room is kept all the same, because that is what makes the middle of the
   // content the middle of the screen: a block centred in it is then centred on
   // the panel, which is where somebody setting a device up is looking.
   if (frame_ == Frame::kBare) {
      return Point{BareEnd(panel_)};
   }
   return Point{panel_(token::kStatusBar).value + panel_(token::kHeader).value};
}

Point Screen::ContentBottom() const {
   // Where there is a keyboard, it decides: nothing may stand behind it, and
   // it reaches the bottom edge itself.
   if (keyboard_ != nullptr) {
      return Point{lv_obj_get_y(keyboard_)};
   }

   // A bare screen keeps room at the bottom for the two answers every step of
   // a setup carries, one in each corner. They are not a band and draw no
   // surface, but the content stops above them all the same.
   if (frame_ == Frame::kBare) {
      return Point{panel_.height.value - BareEnd(panel_)};
   }

   // There is no footer in this design. The one thing that ever stands down
   // there is the player, and only whilst something is playing, so the room for
   // it is kept where it is taken and nowhere else. Kept on every screen it was
   // an empty strip the height of a row at the bottom of anything without a
   // player, which is what a footer looks like when it holds nothing.
   const std::int32_t below = player_ == nullptr ? panel_(token::kEdge).value : panel_(token::kFooter).value;
   return Point{panel_.height.value - below};
}

Point Screen::RowHeight() const { return panel_(token::kHeader); }

int Screen::RowsVisible(Point row_height) const {
   // Less the distance the content holds at its own bottom edge, the same
   // margin every side of it keeps, so the last row does not crowd it.
   const std::int32_t room = ContentBottom().value - ContentTop().value - panel_(token::kEdge).value;

   if (row_height.value <= 0) {
      return 0;
   }

   // Nothing stands between two rows. A list is one surface, its rows sit
   // straight on each other, and what parts them is a hairline of no height.
   // The gap this used to count belongs between two groups, not between two
   // rows, and counting it reported fewer rows than a screen shows.
   return room / row_height.value;
}

Page Screen::Paginate(Point row_height, int shown, int total) const {
   // The counting itself is Panel's, because it needs a row height and a
   // panel to measure against and nothing that only a live screen carries.
   // That is what makes it testable on a machine with no board.
   return panel_.Paginate(RowsVisible(row_height), shown, total);
}

Object Screen::status_bar(const StatusBar& status) {
   // Beside the sidebar rather than above it. The sidebar runs the whole height
   // because it belongs to the device, and the status bar says what the device
   // is doing about what stands beside it.
   const std::int32_t aside = sidebar_ == nullptr ? 0 : panel_(token::kSidebar).value;

   Object band = lv_obj_create(root_);
   MakePlain(band);
   lv_obj_set_size(band, panel_.width.value - aside, panel_(token::kStatusBar).value);
   lv_obj_set_pos(band, aside, 0);
   lv_obj_set_style_bg_color(band, lv_color_hex(token::kSurface), 0);
   lv_obj_set_style_bg_opa(band, LV_OPA_COVER, 0);
   // More room at the left than at the right, because what stands at the left
   // is a line of words and what stands at the right is a run of symbols with
   // their own air around them.
   lv_obj_set_style_pad_left(band, panel_(token::kEdge).value, 0);
   lv_obj_set_style_pad_right(band, panel_(token::kStatusEdge).value, 0);
   status_ = band;

   // Everything sits at the right end, in the order a person reads it: how
   // full it is, what the radio is doing, then the time. Laid out from the
   // right rather than placed, so leaving one out closes the gap.
   lv_obj_set_flex_flow(band, LV_FLEX_FLOW_ROW);
   lv_obj_set_flex_align(band, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
   lv_obj_set_style_pad_column(band, panel_(token::kStatusGap).value, 0);

   const auto symbol = [&](const lv_image_dsc_t* source) -> Object {
      Object image = lv_image_create(band);
      lv_image_set_src(image, source);
      lv_obj_set_style_image_recolor(image, lv_color_hex(token::kStatusInk), 0);
      lv_obj_set_style_image_recolor_opa(image, LV_OPA_COVER, 0);

      // Its own size, stated. Left to the layout an image is stretched or
      // squeezed to whatever the row has left, and a symbol that is a point
      // narrower than it is tall reads as a mistake without looking like one.
      lv_obj_set_size(image, source->header.w, source->header.h);
      return image;
   };

   // Everything in the bar is one weight. What separates the time from the
   // rest is its colour, not a second cut: at 19 points a heavier face reads
   // as a different typeface rather than as emphasis.
   const auto text = [&](const char* content, std::uint32_t colour) -> Object {
      Object label = lv_label_create(band);
      lv_label_set_text(label, content);
      lv_obj_set_style_text_color(label, lv_color_hex(colour), 0);

      const lv_font_t* face = typography().status != nullptr ? typography().status : typography().small;
      if (face != nullptr) {
         lv_obj_set_style_text_font(label, face, 0);
      }

      // No optical nudge here, and that is the exception rather than an
      // oversight. The nudge lifts a line so its capitals sit on the middle of
      // a row that also holds descenders. The status bar holds figures and
      // capitals beside symbols, and the symbols set the line: measured on the panel,
      // lifting the text put it two points above them.
      return label;
   };

   // A spacer that grows, so everything the bar reports sits at the right end.
   // Nothing stands at the left: the status bar reports the device, and what
   // the screen is about is said by the header under it.
   Spacer(band);

   // The battery and its figure first, then the radio. The two battery items
   // belong together and are read as one, so the radio stands beside the pair
   // rather than between them.
   if (status.battery != nullptr) {
      symbol(status.battery);
   }

   if (status.charge >= 0) {
      static char charge[8];
      lv_snprintf(charge, sizeof(charge), "%d %%", status.charge);
      text(charge, token::kStatusInk);
   }

   if (status.network != nullptr) {
      signal_ = symbol(status.network);
   }

   if (status.clock != nullptr) {
      clock_ = text(status.clock, token::kStatusInk);
   }

   return band;
}

Object Screen::Header(const char* title, const WayBack& back, const char* note) {
   // Beside the sidebar, not above it. The status bar reports the state of the
   // device and therefore spans everything; a header says what one is looking
   // at inside an area, and the area begins where the sidebar ends.
   const std::int32_t aside = sidebar_ == nullptr ? 0 : panel_(token::kSidebar).value;
   const std::int32_t above = frame_ == Frame::kBare ? 0 : panel_(token::kStatusBar).value;

   Object band = lv_obj_create(root_);
   MakePlain(band);
   lv_obj_set_size(band, panel_.width.value - aside, panel_(token::kHeader).value);
   lv_obj_set_pos(band, aside, above);
   // The header's own edge, and half a corner on top of it. A line of type
   // flush against a rounded edge reads as colliding with it, and what stands
   // under the header is the list, whose corner has already pulled it that
   // far in. Half the list's radius, so it moves when the radius does.
   lv_obj_set_style_pad_hor(band, panel_(token::kEdge).value + panel_(token::kRadiusCard).value / 2, 0);
   header_ = band;

   // The name at the trailing end, directly over what it names. The way out of
   // the screen has the leading end, where a hand holding the device reaches
   // without moving, so a name yields the place a thumb wants.
   Object heading = lv_label_create(band);
   lv_label_set_text(heading, title);
   lv_obj_set_style_text_color(heading, lv_color_hex(token::kInk), 0);
   if (typography().heading != nullptr) {
      lv_obj_set_style_text_font(heading, typography().heading, 0);
   }
   AlignOptically(heading, panel_, frame_ == Frame::kBare ? LV_ALIGN_CENTER : LV_ALIGN_RIGHT_MID,
                  panel_.TypeSize(token::kHeading));

   // The way back is one thing that can be touched, holding the symbol and the
   // name of what it goes to. The two are one object rather than two beside
   // each other, because a finger aiming at either of them means the same
   // thing.
   if (back.text != nullptr) {
      Object way = lv_obj_create(band);
      MakePlain(way);
      lv_obj_set_height(way, panel_(token::kMinimum).value);
      lv_obj_set_width(way, LV_SIZE_CONTENT);
      lv_obj_set_flex_flow(way, LV_FLEX_FLOW_ROW);
      lv_obj_set_flex_align(way, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      lv_obj_set_style_pad_column(way, panel_(token::kLineGap).value / 2, 0);
      lv_obj_align(way, LV_ALIGN_LEFT_MID, 0, 0);
      lv_obj_add_flag(way, LV_OBJ_FLAG_CLICKABLE);
      MarkAsBandPart(way);

      if (back.symbol != nullptr) {
         Object arrow = lv_image_create(way);
         lv_image_set_src(arrow, back.symbol);
         lv_obj_set_style_image_recolor(arrow, lv_color_hex(token::kMuted), 0);
         lv_obj_set_style_image_recolor_opa(arrow, LV_OPA_COVER, 0);
         lv_obj_set_size(arrow, back.symbol->header.w, back.symbol->header.h);
      }

      Object says = lv_label_create(way);
      lv_label_set_text(says, back.text);
      lv_obj_set_style_text_color(says, lv_color_hex(token::kMuted), 0);
      if (typography().small != nullptr) {
         lv_obj_set_style_text_font(says, typography().small, 0);
      }

      back_ = way;
   }

   // What the screen says about itself stands beside the way back, at the same
   // end, because both are about where one is rather than about what one is
   // looking at. It is a label and nothing else: nothing here leads anywhere.
   if (note != nullptr) {
      Object aside = lv_label_create(band);
      lv_label_set_text(aside, note);
      lv_obj_set_style_text_color(aside, lv_color_hex(token::kMuted), 0);
      if (typography().small != nullptr) {
         lv_obj_set_style_text_font(aside, typography().small, 0);
      }

      if (back_ == nullptr) {
         AlignOptically(aside, panel_, LV_ALIGN_LEFT_MID, panel_.TypeSize(token::kSmall));
      } else {
         lv_obj_align_to(aside, back_, LV_ALIGN_OUT_RIGHT_MID, panel_(token::kGroup).value, 0);
      }
   }

   return band;
}

Object Screen::sidebar(const Sidebar& areas) {
   const std::int32_t corner = panel_(token::kRadiusButton).value;
   const std::int32_t width = panel_(token::kSidebar).value;
   const std::int32_t item = panel_(token::kSidebarItem).value;
   const std::int32_t gap = panel_(token::kEdge).value;
   const std::int32_t count = static_cast<std::int32_t>(areas.icons.size());

   // The four stand as one block in the middle of what is left under the
   // header, rather than spread over it. Spread, the spacing between them says
   // as much as the spacing to the edges, and they stop reading as one set of
   // four. The middle is taken under the header, because that is where the
   // sidebar's own list begins.
   const std::int32_t under = panel_(token::kStatusBar).value;
   const std::int32_t block = count > 0 ? count * item + (count - 1) * gap : 0;
   const std::int32_t first = under + (panel_.height.value - under - block) / 2;

   const bool opened = areas.active >= 0 && areas.active < count;
   const std::int32_t open_top = first + areas.active * (item + gap);
   const std::int32_t open_bottom = open_top + item;

   // The sidebar and the status bar are one surface, and the open item is a
   // piece of the content reaching into it. So the surface is laid in bands
   // above and below that item rather than as one panel with something drawn on
   // it, and every corner it turns towards the content is rounded.
   //
   // The library gives an object one radius for all four corners, so a band is
   // drawn wider and taller than it shows: what should stay square hangs off
   // the panel, where nothing sees it.
   const auto band = [&](std::int32_t from, std::int32_t to) {
      Object piece = lv_obj_create(root_);
      MakePlain(piece);
      lv_obj_set_pos(piece, -corner, from);
      lv_obj_set_size(piece, width + corner, to - from);
      lv_obj_set_style_bg_color(piece, lv_color_hex(token::kSurface), 0);
      lv_obj_set_style_bg_opa(piece, LV_OPA_COVER, 0);
      lv_obj_set_style_radius(piece, corner, 0);
      MarkAsBandPart(piece);
   };

   if (opened) {
      band(-corner, open_top);
      band(open_bottom, panel_.height.value + corner);
   } else {
      band(-corner, panel_.height.value + corner);
   }

   // Where the sidebar meets the status bar the surface turns a corner, and the
   // content begins inside it. It takes two pieces, because the surface has to
   // be there before anything can be rounded out of it: the square fills the
   // corner, and the disc over it is the content taking its part back.
   Object filled = lv_obj_create(root_);
   MakePlain(filled);
   lv_obj_set_pos(filled, width, panel_(token::kStatusBar).value);
   lv_obj_set_size(filled, corner, corner);
   lv_obj_set_style_bg_color(filled, lv_color_hex(token::kSurface), 0);
   lv_obj_set_style_bg_opa(filled, LV_OPA_COVER, 0);
   MarkAsBandPart(filled);

   Object rounded = lv_obj_create(root_);
   MakePlain(rounded);
   lv_obj_set_pos(rounded, width, panel_(token::kStatusBar).value);
   lv_obj_set_size(rounded, 2 * corner, 2 * corner);
   lv_obj_set_style_bg_color(rounded, lv_color_hex(token::kBg), 0);
   lv_obj_set_style_bg_opa(rounded, LV_OPA_COVER, 0);
   lv_obj_set_style_radius(rounded, corner, 0);
   MarkAsBandPart(rounded);

   // The anchors themselves carry no surface of their own. What marks the open
   // one is the gap the bands leave for it, so there is one answer to where it
   // is rather than two that have to agree.
   sidebar_ = lv_obj_create(root_);
   MakePlain(sidebar_);
   lv_obj_set_pos(sidebar_, 0, 0);
   lv_obj_set_size(sidebar_, width, panel_.height.value);

   // The header, as tall as the status bar beside it, so the wordmark and what
   // the device reports stand on one line across the whole panel.
   Object head = lv_obj_create(sidebar_);
   MakePlain(head);
   lv_obj_set_pos(head, 0, 0);
   lv_obj_set_size(head, width, panel_(token::kStatusBar).value);
   MarkAsBandPart(head);

   if (areas.mark != nullptr && areas.mark_fill != nullptr) {
      mark_ = lv_obj_create(head);
      MakePlain(mark_);
      lv_obj_set_size(mark_, areas.mark->header.w, areas.mark->header.h);
      lv_obj_set_style_bg_opa(mark_, LV_OPA_COVER, 0);
      lv_obj_set_style_bg_grad(mark_, areas.mark_fill, 0);
      lv_obj_set_style_bitmap_mask_src(mark_, areas.mark, 0);
      lv_obj_center(mark_);
   } else if (areas.mark != nullptr) {
      mark_ = lv_image_create(head);
      lv_image_set_src(mark_, areas.mark);
      lv_obj_set_size(mark_, areas.mark->header.w, areas.mark->header.h);
      lv_obj_set_style_image_recolor(mark_, lv_color_hex(token::kStatusInk), 0);
      lv_obj_set_style_image_recolor_opa(mark_, LV_OPA_COVER, 0);
      lv_obj_center(mark_);
   }

   std::int32_t index = 0;
   for (const lv_image_dsc_t* symbol : areas.icons) {
      const bool chosen = index == areas.active;

      Object anchor = lv_obj_create(sidebar_);
      MakePlain(anchor);
      lv_obj_set_size(anchor, width, item);
      lv_obj_set_pos(anchor, 0, first + index * (item + gap));
      lv_obj_add_flag(anchor, LV_OBJ_FLAG_CLICKABLE);

      // The header and the items divide the sidebar between them and each one
      // reaches both its edges, which is what a band does. A margin inside one
      // would be a margin inside the sidebar, and the sidebar has none.
      MarkAsBandPart(anchor);

      // Which area it stands for, on the object itself. The header stands in
      // the same container, so counting children says the wrong thing, and it
      // would go on saying it silently the next time something is added.
      lv_obj_set_user_data(anchor, reinterpret_cast<void*>(static_cast<std::uintptr_t>(index)));

      const bool labelled = index < static_cast<std::int32_t>(areas.names.size());
      if (labelled) {
         // Symbol and name are stacked and centred as one block, because the
         // two are read as one thing. Aligning each to an edge of the item
         // instead parts them by whatever the item has left over.
         lv_obj_set_flex_flow(anchor, LV_FLEX_FLOW_COLUMN);
         lv_obj_set_flex_align(anchor, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
         lv_obj_set_style_pad_row(anchor, panel_(token::kLineGap).value / 2, 0);
         lv_obj_set_style_pad_hor(anchor, panel_(token::kStatusEdge).value, 0);
      }

      Object symbol_mark = lv_image_create(anchor);
      lv_image_set_src(symbol_mark, symbol);
      lv_obj_set_style_image_recolor(symbol_mark, lv_color_hex(chosen ? Accent() : token::kMuted), 0);
      lv_obj_set_style_image_recolor_opa(symbol_mark, LV_OPA_COVER, 0);
      if (labelled) {
         Object name = lv_label_create(anchor);
         lv_label_set_text(name, areas.names.begin()[index]);
         lv_obj_set_style_text_color(name, lv_color_hex(chosen ? Accent() : token::kMuted), 0);
         const lv_font_t* face = typography().caption != nullptr ? typography().caption : typography().status;
         if (face != nullptr) {
            lv_obj_set_style_text_font(name, face, 0);
         }
      } else {
         lv_obj_center(symbol_mark);
      }

      index += 1;
   }
   return sidebar_;
}

Object Screen::player_controller(const PlayerController& player) {
   const std::int32_t aside = sidebar_ == nullptr ? 0 : panel_(token::kSidebar).value;
   const std::int32_t clear = panel_(token::kFloating).value;
   const std::int32_t inset = panel_(token::kPlayerInset).value;
   const std::int32_t key = panel_(token::kMediaKey).value;

   // Its height is what it holds plus the air above and below, so the ends
   // follow whenever either of the two moves.
   const std::int32_t height = key + 2 * inset;
   const std::int32_t width = panel_.width.value - aside - 2 * clear;

   player_ = lv_obj_create(root_);
   MakePlain(player_);
   lv_obj_set_size(player_, width, height);
   lv_obj_set_pos(player_, aside + clear, panel_.height.value - clear - height);

   // The content had the whole screen because nothing stood at the bottom of
   // it, and now something does. A screen asks for the player after it has
   // filled the content, so the room is taken back here rather than being kept
   // on every screen against the chance of one.
   if (content_ != nullptr) {
      lv_obj_set_height(content_, ContentBottom().value - ContentTop().value);
   }

   // The ends are squircles like the keys inside them rather than half circles,
   // and being the height of the pill they are the key's own shape grown by the
   // air around it. So the outline runs parallel to what stands in it at every
   // point, which a circle does not. The middle is what is left between them.
   Object left = Squircle(player_, height, token::kRaised);
   lv_obj_set_pos(left, 0, 0);
   MarkAsBandPart(left);

   Object right = Squircle(player_, height, token::kRaised);
   lv_obj_set_pos(right, width - height, 0);
   MarkAsBandPart(right);

   // The seam between an end and the middle sits on the grid. Half a point is
   // invisible on its own and plain to see where two surfaces meet, which is
   // exactly what a seam is.
   const std::int32_t seam = (height / 2) - (height / 2) % token::kGrid.value;

   Object middle = lv_obj_create(player_);
   MakePlain(middle);
   lv_obj_set_size(middle, width - 2 * seam, height);
   lv_obj_set_pos(middle, seam, 0);
   lv_obj_set_style_bg_color(middle, lv_color_hex(token::kRaised), 0);
   lv_obj_set_style_bg_opa(middle, LV_OPA_COVER, 0);
   MarkAsBandPart(middle);

   // Both groups stand the air's width in from the ends, which is where the
   // squircle of an end sits around the squircle of a key.
   Object info = PlayingInfo(player_, panel_, player.cover, player.title, player.second);
   lv_obj_align(info, LV_ALIGN_LEFT_MID, inset, 0);

   Object keys = MediaButtons(
       player_, panel_, {player.back, player.playing ? player.pause : player.play, player.forward, player.volume}, 1);
   lv_obj_align(keys, LV_ALIGN_RIGHT_MID, -inset, 0);

   return player_;
}

void Screen::MarkAsBandPart(Object object) { lv_obj_add_flag(object, kBandPart); }

bool Screen::IsABand(Object object) const {
   if (lv_obj_has_flag(object, kBandPart)) {
      return true;
   }
   return object == status_ || object == header_ || object == sidebar_ || object == player_ || object == keyboard_;
}

bool Screen::InABand(Object object) const {
   for (Object walk = object; walk != nullptr; walk = lv_obj_get_parent(walk)) {
      if (IsABand(walk)) {
         return true;
      }
   }
   return false;
}

namespace {
constexpr int kLayerCount = 3;

/// The layouts, the sizes they are drawn at and the type that holds them all
/// come from `tokens/keyboards.json`, which the layout page in LEVEL reads as
/// well. Nothing about a keyboard is written here.
using keyboards::Alphabet;
using keyboards::End;
using keyboards::kCharacterKeys;
using keyboards::kRowCount;

/// Which alphabet a language types on.
constexpr const Alphabet& AlphabetFor(KeyboardLayout which) {
   switch (which) {
      case KeyboardLayout::kEnglish:
         return keyboards::kEnglish;
      case KeyboardLayout::kFrench:
         return keyboards::kFrench;
      case KeyboardLayout::kSpanish:
         return keyboards::kSpanish;
      case KeyboardLayout::kItalian:
         return keyboards::kItalian;
      case KeyboardLayout::kGerman:
         break;
   }
   return keyboards::kGerman;
}

/// A screen carries one keyboard, so what it is showing is one answer here
/// rather than a field on every key.
struct Showing {
   /// The letters and the two layers of symbols, chosen from `keys.layout`
   /// when the keyboard was built.
   const Alphabet* alphabet = nullptr;

   /// What each key says, and over it what shift would make of it.
   Object labels[kCharacterKeys] = {};
   Object shifted[kCharacterKeys] = {};

   /// Where each row's keys begin in those lists, and how many it has. A layer
   /// with fewer characters than the letters leaves keys over at the end of a
   /// row, and without this the next row's characters would run into them.
   int begins[kRowCount] = {};
   int holds[kRowCount] = {};

   Object band = nullptr;
   Object field = nullptr;

   const lv_image_dsc_t* shift = nullptr;

   /// What the key that leads to the symbols says, because it is also the key
   /// that comes back from them and so it names where it leads.
   Object layer_key = nullptr;

   /// Whether the symbols are showing rather than the letters, and whether
   /// shift is on. Shift belongs to the letters alone.
   bool symbols = false;
   bool capital = false;

   /// How tall a key is, how tall each of the two characters stands, and how
   /// much room is kept between them. A key is written again whenever the layer
   /// or shift changes, and where it says one character it is placed differently
   /// from where it says two, so these figures have to outlast the row's making.
   std::int32_t key_height = 0;
   std::int32_t stack = 0;
   std::int32_t char_cap = 0;
   std::int32_t shifted_cap = 0;
};

Showing showing;

/// How many bytes the character at this position takes. The layers carry
/// characters the design wrote by hand, and several of them are not one byte.
int CharacterBytes(const char* at) {
   const unsigned char lead = static_cast<unsigned char>(*at);
   if ((lead & 0x80U) == 0) {
      return 1;
   }
   if ((lead & 0xE0U) == 0xC0U) {
      return 2;
   }
   if ((lead & 0xF0U) == 0xE0U) {
      return 3;
   }
   return 4;
}

/// How many characters a string holds, a multi-byte one counted once. What a
/// row's width is computed from, since a column is a character and not a
/// byte.
int Columns(const char* text) {
   int count = 0;
   for (const char* at = text; *at != '\0'; count += 1) {
      at += CharacterBytes(at);
   }
   return count;
}

/**
 * Puts a label so that the top of its capitals lands where it is wanted.
 *
 * A label is placed by its box, and a box is a whole line tall: it keeps the
 * room above the letter that a second line of text would need. What one sees on
 * a key is the capital, so the room above it is measured out of the way first.
 *
 * @param label The label, with its face already set.
 * @param top Where the top of a capital is to land, from the top of the key.
 * @param cap How tall a capital is in that face.
 */
void CapAt(Object label, std::int32_t top, std::int32_t cap) {
   const lv_font_t* face = lv_obj_get_style_text_font(label, LV_PART_MAIN);
   const std::int32_t above =
       face == nullptr ? 0 : static_cast<std::int32_t>(lv_font_get_line_height(face)) - face->base_line - cap;
   lv_obj_align(label, LV_ALIGN_TOP_MID, 0, top - above);
}

/**
 * Places what one key says, by whether a shifted mark stands over it.
 *
 * A key carrying both holds the pair in the middle of itself, with the mark
 * over the character. A key carrying only the character puts that character in
 * the middle of the key: leaving it where the pair had it would sit it low and
 * read as though the mark had gone missing rather than never been there.
 *
 * @param index Which key, in the lists `showing` keeps.
 * @param marked Whether a shifted mark stands over the character.
 */
void Place(int index, bool marked) {
   if (showing.labels[index] == nullptr) {
      return;
   }
   if (marked) {
      const std::int32_t both = showing.shifted_cap + showing.stack + showing.char_cap;
      const std::int32_t first = (showing.key_height - both) / 2;
      CapAt(showing.shifted[index], first, showing.shifted_cap);
      CapAt(showing.labels[index], first + showing.shifted_cap + showing.stack, showing.char_cap);
   } else {
      CapAt(showing.labels[index], (showing.key_height - showing.char_cap) / 2, showing.char_cap);
   }
}

/// Copies one character out of a string, however many bytes it takes.
void OneCharacter(const char* at, int bytes, char* into) {
   for (int byte = 0; byte < bytes && byte < 4; ++byte) {
      into[byte] = at[byte];
   }
   into[bytes < 4 ? bytes : 4] = '\0';
}

/**
 * Puts one character into its capital, where it has one.
 *
 * Every letter these layouts type is either an unaccented one or an accented
 * one from the first supplementary block, and those two are the two cases here.
 * An accented letter is two bytes long, and within that block the capitals sit
 * a fixed distance below the small letters, which is why the second byte alone
 * moves. The sharp s and the small letter y with diaeresis both fall outside
 * that run, and both are left alone: the first has no capital on a keyboard,
 * and the second's capital is not in this block at all.
 *
 * @param one The character, in place, as up to four bytes and a terminator.
 */
void ToCapital(char* one) {
   if (one[0] >= 'a' && one[0] <= 'z' && one[1] == '\0') {
      one[0] = static_cast<char>(one[0] - ('a' - 'A'));
      return;
   }

   constexpr unsigned char kSupplement = 0xC3;
   constexpr unsigned char kFirstSmall = 0xA0;
   constexpr unsigned char kLastSmall = 0xBE;
   constexpr unsigned char kDivision = 0xB7;
   constexpr unsigned char kToCapital = 0x20;
   const auto second = static_cast<unsigned char>(one[1]);
   if (static_cast<unsigned char>(one[0]) == kSupplement && second >= kFirstSmall && second <= kLastSmall &&
       second != kDivision && one[2] == '\0') {
      one[1] = static_cast<char>(second - kToCapital);
   }
}

/// Reads one character out of a row and steps past it, or gives nothing back
/// where the row has ended or holds a space there.
///
/// A space is a key the row has nothing for. It still takes its place, so what
/// follows stands where it stands in the letters rather than sliding a key to
/// the left.
///
/// @param at Where in the row to read, moved on by one character.
/// @param into Where to put it, left empty where there is nothing.
void TakeOne(const char** at, char* into) {
   into[0] = '\0';
   if (*at == nullptr || **at == '\0') {
      return;
   }
   const int bytes = CharacterBytes(*at);
   if (**at != ' ') {
      OneCharacter(*at, bytes, into);
   }
   *at += bytes;
}

/**
 * Writes what each key says.
 *
 * The symbols are laid over the letters rather than put in their place: a key
 * the symbols reach shows one and types it, and every other key goes on being
 * the letter it was, mark and all, and goes on working. Switching therefore
 * brings something in and takes nothing away, which is why no key is ever
 * empty and why shift keeps its meaning whilst the symbols are up.
 */
void Relabel() {
   const Alphabet* alphabet = showing.alphabet;
   if (alphabet == nullptr) {
      return;
   }

   for (int row = 0; row < kRowCount; ++row) {
      const char* letters = alphabet->rows[row];
      const char* marks = alphabet->shifted[row];
      const char* extras = showing.symbols ? alphabet->symbols[row] : nullptr;

      for (int place = 0; place < showing.holds[row]; ++place) {
         const int index = showing.begins[row] + place;

         char one[5] = {};
         char above[5] = {};
         char extra[5] = {};
         TakeOne(&letters, one);
         TakeOne(&marks, above);
         TakeOne(&extras, extra);

         // A symbol takes the key for as long as the symbols are up, and it has
         // no second case, so nothing stands over it.
         if (extra[0] != '\0') {
            lv_label_set_text(showing.labels[index], extra);
            lv_label_set_text(showing.shifted[index], "");
            Place(index, false);
            continue;
         }

         // Shift shows what it would make of the key, in place of the key's own
         // character and at its size. Where it makes nothing else, it makes a
         // capital. Either way the small mark goes, because what it announced
         // has arrived.
         if (showing.capital) {
            if (above[0] != '\0') {
               lv_label_set_text(showing.labels[index], above);
            } else {
               ToCapital(one);
               lv_label_set_text(showing.labels[index], one);
            }
            lv_label_set_text(showing.shifted[index], "");
            Place(index, false);
            continue;
         }

         lv_label_set_text(showing.labels[index], one);
         lv_label_set_text(showing.shifted[index], above);
         Place(index, above[0] != '\0');
      }
   }

   // The way back to the letters is written on the key that led away from them.
   if (showing.layer_key != nullptr) {
      lv_label_set_text(showing.layer_key, showing.symbols ? "abc" : "#+=");
   }
}

/// What a key does when it is touched.
enum class Does {
   kType,
   kModify,
   kLayerToggle,
   kSpace,
   kBackspace,
   kEscape,
};

/**
 * How wide each of a row's keys is when the row fills the width it was given.
 *
 * A row is asked to reach both edges of the panel, so its keys share whatever
 * is left once the gaps are taken out. That share is rarely a whole number of
 * grid steps, and a key off the grid puts every key after it off the grid too,
 * so the share is floored to a step and the remainder handed back one step at
 * a time.
 *
 * The keys that receive a step are the outermost first, working inwards, which
 * keeps the row symmetric: a remainder given out from one end would make a row
 * that is visibly heavier on that side.
 */
struct Spread {
   /// What every key is at least.
   std::int32_t base;

   /// How many of them carry one step more than that.
   int wider;
};

/**
 * Works out the widths for a row that has to fill a given span.
 *
 * @param span How wide the row must come out, edge to edge.
 * @param count How many keys share it.
 * @param gap Between two of them.
 * @param step The grid every edge has to land on.
 * @returns The base width and how many keys carry a step more.
 */
constexpr Spread SpreadAcross(std::int32_t span, int count, std::int32_t gap, std::int32_t step) {
   if (count <= 0) {
      return Spread{0, 0};
   }
   const std::int32_t room = span - (count - 1) * gap;
   const std::int32_t base = (room / count) - (room / count) % step;
   return Spread{base, static_cast<int>((room - count * base) / step)};
}

/**
 * The width of one key in a spread row.
 *
 * @param which Its place in the row, counted from the leading edge.
 * @param count How many keys the row has.
 * @param spread What `SpreadAcross` worked out for it.
 * @param step The grid.
 * @returns That key's width.
 */
constexpr std::int32_t WidthAt(int which, int count, const Spread& spread, std::int32_t step) {
   // How far from an edge this key is, counting the leading edge first, then
   // the trailing one, then one in from each, and so on. The keys with the
   // lowest ranks are the ones that take the remainder.
   const int rank = which < count - 1 - which ? 2 * which : 2 * (count - 1 - which) + 1;
   return spread.base + (rank < spread.wider ? step : 0);
}

/// What a key carries so its handler knows what it is for.
void* Touching(Does does) { return reinterpret_cast<void*>(static_cast<std::uintptr_t>(does)); }

void KeyTouched(lv_event_t* event) {
   Object key = static_cast<Object>(lv_event_get_target(event));
   const Does does = static_cast<Does>(reinterpret_cast<std::uintptr_t>(lv_event_get_user_data(event)));

   switch (does) {
      // The character comes from the label the key was given to hold it, not
      // from whichever of its labels happens to stand first. A key that writes
      // carries two, and the one added first is the small mark.
      case Does::kType: {
         Object says = static_cast<Object>(lv_obj_get_user_data(key));
         if (showing.field != nullptr && says != nullptr) {
            lv_textarea_add_text(showing.field, lv_label_get_text(says));
         }
         break;
      }
      // Shift belongs to the letters, which is the only layer with a second
      // case. The way back to the symbols is not here, because a key that
      // sometimes changes the layer and sometimes changes the case is the one a
      // hand gets wrong.
      case Does::kModify:
         showing.capital = !showing.capital;
         Relabel();
         break;
      case Does::kLayerToggle:
         showing.symbols = !showing.symbols;
         showing.capital = false;
         Relabel();
         break;
      case Does::kSpace:
         if (showing.field != nullptr) {
            lv_textarea_add_text(showing.field, " ");
         }
         break;
      case Does::kBackspace:
         if (showing.field != nullptr) {
            lv_textarea_delete_char(showing.field);
         }
         break;
      case Does::kEscape:
         lv_obj_send_event(showing.band, LV_EVENT_CANCEL, nullptr);
         break;
   }
}

}  // namespace

/// How long the caret stands and how long it is gone, in milliseconds. The
/// graphics library's own figure for it, kept here because a caret that blinks
/// at some other rate reads as a device that is busy rather than as one
/// waiting.
constexpr std::uint32_t kCaretBlink = 400;

Object TextField(Object parent, const Panel& panel, bool secret, const lv_image_dsc_t* reveal,
                 const lv_image_dsc_t* conceal) {
   Object field = lv_textarea_create(parent);
   lv_textarea_set_one_line(field, true);
   lv_textarea_set_password_mode(field, secret);

   lv_obj_set_style_bg_color(field, lv_color_hex(token::kRaised), 0);
   lv_obj_set_style_bg_opa(field, LV_OPA_COVER, 0);
   lv_obj_set_style_border_width(field, 0, 0);
   lv_obj_set_style_radius(field, panel(token::kRadiusButton).value, 0);
   lv_obj_set_style_pad_hor(field, panel(token::kInset).value, 0);
   lv_obj_set_style_text_color(field, lv_color_hex(token::kInk), 0);

   // The caret carries the accent, because it is the one thing on the screen
   // that says where the next character lands.
   lv_obj_set_style_bg_color(field, lv_color_hex(Accent()), LV_PART_CURSOR);
   lv_obj_set_style_bg_opa(field, LV_OPA_COVER, LV_PART_CURSOR);

   // And it is a point narrower than the letter it stands before. The graphics
   // library offers no width for a caret: it takes the width of that letter and
   // adds the padding, so the padding is where a point comes off.
   lv_obj_set_style_pad_right(field, -1, LV_PART_CURSOR);

   // And it blinks, which the graphics library does only where this is set: at
   // nought it deletes the animation and leaves the caret standing.
   //
   // What that costs depends on something this library does not decide. Where
   // the display draws what changed, it is the caret's own rectangle twice a
   // second, a few points by a line's height. Where the display redraws the
   // whole screen for any change at all, it is the whole screen twice a second,
   // and on a panel of 800 by 480 that alone spends a quarter of the time
   // redrawing a screen nobody touched, with every keystroke queued behind it.
   // A caret is the cheapest thing on a screen or one of the dearest, and which
   // one is settled where the display is set up.
   lv_obj_set_style_anim_duration(field, kCaretBlink, LV_PART_CURSOR);

   // As tall as a button, because a field is a control and not a row in a
   // list. At a row's height it stands over the keyboard as the largest thing
   // on the screen, which is not what is being read there.
   lv_obj_set_height(field, panel(token::kButton).value);

   // A grade below the heading over it, and in the plain cut.
   const lv_font_t* face = typography().field != nullptr ? typography().field : typography().body;
   if (face != nullptr) {
      lv_obj_set_style_text_font(field, face, 0);

      // A field of a stated height puts its one line at the top, and the caret
      // with it. The same air above and below puts the line in the middle of
      // the field, and it puts the middle of what the field holds on the middle
      // of the field itself, which is what the key at the end aligns to.
      const std::int32_t air = (panel(token::kButton).value - lv_font_get_line_height(face)) / 2;
      lv_obj_set_style_pad_ver(field, air, 0);
   }

   // A round bullet rather than the asterisk the library uses by default. An
   // asterisk sits high in the line and is drawn small at any size, so a field
   // of them reads as small type however large the face is.
   lv_textarea_set_password_bullet(field, "\u2022");

   // The key that shows the word and hides it again. It stands inside the
   // field at its right end, and the text stops before it rather than running
   // underneath.
   if (secret && reveal != nullptr && conceal != nullptr) {
      // As large as a key, so a finger hits it, and set in from the right edge
      // by the same inset the text holds at the left. It is aligned inside the
      // text's own box, so the room made for it has to be given back.
      const std::int32_t size = panel(token::kButton).value;
      const std::int32_t inset = panel(token::kInset).value;

      Object looking = lv_obj_create(field);
      MakePlain(looking);
      lv_obj_set_size(looking, size, size);
      lv_obj_align(looking, LV_ALIGN_RIGHT_MID, inset + size, 0);
      lv_obj_add_flag(looking, LV_OBJ_FLAG_CLICKABLE);

      Object mark = lv_image_create(looking);
      lv_image_set_src(mark, conceal);
      lv_obj_set_style_image_recolor(mark, lv_color_hex(token::kMuted), 0);
      lv_obj_set_style_image_recolor_opa(mark, LV_OPA_COVER, 0);
      lv_obj_center(mark);

      lv_obj_set_style_pad_right(field, 2 * inset + size, 0);

      // Which two symbols the key swaps between. Kept here rather than on the
      // object, for the same reason the keyboard keeps its layer here: a screen
      // carries one field being typed into.
      static const lv_image_dsc_t* pair[2];
      pair[0] = conceal;
      pair[1] = reveal;
      lv_obj_set_user_data(looking, field);

      lv_obj_add_event_cb(
          looking,
          [](lv_event_t* event) {
             Object key = static_cast<Object>(lv_event_get_target(event));
             Object shown = static_cast<Object>(lv_obj_get_user_data(key));
             const bool hidden = lv_textarea_get_password_mode(shown);

             lv_textarea_set_password_mode(shown, !hidden);
             lv_image_set_src(lv_obj_get_child(key, 0), pair[hidden ? 1 : 0]);
          },
          LV_EVENT_CLICKED, nullptr);
   }

   // The focus is what says the field is the one being typed into, and this
   // screen has one field and nothing else to give it to.
   lv_obj_add_state(field, LV_STATE_FOCUSED);
   return field;
}

Object Screen::keyboard(const Keyboard& keys) {
   const std::int32_t key_width = panel_(token::kKeyboardKey).value;
   const std::int32_t key_height = panel_(token::kKeyboardKeyHigh).value;
   const std::int32_t gap = panel_(token::kKeyboardGap).value;
   const std::int32_t edge = panel_(token::kKeyboardEdge).value;
   const std::int32_t corner = panel_(token::kKeyboardCorner).value;
   const std::int32_t available = panel_.width.value - 2 * edge;

   // Five rows: the four the alphabet decides, and the one that carries space
   // and the answers. Set from the bottom up, so the last row holds the same
   // margin as the first.
   constexpr int kRows = kRowCount + 1;
   const std::int32_t top = panel_.height.value - edge - kRows * key_height - (kRows - 1) * gap;
   const auto row_at = [&](int row) { return row * (key_height + gap); };

   keyboard_ = lv_obj_create(root_);
   MakePlain(keyboard_);
   showing = Showing{};
   showing.band = keyboard_;
   lv_obj_set_pos(keyboard_, 0, top);
   lv_obj_set_size(keyboard_, panel_.width.value, panel_.height.value - top);

   // The keys lie on a tray rather than on the screen itself. Without it the
   // gaps between them read as the screen showing through, and the keyboard
   // stops being one thing.
   lv_obj_set_style_bg_color(keyboard_, lv_color_hex(token::kKeyTray), 0);
   lv_obj_set_style_bg_opa(keyboard_, LV_OPA_COVER, 0);

   showing.field = keys.field;
   showing.shift = keys.shift;
   showing.alphabet = &AlphabetFor(keys.layout);
   const Alphabet& letters = *showing.alphabet;

   // One cap. Three steps, and the middle one is where a key stops writing: a
   // control is a step above the tray it lies on, and a key that writes is well
   // clear of both. No key carries an edge, because a keyboard read in a hurry
   // is read by its fill.

   const auto cap = [&](std::int32_t left, std::int32_t row_top, std::int32_t width, std::uint32_t fill, Does does) {
      Object key = lv_obj_create(keyboard_);
      MakePlain(key);
      lv_obj_set_pos(key, left, row_top);
      lv_obj_set_size(key, width, key_height);
      lv_obj_set_style_radius(key, panel_(token::kRadiusButton).value, 0);
      lv_obj_set_style_bg_color(key, lv_color_hex(fill), 0);
      lv_obj_set_style_bg_opa(key, LV_OPA_COVER, 0);

      // A held key stands in the accent and nothing else. A flag above it would
      // have one character to show, namely the one already on the key, and what
      // was typed stands in the field above anyway.
      lv_obj_set_style_bg_color(key, lv_color_hex(Accent()), LV_STATE_PRESSED);

      lv_obj_add_flag(key, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(key, KeyTouched, LV_EVENT_CLICKED, Touching(does));
      return key;
   };

   /// A surface that carries a row out to the margin and does nothing. It takes
   /// no touch, because a key that answers nothing is worse than none.
   const auto filler = [&](std::int32_t left, std::int32_t row_top, std::int32_t width) {
      if (width <= 0) {
         return;
      }
      Object rest = lv_obj_create(keyboard_);
      MakePlain(rest);
      lv_obj_set_pos(rest, left, row_top);
      lv_obj_set_size(rest, width, key_height);
      lv_obj_set_style_radius(rest, panel_(token::kRadiusButton).value, 0);
      lv_obj_set_style_bg_color(rest, lv_color_hex(token::kKeyFiller), 0);
      lv_obj_set_style_bg_opa(rest, LV_OPA_COVER, 0);
   };

   // What stands on a key is stated as the height of a capital, because that is
   // the part one sees. A typeface wants a point size for it, and at this
   // design's cap ratio of seven tenths that is the cap divided by it.
   const auto typeSize = [&](Millimeter cap) { return Point{(panel_(cap).value * 10 + 3) / 7}; };

   const auto lettering = [&](Object key, const char* what, std::uint32_t ink, Millimeter cap) {
      Object label = lv_label_create(key);
      lv_label_set_text(label, what);
      lv_obj_set_style_text_color(label, lv_color_hex(ink), 0);

      // Two cuts, one for the character and a smaller one for what shift makes
      // of it. Which is which follows from the cap that was asked for, so a
      // caller states a height and never a face.
      const bool small = cap.value < token::kKeyboardChar.value;
      const lv_font_t* face = small ? typography().key_shifted : typography().key;
      if (face == nullptr) {
         face = small ? typography().caption : typography().body;
      }
      if (face != nullptr) {
         lv_obj_set_style_text_font(label, face, 0);
      }
      AlignOptically(label, panel_, LV_ALIGN_CENTER, typeSize(cap));
      return label;
   };

   const auto marking = [&](Object key, const lv_image_dsc_t* symbol) {
      Object mark = lv_image_create(key);
      lv_image_set_src(mark, symbol);
      lv_obj_set_style_image_recolor(mark, lv_color_hex(token::kKeyMark), 0);
      lv_obj_set_style_image_recolor_opa(mark, LV_OPA_COVER, 0);
      return mark;
   };

   // A key that writes carries its character, and over it what shift would make
   // of it. The two are labels placed by hand rather than two lines of one
   // label, because a line of text keeps room above and below it that a key has
   // no use for. `Place` does the placing, and these are the figures it needs.
   int index = 0;
   showing.key_height = key_height;
   showing.stack = panel_(token::kKeyboardStack).value;
   showing.char_cap = panel_(token::kKeyboardChar).value;
   showing.shifted_cap = panel_(token::kKeyboardShifted).value;
   const auto writes = [&](std::int32_t left, std::int32_t row_top, const char* one) {
      Object key = cap(left, row_top, key_width, token::kKey, Does::kType);
      if (index >= kCharacterKeys) {
         return;
      }
      Object over = lettering(key, "", token::kKeyShifted, token::kKeyboardShifted);
      Object under = lettering(key, one, token::kKeyInk, token::kKeyboardChar);

      showing.shifted[index] = over;
      showing.labels[index] = under;

      // The key holds on to the label that carries its character, so touching
      // it types what one reads on it. The alternative is to take whichever
      // label stands first, and that is the mark.
      lv_obj_set_user_data(key, under);

      // Where the two stand is settled when the key is written rather than
      // here, because it depends on what the key says and that changes with the
      // layer and with shift.
      Place(index, false);
      index += 1;
   };

   /// Lays one row: what stands at its leading end, the characters, and what
   /// stands at its trailing end, which takes whatever they leave.
   const auto lay = [&](int row) {
      const std::int32_t y = row_at(row);
      const char* text = letters.rows[row];
      const int count = Columns(text);
      const std::int32_t spent = count * key_width + (count - 1) * gap;
      const End lead_is = letters.ends[row][0];
      const End trail_is = letters.ends[row][1];
      const std::int32_t lead = lead_is == End::kNothing ? 0 : panel_(letters.lead[row]).value;
      const std::int32_t trail = available - spent - lead - (lead > 0 ? gap : 0) - gap;

      std::int32_t x = edge;
      if (lead > 0) {
         if (lead_is == End::kShift) {
            Object key = cap(x, y, lead, token::kKeyQuiet, Does::kModify);
            if (keys.shift != nullptr) {
               lv_obj_align(marking(key, keys.shift), LV_ALIGN_BOTTOM_LEFT, corner, -corner);
            }
         } else {
            filler(x, y, lead);
         }
         x += lead + gap;
      }

      // Every layer draws as many keys as the letters need, because a layer with
      // fewer characters must not move the keys the others share.
      showing.begins[row] = index;
      showing.holds[row] = count;
      for (const char* at = text; *at != '\0';) {
         const int bytes = CharacterBytes(at);
         char one[5] = {};
         OneCharacter(at, bytes, one);
         writes(x, y, one);
         x += key_width + gap;
         at += bytes;
      }

      if (trail <= 0 || trail_is == End::kNothing) {
         return;
      }
      if (trail_is == End::kShift) {
         Object key = cap(x, y, trail, token::kKeyQuiet, Does::kModify);
         if (keys.shift != nullptr) {
            lv_obj_align(marking(key, keys.shift), LV_ALIGN_BOTTOM_RIGHT, -corner, -corner);
         }
      } else if (trail_is == End::kBackspace) {
         Object key = cap(x, y, trail, token::kKeyQuiet, Does::kBackspace);
         if (keys.backspace != nullptr) {
            lv_obj_align(marking(key, keys.backspace), LV_ALIGN_BOTTOM_RIGHT, -corner, -corner);
         }
      } else {
         filler(x, y, trail);
      }
   };

   for (int row = 0; row < kRowCount; ++row) {
      lay(row);
   }

   // The bottom row. One key leads to the symbols and comes back from them, so
   // it is the one key here that changes what it says. Escape stands at the
   // trailing edge, because it is the way back and every screen keeps that
   // where a thumb already expects it. The space bar takes what the three of
   // them leave, and is filled like a key that writes because that is what it
   // does.
   const std::int32_t bottom = row_at(kRowCount);
   const std::int32_t symbols_width = panel_(token::kKeyboardSymbols).value;
   const std::int32_t confirm_width = panel_(token::kKeyboardConfirm).value;
   const std::int32_t escape_width = panel_(token::kKeyboardEscape).value;

   Object symbols = cap(edge, bottom, symbols_width, token::kKeyQuiet, Does::kLayerToggle);
   showing.layer_key = lettering(symbols, "#+=", token::kKeyMark, token::kKeyboardMark);
   lv_obj_align(showing.layer_key, LV_ALIGN_BOTTOM_LEFT, corner, -corner);

   std::int32_t right = edge + available;
   if (keys.escape != nullptr) {
      Object escaping = cap(right - escape_width, bottom, escape_width, token::kKeyQuiet, Does::kEscape);
      lv_obj_align(marking(escaping, keys.escape), LV_ALIGN_BOTTOM_RIGHT, -corner, -corner);
      right -= escape_width + gap;
   }

   if (keys.confirm != nullptr) {
      // As tall as the keys beside it rather than as tall as a button
      // elsewhere. Centring a shorter one in the row leaves it a point off the
      // grid, and a key that ends the task is a key of this row first.
      Object done = Button(keyboard_, panel_, keys.confirm, Emphasis::kAccent);
      lv_obj_set_size(done, confirm_width, key_height);
      lv_obj_set_pos(done, right - confirm_width, bottom);
      lv_obj_add_event_cb(
          done, [](lv_event_t*) { lv_obj_send_event(showing.band, LV_EVENT_READY, nullptr); }, LV_EVENT_CLICKED,
          nullptr);
      right -= confirm_width + gap;
   }

   const std::int32_t space_left = edge + symbols_width + gap;
   cap(space_left, bottom, right - gap - space_left, token::kKey, Does::kSpace);

   // The field spans the content it stands in, which is the widest thing on the
   // screen and lands on the grid in every language.
   if (keys.field != nullptr) {
      lv_obj_set_width(keys.field, lv_pct(100));
   }

   // What stands above a keyboard gets the room the keyboard leaves and no
   // more. A screen carrying one also keeps nothing in its bottom corners,
   // because those corners are keys, so the room a bare screen holds for the
   // answers that would stand there is given back at the top too.
   if (content_ != nullptr) {
      lv_obj_set_y(content_, ContentTop().value);
      lv_obj_set_height(content_, ContentBottom().value - ContentTop().value);
      lv_obj_set_flex_align(content_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
   }

   Relabel();
   return keyboard_;
}

Object Screen::card(const Card& what) {
   const std::int32_t inset = panel_(token::kInset).value;
   const std::int32_t symbol = panel_(token::kCardSymbol).value;

   // What is behind it stays where it is and is dimmed. A message about a
   // screen with that screen taken away is a message somebody has to answer
   // from memory.
   dimming_ = lv_obj_create(root_);
   MakePlain(dimming_);
   lv_obj_set_size(dimming_, panel_.width.value, panel_.height.value);
   lv_obj_set_pos(dimming_, 0, 0);
   lv_obj_set_style_bg_color(dimming_, lv_color_hex(token::kBg), 0);
   lv_obj_set_style_bg_opa(dimming_, LV_OPA_70, 0);

   // It catches what is touched, so nothing behind it answers whilst it stands.
   lv_obj_add_flag(dimming_, LV_OBJ_FLAG_CLICKABLE);
   MarkAsBandPart(dimming_);

   card_ = lv_obj_create(dimming_);
   MakePlain(card_);
   lv_obj_set_width(card_, panel_(token::kCard).value);

   // Its height follows what it holds. A stated one gives the air inside it
   // away to whatever happens to be put in.
   lv_obj_set_height(card_, LV_SIZE_CONTENT);
   lv_obj_center(card_);
   lv_obj_set_style_bg_color(card_, lv_color_hex(token::kOverlay), 0);
   lv_obj_set_style_bg_opa(card_, LV_OPA_COVER, 0);
   lv_obj_set_style_pad_all(card_, inset, 0);

   // The card's own corner, which every large surface carries. The button's
   // corner already follows this one, so deriving the card's corner from the
   // button's would be the same rule read backwards.
   lv_obj_set_style_radius(card_, panel_(token::kRadiusCard).value, 0);

   lv_obj_set_flex_flow(card_, LV_FLEX_FLOW_COLUMN);
   lv_obj_set_style_pad_row(card_, panel_(token::kGroup).value, 0);
   MarkAsBandPart(card_);

   // The title, with the symbol for its kind before it.
   Object heading = lv_obj_create(card_);
   MakePlain(heading);
   lv_obj_set_width(heading, lv_pct(100));

   // As tall as the symbol that stands in it, rather than as tall as whatever
   // the typeface gives. A row of a stated height keeps the stack under it on
   // the grid; one that takes its height from a line of text was 43 points on
   // this panel, and every row below it then sat on a half point.
   lv_obj_set_height(heading, symbol);
   lv_obj_set_flex_flow(heading, LV_FLEX_FLOW_ROW);
   lv_obj_set_flex_align(heading, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
   lv_obj_set_style_pad_column(heading, inset, 0);

   if (what.symbol != nullptr) {
      Object mark = lv_image_create(heading);
      lv_image_set_src(mark, what.symbol);
      lv_obj_set_size(mark, symbol, symbol);

      // Centred in the box the token gives it. A symbol is written with its
      // empty border cut off, so it is smaller than that box in one direction
      // or both, and the graphics library would put it in the top left corner.
      // Beside a title it would then read as a symbol that does not line up
      // with the words next to it.
      lv_image_set_inner_align(mark, LV_IMAGE_ALIGN_CENTER);
      lv_obj_set_style_image_recolor(mark, lv_color_hex(what.kind == Card::Kind::kTrouble ? token::kDanger : Accent()),
                                     0);
      lv_obj_set_style_image_recolor_opa(mark, LV_OPA_COVER, 0);
   }

   if (what.title != nullptr) {
      Object words = lv_label_create(heading);
      lv_label_set_text(words, what.title);
      lv_obj_set_style_text_color(words, lv_color_hex(token::kInk), 0);
      if (typography().heading != nullptr) {
         lv_obj_set_style_text_font(words, typography().heading, 0);
      }
   }

   if (what.message != nullptr) {
      Object body = lv_label_create(card_);
      lv_label_set_text(body, what.message);
      lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
      lv_obj_set_width(body, lv_pct(100));
      lv_obj_set_style_text_color(body, lv_color_hex(token::kMuted), 0);
   }

   // Where a caller puts what the message alone cannot say. Before the footer,
   // so the buttons are at the foot however much is put in here, and holding
   // nothing where nobody puts anything: its height follows its contents, so an
   // empty one takes no room and a card without content looks exactly as it did.
   card_content_ = lv_obj_create(card_);
   MakePlain(card_content_);
   lv_obj_set_width(card_content_, lv_pct(100));
   lv_obj_set_height(card_content_, LV_SIZE_CONTENT);
   lv_obj_set_flex_flow(card_content_, LV_FLEX_FLOW_COLUMN);
   lv_obj_set_style_pad_row(card_content_, panel_(token::kLineGap).value, 0);

   // The buttons, at the right end. That is where a step ends, and a row of
   // them starting at the left reads as a list of equal things rather than as
   // one action with a way out beside it.
   card_footer_ = lv_obj_create(card_);
   MakePlain(card_footer_);
   lv_obj_set_width(card_footer_, lv_pct(100));
   lv_obj_set_height(card_footer_, LV_SIZE_CONTENT);
   lv_obj_set_flex_flow(card_footer_, LV_FLEX_FLOW_ROW);
   lv_obj_set_flex_align(card_footer_, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
   lv_obj_set_style_pad_column(card_footer_, panel_(token::kLineGap).value, 0);

   // Faded in rather than put there. It arrives over something somebody was
   // already reading, and a surface that appears between two frames is read as
   // the screen having changed rather than as something being said about it.
   lv_obj_set_style_opa(dimming_, LV_OPA_TRANSP, 0);
   lv_anim_t appearing;
   lv_anim_init(&appearing);
   lv_anim_set_var(&appearing, dimming_);
   lv_anim_set_values(&appearing, LV_OPA_TRANSP, LV_OPA_COVER);
   lv_anim_set_duration(&appearing, kCardFadeMs);
   lv_anim_set_exec_cb(&appearing, [](void* object, int32_t value) {
      lv_obj_set_style_opa(static_cast<Object>(object), static_cast<lv_opa_t>(value), 0);
   });
   lv_anim_start(&appearing);

   // Its height comes out of what it holds, and what it holds is any number of
   // points tall. Centred, half of that height decides where it starts, so an
   // odd figure anywhere in it puts the whole card half a point off the grid
   // and everything standing in it with it.
   lv_obj_update_layout(card_);
   const std::int32_t step = token::kGrid.value;
   lv_obj_align(card_, LV_ALIGN_CENTER, -(lv_obj_get_x(card_) % step), -(lv_obj_get_y(card_) % step));

   return card_;
}

void Screen::DismissCard() {
   if (dimming_ == nullptr) {
      return;
   }

   lv_obj_delete(dimming_);
   dimming_ = nullptr;
   card_ = nullptr;
   card_footer_ = nullptr;
}

Object Screen::Content() {
   if (content_ != nullptr) {
      return content_;
   }

   // The sidebar, where there is one, takes the left of the panel and the
   // content begins beside it. Taken from the design rather than asked of the
   // object: the object has been given its size but no layout has run yet, so
   // asking it returns nought and everything on the screen sits a sidebar's
   // width too far left.
   const std::int32_t aside = sidebar_ == nullptr ? 0 : panel_(token::kSidebar).value;

   content_ = lv_obj_create(root_);
   MakePlain(content_);
   lv_obj_set_pos(content_, aside + panel_(token::kEdge).value, ContentTop().value);
   lv_obj_set_size(content_, panel_.width.value - aside - 2 * panel_(token::kEdge).value,
                   ContentBottom().value - ContentTop().value);

   // Whatever goes in stacks downwards with the design's own gap between two
   // things that belong together, so nothing states a position.
   lv_obj_set_flex_flow(content_, LV_FLEX_FLOW_COLUMN);
   lv_obj_set_style_pad_row(content_, panel_(token::kLineGap).value, 0);

   // Stacked from the top and centred across. Anything narrower than the
   // content stands in the middle of it, because a block set to one side with
   // nothing beside it reads as though something were missing there.
   lv_obj_set_flex_align(content_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

   // The same distance at the bottom that the content holds at its sides.
   // Scrolled to the end, the last row would otherwise sit against whatever
   // stands under it and read as cut off rather than as finished.
   lv_obj_set_style_pad_bottom(content_, panel_(token::kEdge).value, 0);

   // A list scrolls. It costs frames, because a moving surface is the most
   // expensive thing this panel does, and it is what a person expects from a
   // list all the same. The content area is therefore the one container on a
   // screen that scrolls, and it does so vertically only: sideways movement
   // belongs to changing the screen, not to reading one.
   lv_obj_add_flag(content_, LV_OBJ_FLAG_SCROLLABLE);

   // And it can be found by a finger, which is not the same thing. The library
   // only begins a scroll from an object it found under the press, and it only
   // finds what carries this flag: `lv_obj_hit_test` refuses everything else
   // before it so much as looks at the point. `MakePlain` takes the flag off,
   // rightly, because a container is scenery; the content area is the one
   // container on a screen that is meant to be grabbed.
   //
   // Without it, a screen whose rows lead somewhere scrolls, because the finger
   // lands on a row, and a screen of plain rows does not, because it lands on
   // nothing. The two look and measure identically, which is what made it hard
   // to see.
   //
   // Nothing listens here, so this is still not a control: the checks count
   // something as touchable when it is one by type, or when it is clickable and
   // something is listening.
   lv_obj_add_flag(content_, LV_OBJ_FLAG_CLICKABLE);
   lv_obj_set_scroll_dir(content_, LV_DIR_VER);
   lv_obj_set_scrollbar_mode(content_, LV_SCROLLBAR_MODE_AUTO);
   return content_;
}

Object List(Object parent, const Panel& panel, int across) {
   Object group = lv_obj_create(parent);
   MakePlain(group);
   lv_obj_set_width(group, lv_pct(across));
   lv_obj_set_height(group, LV_SIZE_CONTENT);

   // One surface for the whole group, with the corner around all of it. A
   // list is one of the large surfaces, so it carries the card's corner.
   lv_obj_set_style_bg_color(group, lv_color_hex(token::kSurface), 0);
   lv_obj_set_style_bg_opa(group, LV_OPA_COVER, 0);
   lv_obj_set_style_radius(group, panel(token::kRadiusCard).value, 0);
   lv_obj_set_style_clip_corner(group, true, 0);

   // The rows sit directly on each other. What parts them is a line, not a gap.
   lv_obj_set_flex_flow(group, LV_FLEX_FLOW_COLUMN);
   lv_obj_set_style_pad_row(group, 0, 0);
   return group;
}

Object Row(Object parent, const Panel& panel, const char* name, const char* value, const lv_image_dsc_t* icon,
           std::initializer_list<const lv_image_dsc_t*> marks) {
   // Inside a group the row carries no surface of its own: the group draws it
   // once for all of them, and a second one on top would show at the corners.
   const bool grouped = lv_obj_get_style_bg_opa(parent, LV_PART_MAIN) != LV_OPA_TRANSP;

   Object line = lv_obj_create(parent);
   MakePlain(line);
   lv_obj_set_width(line, lv_pct(100));

   // A row is as tall as the header, which is what the design gives both.
   lv_obj_set_height(line, panel(token::kHeader).value);
   lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE);
   lv_obj_set_style_pad_hor(line, panel(token::kInset).value, 0);

   if (!grouped) {
      lv_obj_set_style_bg_color(line, lv_color_hex(token::kSurface), 0);
      lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
      lv_obj_set_style_radius(line, panel(token::kRadiusButton).value, 0);
   }

   std::int32_t text_left = 0;

   if (icon != nullptr) {
      Object symbol = lv_image_create(line);
      lv_image_set_src(symbol, icon);

      // A symbol is tinted rather than coloured in the file: it carries an
      // alpha channel and nothing else, so one image serves a muted row and an
      // accented one. A picture that brings its own colours is left alone,
      // because tinting a flag paints over the thing it is.
      if (icon->header.cf == LV_COLOR_FORMAT_A8) {
         lv_obj_set_style_image_recolor(symbol, lv_color_hex(token::kMuted), 0);
         lv_obj_set_style_image_recolor_opa(symbol, LV_OPA_COVER, 0);
      }
      // Lifted by the same amount the name beside it is. The nudge puts a line
      // of type on the middle of the row by its capitals rather than by its
      // box, and a symbol left on the true middle then sits visibly below the
      // word it belongs to.
      AlignOptically(symbol, panel, LV_ALIGN_LEFT_MID, panel.TypeSize(token::kBody));

      // The gap between a symbol and the word it belongs to is the same
      // everywhere on these screens, and it is the design's own inset.
      text_left = icon->header.w + panel(token::kInset).value;
   }

   if (grouped && lv_obj_get_child_count(parent) > 1) {
      // A hairline above every row but the first, starting where the text
      // starts and running to the far edge of the group. That inset is what
      // makes a list read as a column of entries: a line across the whole
      // width would part it into separate things again, which is the look the
      // group exists to leave behind.
      //
      // Its width has to be a figure rather than a share of the row, because
      // it is the row's width less the indent, so the layout is settled here
      // to ask what that width is.
      lv_obj_update_layout(line);
      const std::int32_t inset = panel(token::kInset).value;

      Object hairline = lv_obj_create(line);
      MakePlain(hairline);
      lv_obj_set_size(hairline, lv_obj_get_width(line) - inset - text_left, 1);
      lv_obj_align(hairline, LV_ALIGN_TOP_LEFT, text_left, 0);
      lv_obj_set_style_bg_color(hairline, lv_color_hex(token::kLine), 0);
      lv_obj_set_style_bg_opa(hairline, LV_OPA_COVER, 0);
      lv_obj_remove_flag(hairline, LV_OBJ_FLAG_CLICKABLE);
   }

   Object label = lv_label_create(line);
   lv_label_set_text(label, name);
   lv_obj_set_style_text_color(label, lv_color_hex(token::kInk), 0);
   AlignOptically(label, panel, LV_ALIGN_LEFT_MID, panel.TypeSize(token::kBody), text_left);

   // The mark sits at the very end, and the value gives way to it. Both at the
   // right edge would put a word and a symbol on top of each other, and which
   // one wins would depend on the length of the word.
   // Laid out from the right backwards, because the last one is the one whose
   // place is fixed and every earlier one follows from it.
   std::int32_t text_right = 0;
   for (auto mark = std::rbegin(marks); mark != std::rend(marks); ++mark) {
      if (*mark == nullptr) {
         continue;
      }
      Object sign = lv_image_create(line);
      lv_image_set_src(sign, *mark);
      lv_obj_set_style_image_recolor(sign, lv_color_hex(token::kMuted), 0);
      lv_obj_set_style_image_recolor_opa(sign, LV_OPA_COVER, 0);
      lv_obj_align(sign, LV_ALIGN_RIGHT_MID, -text_right, 0);
      text_right += (*mark)->header.w + panel(token::kLineGap).value;
   }
   if (text_right > 0) {
      text_right += panel(token::kInset).value - panel(token::kLineGap).value;
   }

   if (value != nullptr) {
      Object right = lv_label_create(line);
      lv_label_set_text(right, value);
      lv_obj_set_style_text_color(right, lv_color_hex(token::kMuted), 0);
      if (typography().small != nullptr) {
         lv_obj_set_style_text_font(right, typography().small, 0);
      }
      AlignOptically(right, panel, LV_ALIGN_RIGHT_MID, panel.TypeSize(token::kSmall), -text_right);
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
Object NameOf(Object row) {
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
void OnRowTouched(lv_event_t* event) {
   Object touched = static_cast<Object>(lv_event_get_target(event));
   Object group = lv_obj_get_parent(touched);

   for (std::uint32_t index = 0; index < lv_obj_get_child_count(group); ++index) {
      Object row = lv_obj_get_child(group, index);
      MarkCurrent(row, row == touched);
   }

   lv_obj_send_event(group, LV_EVENT_VALUE_CHANGED, nullptr);
}

/**
 * Makes a row answer a finger.
 *
 * What the finger gets back before anything else happens. It is the same ground
 * a marked row stands on, at half strength, so pressing a row looks like the
 * beginning of choosing it rather than like a separate colour. Without one a
 * finger gets no answer until the screen changes, and on a panel that redraws
 * in tens of milliseconds that reads as a device that did not notice.
 *
 * @param row The row.
 */
void AnswerToTouch(Object row) {
   lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
   lv_obj_set_style_bg_color(row, lv_color_hex(token::kRaised), LV_STATE_PRESSED);
   lv_obj_set_style_bg_opa(row, LV_OPA_50, LV_STATE_PRESSED);
}

}  // namespace

void MarkCurrent(Object row, bool current) {
   if (current) {
      lv_obj_add_state(row, LV_STATE_CHECKED);
      lv_obj_set_style_bg_color(row, lv_color_hex(token::kRaised), 0);
      lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
   } else {
      lv_obj_remove_state(row, LV_STATE_CHECKED);

      // Back to transparent rather than to the surface colour: inside a group
      // the surface belongs to the group, and painting it again here would
      // show at the corners the group clips.
      lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
   }

   Object name = NameOf(row);
   if (name != nullptr) {
      lv_obj_set_style_text_color(name, lv_color_hex(current ? Accent() : token::kInk), 0);
   }
}

void LeadsAway(Object row) { AnswerToTouch(row); }

void ChooseOne(Object group, int chosen_row) {
   for (std::uint32_t index = 0; index < lv_obj_get_child_count(group); ++index) {
      Object row = lv_obj_get_child(group, index);

      AnswerToTouch(row);
      lv_obj_add_event_cb(row, OnRowTouched, LV_EVENT_CLICKED, nullptr);

      MarkCurrent(row, static_cast<int>(index) == chosen_row);
   }
}

int Chosen(Object group) {
   for (std::uint32_t index = 0; index < lv_obj_get_child_count(group); ++index) {
      if (lv_obj_has_state(lv_obj_get_child(group, index), LV_STATE_CHECKED)) {
         return static_cast<int>(index);
      }
   }
   return -1;
}

Object Heading(Object parent, const Panel& panel, const char* words) {
   Object label = lv_label_create(parent);
   lv_label_set_text(label, words);
   lv_obj_set_style_text_color(label, lv_color_hex(token::kInk), 0);
   if (typography().heading != nullptr) {
      lv_obj_set_style_text_font(label, typography().heading, 0);
   }

   // Its height comes from the typeface, and a typeface is any number of
   // points tall. Rounded up to the grid, because everything stacked beneath
   // it would otherwise inherit whatever odd figure the font happens to give.
   lv_obj_update_layout(label);
   lv_obj_set_height(label, panel.RoundedUpToGrid(Point{lv_obj_get_height(label)}).value);

   return label;
}

Object Button(Object parent, const Panel& panel, const char* label, Emphasis emphasis) {
   Object control = lv_button_create(parent);
   lv_obj_set_style_radius(control, panel(token::kRadiusButton).value, 0);
   lv_obj_set_style_pad_hor(control, panel(token::kInset).value, 0);

   // A minimum rather than a size, so a longer word in another language makes
   // the button wider instead of being cut off inside it.
   lv_obj_set_width(control, LV_SIZE_CONTENT);
   lv_obj_set_style_min_width(control, panel(token::kMinimum).value, 0);

   lv_obj_set_height(control, panel(token::kButton).value);

   // What it is filled with depends on what it stands on. The ordinary fill is
   // the colour of a raised surface, and a button on one of those would be
   // filled with the colour it sits on: only its word would show, and beside an
   // accented button it reads as the smaller of the two. Asked of the surface
   // rather than passed in, because a caller putting a button somewhere should
   // not have to know what colour that place happens to be.
   Object beneath = lv_obj_get_parent(control);
   while (beneath != nullptr && lv_obj_get_style_bg_opa(beneath, LV_PART_MAIN) == LV_OPA_TRANSP) {
      beneath = lv_obj_get_parent(beneath);
   }
   const bool on_a_surface =
       beneath != nullptr && !lv_color_eq(lv_obj_get_style_bg_color(beneath, LV_PART_MAIN), lv_color_hex(token::kBg));

   const std::uint32_t ground = on_a_surface ? token::kSurface : token::kRaised;

   // The warning colour is a fixed one and the accent is whatever the device is
   // set to. That is the point of it: a person changing the colour of their
   // panel does not get to change what an irreversible answer looks like.
   std::uint32_t fill = ground;
   std::uint32_t ink = token::kInk;
   if (emphasis == Emphasis::kAccent) {
      fill = Action();
      ink = ActionInk();
   } else if (emphasis == Emphasis::kWarning) {
      fill = token::kDanger;
      ink = token::kDangerInk;
   }
   lv_obj_set_style_bg_color(control, lv_color_hex(fill), 0);

   Object text = lv_label_create(control);
   lv_label_set_text(text, label);
   if (typography().strong != nullptr) {
      lv_obj_set_style_text_font(text, typography().strong, 0);
   }
   lv_obj_set_style_text_color(text, lv_color_hex(ink), 0);

   AlignOptically(text, panel, LV_ALIGN_CENTER, panel.TypeSize(token::kBody));

   // Its width comes out of the word inside it, and a word is any number of
   // points wide. An odd one cannot be centred on the grid, and two surfaces
   // half a point apart is exactly what the grid is there to prevent.
   lv_obj_update_layout(control);
   lv_obj_set_width(control, panel.RoundedUpToGrid(Point{lv_obj_get_width(control)}).value);

   return control;
}

Object CentredBlock(Object parent, const Panel& panel, int across) {
   Object block = lv_obj_create(parent);
   MakePlain(block);
   lv_obj_set_width(block, lv_pct(across));

   // As tall as what goes into it, which is what lets it be centred at all:
   // a block of a stated height would be centred as that height rather than
   // as its contents.
   lv_obj_set_height(block, LV_SIZE_CONTENT);

   // Placed by its own centring rather than by the content area, which stacks
   // what it holds from the top. Without this the block sits against the header
   // and the centring is asked for and then overruled, which is the sort of
   // thing that looks like a measurement being wrong.
   lv_obj_add_flag(block, LV_OBJ_FLAG_IGNORE_LAYOUT);
   lv_obj_center(block);

   // Centred whilst it fits, and against the top once it does not. A block
   // taller than the place it stands in is centred half above it, and what goes
   // above is cut off: the heading first, which is the one thing that says what
   // the screen is asking. It is decided again whenever the block changes size,
   // because what goes into it arrives after it is made.
   lv_obj_add_event_cb(
       block,
       [](lv_event_t* event) {
          Object grown = static_cast<Object>(lv_event_get_target(event));
          Object around = lv_obj_get_parent(grown);
          if (around == nullptr) {
             return;
          }
          if (lv_obj_get_height(grown) > lv_obj_get_content_height(around)) {
             lv_obj_align(grown, LV_ALIGN_TOP_MID, 0, 0);
          } else {
             lv_obj_center(grown);
          }
       },
       LV_EVENT_SIZE_CHANGED, nullptr);

   lv_obj_set_flex_flow(block, LV_FLEX_FLOW_COLUMN);
   lv_obj_set_flex_align(block, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
   lv_obj_set_style_pad_row(block, panel(token::kGroup).value, 0);
   return block;
}

Object ContentUnavailableView(Object parent, const Panel& panel, const lv_image_dsc_t* icon, const char* text,
                              const char* sub_text) {
   Object block = CentredBlock(parent, panel);

   // The three parts are not evenly spaced, so each says what stands above it
   // rather than the block spacing them all alike. The sub-text sits closer to
   // the text than the text does to the symbol, and that difference is what
   // makes the two lines read as one thing instead of as two.
   lv_obj_set_style_pad_row(block, 0, 0);

   if (icon != nullptr) {
      const std::int32_t size = panel(token::kContentUnavailableSymbol).value;
      Object symbol = lv_image_create(block);
      lv_image_set_src(symbol, icon);
      lv_obj_set_size(symbol, size, size);

      // The square is the size the design gives this arrangement, and the
      // drawing inside it is whatever is left after the generator cuts the
      // empty border off. Centred in the square, because the stack above and
      // below is placed against the square: a drawing left in its corner sits
      // visibly high and to one side of everything it is meant to be over.
      lv_image_set_inner_align(symbol, LV_IMAGE_ALIGN_CENTER);

      lv_obj_set_style_image_recolor(symbol, lv_color_hex(Accent()), 0);
      lv_obj_set_style_image_recolor_opa(symbol, LV_OPA_COVER, 0);
   }

   Object saying = lv_label_create(block);
   lv_label_set_text(saying, text);
   lv_obj_set_style_text_color(saying, lv_color_hex(token::kInk), 0);
   if (typography().heading != nullptr) {
      lv_obj_set_style_text_font(saying, typography().heading, 0);
   }
   if (icon != nullptr) {
      lv_obj_set_style_margin_top(saying, panel(token::kInset).value, 0);
   }

   if (sub_text != nullptr) {
      Object under = lv_label_create(block);
      lv_label_set_text(under, sub_text);
      lv_obj_set_style_text_color(under, lv_color_hex(token::kFaint), 0);
      lv_obj_set_style_margin_top(under, panel(token::kSubTextGap).value, 0);
   }

   return block;
}

Object IconButton(Object parent, const Panel& panel, const lv_image_dsc_t* symbol, Point size, bool accent) {
   Object key = Squircle(parent, size.value, accent ? Action() : token::kKey);
   lv_obj_add_flag(key, LV_OBJ_FLAG_CLICKABLE);

   Object mark = lv_image_create(key);
   lv_image_set_src(mark, symbol);
   lv_obj_set_style_image_recolor(mark, lv_color_hex(accent ? ActionInk() : token::kInk), 0);
   lv_obj_set_style_image_recolor_opa(mark, LV_OPA_COVER, 0);
   lv_obj_center(mark);
   return key;
}

Object PlayingInfo(Object parent, const Panel& panel, const lv_image_dsc_t* cover, const char* title,
                   const char* second) {
   Object group = lv_obj_create(parent);
   MakePlain(group);
   lv_obj_set_size(group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
   lv_obj_set_flex_flow(group, LV_FLEX_FLOW_ROW);
   lv_obj_set_flex_align(group, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
   lv_obj_set_style_pad_column(group, panel(token::kInset).value, 0);

   // The artwork carries the same shape and the same size as a key, so the two
   // ends of the pill hold the same thing in different clothing.
   if (cover != nullptr) {
      SquircleImage(group, panel(token::kMediaKey).value, cover);
   }

   // The two lines stack, and what they are called says which is which: the
   // title is what is playing and the second line is everything about it.
   Object words = lv_obj_create(group);
   MakePlain(words);
   lv_obj_set_size(words, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
   lv_obj_set_flex_flow(words, LV_FLEX_FLOW_COLUMN);
   lv_obj_set_flex_align(words, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

   Object name = lv_label_create(words);
   lv_label_set_text(name, title);
   lv_obj_set_style_text_color(name, lv_color_hex(token::kInk), 0);

   Object where = lv_label_create(words);
   lv_label_set_text(where, second);
   lv_obj_set_style_text_color(where, lv_color_hex(token::kFaint), 0);
   if (typography().small != nullptr) {
      lv_obj_set_style_text_font(where, typography().small, 0);
   }
   return group;
}

Object MediaButtons(Object parent, const Panel& panel, std::initializer_list<const lv_image_dsc_t*> symbols,
                    int accent) {
   Object group = lv_obj_create(parent);
   MakePlain(group);
   lv_obj_set_size(group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
   lv_obj_set_flex_flow(group, LV_FLEX_FLOW_ROW);
   lv_obj_set_flex_align(group, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
   lv_obj_set_style_pad_column(group, panel(token::kInset).value, 0);

   int index = 0;
   for (const lv_image_dsc_t* symbol : symbols) {
      IconButton(group, panel, symbol, panel(token::kMediaKey), index == accent);
      index += 1;
   }
   return group;
}

namespace {

/// Where a slider keeps the label that carries its value, so it can be written
/// afresh without the slider being built again.
constexpr std::uint32_t kSliderValueIndex = 1;

/**
 * Puts a slider's handle where its value now stands.
 *
 * The handle is a shape of this design laid over the library's own control
 * rather than a property of it, so nothing moves it unless it is told to. It
 * follows the value rather than the finger, which is also what puts it in the
 * right place when a value is set from somewhere other than a drag.
 */
void PlaceHandle(lv_event_t* event) {
   Object control = static_cast<Object>(lv_event_get_target(event));
   Object handle = static_cast<Object>(lv_event_get_user_data(event));

   const std::int32_t across = lv_obj_get_width(control) - lv_obj_get_width(handle);
   if (across <= 0) {
      return;
   }

   // The handle stands inside the bar at both ends rather than hanging over
   // them, so the whole of it is on the glass wherever the value is.
   const std::int32_t range = lv_slider_get_max_value(control) - lv_slider_get_min_value(control);
   const std::int32_t along = range == 0 ? 0 : (lv_slider_get_value(control) - lv_slider_get_min_value(control));
   std::int32_t at = range == 0 ? 0 : along * across / range;

   // On the grid, like every other edge. A handle that landed between two
   // points of it would sit half a point beside the bar it stands on, which is
   // exactly what the grid is there to prevent.
   at -= at % token::kGrid.value;

   lv_obj_align_to(handle, control, LV_ALIGN_LEFT_MID, at, 0);
}

/// The whole of a share, as the graphics library counts a slider's range. It
/// works in whole numbers, so a share from 0 to 1 is carried as thousandths:
/// on a bar 700 points long, a percent is seven points and a reader sees the
/// step.
constexpr std::int32_t kShareSteps = 1000;

}  // namespace

Object Bar(Object parent, const Panel& panel, float filled) {
   const std::int32_t height = panel(token::kTrack).value;

   Object track = lv_obj_create(parent);
   MakePlain(track);
   lv_obj_set_width(track, lv_pct(100));
   lv_obj_set_height(track, height);

   // The empty part stands in the raised ink rather than in the surface or the
   // line. A bar lies on two different grounds, the screen itself and the
   // lighter ground of a card, and only this one is told apart from both.
   lv_obj_set_style_bg_color(track, lv_color_hex(token::kRaised), 0);
   lv_obj_set_style_bg_opa(track, LV_OPA_COVER, 0);
   lv_obj_set_style_radius(track, height / 2, 0);

   Object full = lv_obj_create(track);
   MakePlain(full);
   lv_obj_set_height(full, height);
   lv_obj_set_style_bg_color(full, lv_color_hex(Accent()), 0);
   lv_obj_set_style_bg_opa(full, LV_OPA_COVER, 0);
   lv_obj_set_style_radius(full, height / 2, 0);
   lv_obj_align(full, LV_ALIGN_LEFT_MID, 0, 0);

   const float held = filled < 0.0f ? 0.0f : (filled > 1.0f ? 1.0f : filled);
   lv_obj_set_width(full, lv_pct(static_cast<std::int32_t>(held * 100)));

   // The two are one shape rather than two surfaces beside each other, so the
   // inner corner is the outer one and the check that compares them agrees.
   Screen::MarkAsBandPart(full);
   return track;
}

Object Slider(Object parent, const Panel& panel, const char* label, const char* value, float filled) {
   const std::int32_t knob = panel(token::kSliderKnob).value;
   const std::int32_t height = panel(token::kTrack).value;

   Object group = lv_obj_create(parent);
   MakePlain(group);
   lv_obj_set_width(group, lv_pct(100));
   lv_obj_set_height(group, LV_SIZE_CONTENT);

   Object name = lv_label_create(group);
   lv_label_set_text(name, label);
   lv_obj_set_style_text_color(name, lv_color_hex(token::kMuted), 0);
   lv_obj_align(name, LV_ALIGN_TOP_LEFT, 0, 0);

   Object says = lv_label_create(group);
   lv_label_set_text(says, value == nullptr ? "" : value);
   lv_obj_set_style_text_color(says, lv_color_hex(token::kInk), 0);
   if (typography().strong != nullptr) {
      lv_obj_set_style_text_font(says, typography().strong, 0);
   }
   lv_obj_align(says, LV_ALIGN_TOP_RIGHT, 0, 0);

   // The library's own slider, because what makes one is following a finger
   // across the glass and that is the part it already does. What Caliper
   // decides is how it looks and how large the handle is.
   Object control = lv_slider_create(group);
   lv_obj_set_width(control, lv_pct(100));
   lv_obj_set_height(control, height);
   lv_obj_align(control, LV_ALIGN_TOP_LEFT, 0, panel.TypeSize(token::kBody).value + panel(token::kSliderLabel).value);

   lv_slider_set_range(control, 0, kShareSteps);
   lv_slider_set_value(control, static_cast<std::int32_t>(filled * kShareSteps), LV_ANIM_OFF);

   lv_obj_set_style_bg_color(control, lv_color_hex(token::kRaised), LV_PART_MAIN);
   lv_obj_set_style_bg_opa(control, LV_OPA_COVER, LV_PART_MAIN);
   lv_obj_set_style_radius(control, height / 2, LV_PART_MAIN);

   lv_obj_set_style_bg_color(control, lv_color_hex(Accent()), LV_PART_INDICATOR);
   lv_obj_set_style_bg_opa(control, LV_OPA_COVER, LV_PART_INDICATOR);
   lv_obj_set_style_radius(control, height / 2, LV_PART_INDICATOR);

   // The library's own handle is taken away and one of this design's shapes is
   // put in its place. A stencil set on the knob part is not used for it, so
   // the handle came out as a plain rectangle: what draws a squircle here is
   // the same `Squircle` every other shape of this design goes through.
   lv_obj_set_style_bg_opa(control, LV_OPA_TRANSP, LV_PART_KNOB);

   Object handle = Squircle(group, knob, token::kInk);

   // It follows the value rather than the finger. The library moves the value
   // whilst a finger drags, and this is put where that value now is, which also
   // puts it in the right place when a value is set from somewhere else.
   lv_obj_add_flag(handle, LV_OBJ_FLAG_IGNORE_LAYOUT);
   lv_obj_add_event_cb(control, PlaceHandle, LV_EVENT_VALUE_CHANGED, handle);
   lv_obj_add_event_cb(control, PlaceHandle, LV_EVENT_SIZE_CHANGED, handle);

   // What may be hit reaches a fingertip in height, whilst the bar stays as

   // What may be hit reaches a fingertip in height, whilst the bar stays as
   // narrow as the design draws it. Without this the control is sixteen points
   // tall to a finger as well as to the eye, which is a third of what a hand
   // needs.
   lv_obj_set_ext_click_area(control, (panel(token::kFingertip).value - height) / 2);

   // The group reports what the slider does, so a caller listens to the one
   // object it was given rather than reaching inside for the control.
   lv_obj_add_event_cb(
       control,
       [](lv_event_t* event) {
          Object touched = static_cast<Object>(lv_event_get_target(event));
          lv_obj_send_event(lv_obj_get_parent(touched), LV_EVENT_VALUE_CHANGED, nullptr);
       },
       LV_EVENT_VALUE_CHANGED, nullptr);

   return group;
}

float SliderAt(Object slider) {
   for (std::uint32_t index = 0; index < lv_obj_get_child_count(slider); ++index) {
      Object child = lv_obj_get_child(slider, index);
      if (lv_obj_check_type(child, &lv_slider_class)) {
         return static_cast<float>(lv_slider_get_value(child)) / kShareSteps;
      }
   }
   return 0.0f;
}

void SetSliderValue(Object slider, const char* value) {
   Object says = lv_obj_get_child(slider, kSliderValueIndex);
   if (says != nullptr && lv_obj_check_type(says, &lv_label_class)) {
      lv_label_set_text(says, value);
   }
}

Object Spacer(Object parent, Point size) {
   Object gap = lv_obj_create(parent);
   MakePlain(gap);

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
