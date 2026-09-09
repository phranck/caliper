#ifndef CALIPER_THEME_H_
#define CALIPER_THEME_H_

#include <cstdint>

#include "caliper/panel.h"
#include "lvgl.h"

namespace cal {

/**
 * The typefaces a screen is set in, one per grade of the scale.
 *
 * They belong to the product and not to this library: a library that ships a
 * typeface has decided what the product looks like. Caliper says how large a
 * grade is and the product says in what.
 */
struct Typography {
   /// Ordinary text, and what anything unspecified falls back to.
   const lv_font_t* body = nullptr;

   /// A heading, larger, for what one is looking at.
   const lv_font_t* heading = nullptr;

   /// The smallest grade, for a value beside a name.
   const lv_font_t* small = nullptr;

   /// The same size as body in a heavier cut, for the one or two words a
   /// control carries. A label on a button is read at a glance rather than
   /// line by line, and the heavier cut is what makes that work at this size.
   const lv_font_t* strong = nullptr;

   /// What the status bar is set in, at its own grade. It may be a different
   /// face: the bar carries figures and single words at the very top of the
   /// screen, where a condensed face is tight and a normal one reads at a
   /// glance. Null falls back to `small`.
   const lv_font_t* status = nullptr;

   /// The smallest grade, which is what a sidebar item is labelled in. It falls
   /// back to the status face where none is given.
   const lv_font_t* caption = nullptr;

   /// What is typed into a field. The grade a key of the keyboard is set in,
   /// in the plain cut: it is read back rather than announced, and it stands
   /// under the question it answers. Null falls back to `body`.
   const lv_font_t* field = nullptr;

   /// What a key of the keyboard says, at the cap height `keyboard_char`
   /// states. Null falls back to `body`.
   const lv_font_t* key = nullptr;

   /// What shift would make of a key, set over the character at the cap height
   /// `keyboard_shifted` states. It is smaller than anything else this design
   /// sets, because it is what one looks for rather than what one reads. Null
   /// falls back to `caption`.
   const lv_font_t* key_shifted = nullptr;
};

/**
 * The accent in force.
 *
 * One colour on a screen of greys, and the one a person picks, so it is not
 * known until they have. It is read at the moment a thing is built rather than
 * at the moment it is drawn, which means a change reaches a screen that is
 * built again afterwards. That is how the language already works, and for the
 * same reason: what changes with it is not only a colour but which screen
 * somebody is looking at.
 *
 * @returns The accent, as the design's own until somebody sets another.
 */
std::uint32_t Accent();

/// What stands on the accent, which is white until the accent is light enough
/// that white stops being readable on it.
std::uint32_t AccentInk();

/// The accent dimmed towards the ground, for what is present but not now.
std::uint32_t AccentDim();

/**
 * Sets the accent, and with it the two that follow from it.
 *
 * The ink and the dimmed version are worked out here rather than at every
 * drawing, because they follow from one colour that changes rarely and the
 * arithmetic behind the ink is not cheap.
 *
 * @param colour The accent from now on.
 */
void SetAccent(std::uint32_t colour);

/**
 * Registers Caliper's look with the graphics library, for one display.
 *
 * It is a theme rather than a second styling world beside the library's own.
 * Anything taken straight from LVGL is styled by it too, so a widget nobody
 * wrapped still belongs to the same design instead of standing beside it.
 *
 * The values come from the generated header, so the device and the documents
 * that describe it cannot carry different ones.
 *
 * @param display The display to style.
 * @param panel The panel it draws on, which turns the physical values into
 *              points.
 * @param fonts The typefaces to set text in.
 */
void InstallTheme(lv_display_t* display, const Panel& panel, const Typography& fonts);

/// The typefaces the theme was installed with, for a component that sets text.
const Typography& typography();

}  // namespace cal

#endif  // CALIPER_THEME_H_
