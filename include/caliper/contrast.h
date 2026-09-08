#ifndef CALIPER_CONTRAST_H_
#define CALIPER_CONTRAST_H_

#include <cstdint>

/**
 * Which ink stands on which ground, worked out rather than chosen.
 *
 * This is here rather than in the theme because it touches neither the graphics
 * library nor a panel: it is arithmetic about light, and it can therefore be
 * checked on a machine with nothing attached, which is where a mistake in it
 * has to be caught. On the glass it does not look wrong, it looks dim.
 */
namespace cal {

/**
 * How much light a colour gives off.
 *
 * A colour as it is written is not proportional to the light coming off the
 * glass, and everything about contrast is. Skipping that step is how a pair
 * that reads as fine on paper turns out unreadable on the panel.
 *
 * @param colour The colour, as `0xRRGGBB`.
 * @returns Its luminance, from 0 for black to 1 for white.
 */
float Luminance(std::uint32_t colour);

/**
 * How far apart two colours stand.
 *
 * @param one A colour.
 * @param other The other.
 * @returns Their ratio, from 1 for two of the same to 21 for black on white.
 */
float Contrast(std::uint32_t one, std::uint32_t other);

/// The floor a word set at the size a button uses holds against its ground.
/// Smaller text needs more, and nothing on these screens sets a word this large
/// anywhere else.
inline constexpr float kLargeTextContrast = 3.0f;

/**
 * What a large word is set in when it stands on a colour.
 *
 * White whilst it still reaches the floor above, and the ground's own colour
 * once it does not. Between the two there is a stretch where either would do,
 * and white is kept there because a filled control with white on it is what the
 * design draws. Its own accent lands at 3.03, which is why white stands on it.
 *
 * @param colour The colour the word stands on.
 * @returns The colour the word is set in.
 */
std::uint32_t InkOn(std::uint32_t colour);

/**
 * A colour taken most of the way to the ground, for what is present but not
 * now.
 *
 * Written as a rule rather than as a second colour, because a second colour
 * would be right for one accent and wrong for the other fifteen.
 *
 * @param colour The colour to dim.
 * @returns It, dimmed.
 */
std::uint32_t DimmedAccent(std::uint32_t colour);

}  // namespace cal

#endif  // CALIPER_CONTRAST_H_
