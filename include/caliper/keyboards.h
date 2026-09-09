// Generated from tokens/keyboards.json by tools/generate_keyboards.py.
// Do not edit: the next build overwrites this file.

#ifndef CALIPER_KEYBOARDS_H_
#define CALIPER_KEYBOARDS_H_

#include <cstdint>

#include "caliper/units.h"

// The sizes and the colours stand in the namespace every design value
// stands in, under the names they had whilst they lived in caliper.toml,
// so moving them here changed nothing at the places that read them.
namespace cal::token {

inline constexpr Millimeter kKeyboardKey{7.1f};
inline constexpr Millimeter kKeyboardKeyHigh{6.8f};
inline constexpr Millimeter kKeyboardGap{1.09f};
inline constexpr Millimeter kKeyboardEdge{1.36f};
inline constexpr Millimeter kKeyboardSymbols{11.4f};
inline constexpr Millimeter kKeyboardConfirm{20.9f};
inline constexpr Millimeter kKeyboardEscape{12.0f};
inline constexpr Millimeter kKeyboardChar{2.18f};
inline constexpr Millimeter kKeyboardShifted{1.63f};
inline constexpr Millimeter kKeyboardStack{1.09f};
inline constexpr Millimeter kKeyboardMark{2.2f};
inline constexpr Millimeter kKeyboardCorner{1.1f};

inline constexpr std::uint32_t kKeyTray = 0x1b1b1b;
inline constexpr std::uint32_t kKeyFiller = 0x282828;
inline constexpr std::uint32_t kKeyQuiet = 0x3a3a3a;
inline constexpr std::uint32_t kKey = 0x626262;
inline constexpr std::uint32_t kKeyInk = 0xececec;
inline constexpr std::uint32_t kKeyShifted = 0xc0c0c0;
inline constexpr std::uint32_t kKeyMark = 0xc0c0c0;

}  // namespace cal::token

// The layouts stand apart from them, because a keyboard is the one design
// value that is a shape rather than a number.
namespace cal::keyboards {

/// How many rows of characters a keyboard has.
inline constexpr int kRowCount = 4;

/// How many keys the rows hold between them, which is how many labels a
/// keyboard needs however few characters a layer puts on them.
inline constexpr int kCharacterKeys = 47;

/// What stands at the end of a row.
enum class End { kNothing, kFiller, kShift, kBackspace };

/**
 * One country's first layer.
 *
 * `rows` is what the keys say unshifted, and `shifted` says what shift makes
 * of each of them, position by position, with a space where it makes nothing
 * new. A letter has no entry there, because shift raises the whole row.
 *
 * `symbols` holds what that country's layout cannot reach, and nothing else.
 * A tablet keyboard needs two layers of them because its letters carry no
 * punctuation at all; this one carries a whole national layout with a second
 * case, so it reaches nearly everything already. A character that can be
 * typed in two places, and in two different places, is the one thing a
 * keyboard must not do, so what is reachable does not appear there.
 *
 * One row per kind, so a row means something and a character is looked for
 * rather than hunted. Each row is centred, and a space leaves a key alone
 * whilst keeping its place, which is what centres it and what lets a row end
 * short without the rest closing up. The fourth row carries nothing, because
 * three kinds hold everything any of these languages is missing and a row
 * that empties rather than vanishes keeps the keyboard the height it had.
 */
struct Alphabet {
   const char* rows[kRowCount];
   const char* shifted[kRowCount];
   const char* symbols[kRowCount];
   End ends[kRowCount][2];
   Millimeter lead[kRowCount];
};

/// Deutsch, the layout of the keyboard its country writes on.
inline constexpr Alphabet kGerman{
    {"^1234567890ß", "qwertzuiopü+", "asdfghjklöä#", "<yxcvbnm,.-"},
    {"°!\"§$%&/()=?", "           *", "           '", ">       ;:_"},
    {" ~`[]{}\\|@", "    €£¥", "   «»¡¿…", ""},
    {{End::kNothing, End::kBackspace}, {End::kFiller, End::kFiller}, {End::kFiller, End::kFiller}, {End::kShift, End::kFiller}},
    {Millimeter{0.0f}, Millimeter{2.72f}, Millimeter{4.9f}, Millimeter{9.8f}}};

/// English, the layout of the keyboard its country writes on.
inline constexpr Alphabet kEnglish{
    {"§1234567890-", "qwertyuiop[]", "asdfghjkl;'\\", "`zxcvbnm,./"},
    {"±!@#$%^&*()_", "          {}", "         :\"|", "~       <>?"},
    {"     =+", "    €£¥", "   °«»¡¿…", ""},
    {{End::kNothing, End::kBackspace}, {End::kFiller, End::kFiller}, {End::kFiller, End::kFiller}, {End::kShift, End::kFiller}},
    {Millimeter{0.0f}, Millimeter{2.72f}, Millimeter{4.9f}, Millimeter{9.8f}}};

/// Français, the layout of the keyboard its country writes on.
inline constexpr Alphabet kFrench{
    {"@&é\"'(§è!çà)", "azertyuiop^$", "qsdfghjklmù`", "<wxcvbn,;:="},
    {"#1234567890°", "          ¨*", "          %£", ">       ./+"},
    {" ~_[]{}\\|-?", "     €¥", "   «»¡¿…", ""},
    {{End::kNothing, End::kBackspace}, {End::kFiller, End::kFiller}, {End::kFiller, End::kFiller}, {End::kShift, End::kFiller}},
    {Millimeter{0.0f}, Millimeter{2.72f}, Millimeter{4.9f}, Millimeter{9.8f}}};

/// Español, the layout of the keyboard its country writes on.
inline constexpr Alphabet kSpanish{
    {"º1234567890'", "qwertyuiop`+", "asdfghjklñ´ç", "<zxcvbnm,.-"},
    {"ª!\"·$%&/()=?", "          ^*", "         Ñ¨Ç", ">       ;:_"},
    {" ~[]{}\\|@#", "    €£¥", "  §°«»¡¿…", ""},
    {{End::kNothing, End::kBackspace}, {End::kFiller, End::kFiller}, {End::kFiller, End::kFiller}, {End::kShift, End::kFiller}},
    {Millimeter{0.0f}, Millimeter{2.72f}, Millimeter{4.9f}, Millimeter{9.8f}}};

/// Italiano, the layout of the keyboard its country writes on.
inline constexpr Alphabet kItalian{
    {"\\1234567890'", "qwertyuiopè+", "asdfghjklòàù", "<zxcvbnm,.-"},
    {"|!\"£$%&/()=?", "          é*", "         ç°", ">       ;:_"},
    {" ~`^[]{}@#", "     €¥", "   §«»¡¿…", ""},
    {{End::kNothing, End::kBackspace}, {End::kFiller, End::kFiller}, {End::kFiller, End::kFiller}, {End::kShift, End::kFiller}},
    {Millimeter{0.0f}, Millimeter{2.72f}, Millimeter{4.9f}, Millimeter{9.8f}}};

}  // namespace cal::keyboards

#endif  // CALIPER_KEYBOARDS_H_
