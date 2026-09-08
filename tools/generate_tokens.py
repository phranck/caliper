#!/usr/bin/env python3
"""Writes the design values into the two forms that consume them.

`tokens/caliper.toml` holds them once. This turns that file into a C++ header
for the device and a Python module for whatever draws the documents, and both
outputs say at the top that they are generated. Neither is ever edited, which
is the whole mechanism that keeps a device and the papers describing it from
drifting apart.

The generator runs before the compiler rather than by hand. Changing the values
and building gives the new header; changing the header and building gives the
old one back.

Run it with no arguments to write both files in their usual places:

    python3 tools/generate_tokens.py
"""

from __future__ import annotations

import re
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "tokens" / "caliper.toml"
HEADER = ROOT / "include" / "caliper" / "tokens.h"
MODULE = ROOT / "tools" / "caliper_tokens.py"

#: A value written as a quantity, such as "3.27 mm". Anything else in a group
#: that holds quantities is a mistake rather than a plain number, so it is
#: reported instead of being passed through.
QUANTITY = re.compile(r"^\s*(-?\d+(?:\.\d+)?)\s*mm\s*$")

#: Groups whose entries are all millimetre quantities.
MILLIMETRE_GROUPS = ("spacing", "touch", "layout")

#: Where the reading distances live, mixed in with the ratios that are plain
#: numbers, so this group is read entry by entry.
TYPE_GROUP = "type"
TYPE_RATIOS = ("cap_angle_mrad", "cap_ratio", "x_height_ratio", "descender_ratio")


def millimetres(name: str, value: str) -> float:
    """Reads a quantity such as "3.27 mm" and returns the number in it.

    @param name The key it was written under, so a failure names the culprit.
    @param value The value as it stands in the file.
    @returns The size in millimetres.
    """
    match = QUANTITY.match(value)
    if match is None:
        raise SystemExit(f"{SOURCE.name}: {name} is not a millimetre quantity: {value!r}")
    return float(match.group(1))


def read_tokens() -> dict:
    """Reads the source file and converts every quantity into a number.

    @returns The values, grouped as they are in the file, with millimetres as
             floats and colours and ratios untouched.
    """
    with SOURCE.open("rb") as handle:
        raw = tomllib.load(handle)

    tokens: dict = {}

    for group in MILLIMETRE_GROUPS:
        tokens[group] = {name: millimetres(f"{group}.{name}", value)
                         for name, value in raw[group].items()}

    tokens[TYPE_GROUP] = {}
    for name, value in raw[TYPE_GROUP].items():
        if name in TYPE_RATIOS:
            tokens[TYPE_GROUP][name] = float(value)
        else:
            tokens[TYPE_GROUP][name] = millimetres(f"type.{name}", value)

    tokens["radius"] = {}
    for name, value in raw["radius"].items():
        if isinstance(value, str):
            tokens["radius"][name] = millimetres(f"radius.{name}", value)
        else:
            tokens["radius"][name] = float(value)

    tokens["grid"] = {name: int(value) for name, value in raw["grid"].items()}
    tokens["colour"] = dict(raw["colour"])
    return tokens


def cpp_float(value: float) -> str:
    """Formats a number as a C++ float literal.

    A literal always carries a decimal point, because `9f` is not one and only
    fails at the compiler, long after this file has been written.

    @param value The number.
    @returns The literal, such as "9.0f".
    """
    text = f"{value:.6g}"
    if "." not in text and "e" not in text:
        text += ".0"
    return text + "f"


def cpp_identifier(name: str) -> str:
    """Turns a key from the file into the name the Google guide asks for.

    A constant is written with a leading `k` and then mixed case, so `line-gap`
    in the file becomes `kLineGap` in the header. The key stays as it is: the
    file is read by people rather than by a compiler.
    """
    return "k" + "".join(part.capitalize() for part in name.replace("-", "_").split("_"))


def write_header(tokens: dict) -> None:
    """Writes the C++ header the firmware includes.

    Every size carries its unit as a type rather than being a bare number, so a
    millimetre cannot be passed where points are expected without going through
    a panel. That is the reason this library exists, and generating the header
    is what makes it hold for the design values as well as for the code.
    """
    lines = [
        "// Generated from tokens/caliper.toml by tools/generate_tokens.py.",
        "// Do not edit: the next build overwrites this file.",
        "",
        "#ifndef CALIPER_TOKENS_H_",
        "#define CALIPER_TOKENS_H_",
        "",
        "#include <cstdint>",
        "",
        '#include "caliper/units.h"',
        "",
        "namespace cal::token {",
        "",
        "// Spacings. Millimetres, because a margin answers to an eye and a finger",
        "// rather than to a pixel count.",
    ]

    for name, value in tokens["spacing"].items():
        lines.append(f"inline constexpr Millimeter {cpp_identifier(name)}{{{cpp_float(value)}}};")

    lines += ["",
              "// The heights that decide where the content area begins and ends:",
              "// StatusBar and Header above it, and the room reserved below it."]
    for name, value in tokens["layout"].items():
        lines.append(
            f"inline constexpr Millimeter {cpp_identifier('band_' + name)}{{{cpp_float(value)}}};")

    lines += ["",
              "// What a hand needs. The floor every touch target is measured against."]
    for name, value in tokens["touch"].items():
        lines.append(
            f"inline constexpr Millimeter {cpp_identifier(name)}{{{cpp_float(value)}}};")

    lines += ["",
              "// The type scale as a rule: every grade is a cap height seen under one",
              "// angle from the distance it is meant to be read at."]
    for name in TYPE_RATIOS:
        lines.append(
            f"inline constexpr float {cpp_identifier(name)} = "
            f"{cpp_float(tokens['type'][name])};")
    for name, value in tokens["type"].items():
        if name not in TYPE_RATIOS:
            lines.append(f"inline constexpr Reading {cpp_identifier(name)}{{{cpp_float(value)}}};")

    lines += ["",
              "// Only the outer corners. Anything nested is computed by inner_radius",
              "// below, because two numbers drift into a pinched corner."]
    for name, value in tokens["radius"].items():
        if name.startswith("squircle"):
            lines.append(
                f"inline constexpr float {cpp_identifier(name)} = {cpp_float(value)};")
        else:
            lines.append(
                f"inline constexpr Millimeter {cpp_identifier('radius_' + name)}"
                f"{{{cpp_float(value)}}};")

    lines += [
        "",
        "/// A corner inside another is the one around it less the space between",
        "/// them. Stated as a rule rather than as a second number, so the two",
        "/// cannot fall out of step when either moves.",
        "constexpr Millimeter InnerRadius(Millimeter outer, Millimeter gap = kInset) {",
        "   return Millimeter{outer.value - gap.value};",
        "}",
        "",
        "// The corner inside the card's own, so a button matches whatever surface",
        "// it stands on rather than carrying a second figure that can drift from it.",
        "inline constexpr Millimeter kRadiusButton = InnerRadius(kRadiusCard, kInset);",
        "",
        "// The grid every edge lands on, in whole pixels.",
        f"inline constexpr Point kGrid{{{tokens['grid']['unit']}}};",
        "",
        "// The palette. A colour has no measurement, so these pass through as they",
        "// were written.",
    ]

    for name, value in tokens["colour"].items():
        lines.append(
            f"inline constexpr std::uint32_t {cpp_identifier(name)} = 0x{value.lstrip('#')};")

    lines += ["", "}  // namespace cal::token", "",
          "#endif  // CALIPER_TOKENS_H_", ""]

    HEADER.parent.mkdir(parents=True, exist_ok=True)
    HEADER.write_text("\n".join(lines), encoding="utf-8")


def write_module(tokens: dict) -> None:
    """Writes the Python module the paper generator reads.

    It carries the same values in millimetres and the arithmetic that turns them
    into points, because a document is drawn for a panel just as a device is.
    `Papers/level/sources/build_screens.py` imports this module directly, so a
    key removed or renamed here breaks a build outside this repository, with
    nothing in it to say so.
    """
    lines = [
        '"""The design values, generated from tokens/caliper.toml.',
        "",
        "Do not edit: tools/generate_tokens.py overwrites this file. Change",
        "tokens/caliper.toml and run it again.",
        '"""',
        "",
        "SPACING_MM = {",
    ]
    for name, value in tokens["spacing"].items():
        lines.append(f'    "{name}": {value},')
    lines += ["}", "", "LAYOUT_MM = {"]
    for name, value in tokens["layout"].items():
        lines.append(f'    "{name}": {value},')
    lines += ["}", "", "TOUCH_MM = {"]
    for name, value in tokens["touch"].items():
        lines.append(f'    "{name}": {value},')
    lines += ["}", "", "READING_MM = {"]
    for name, value in tokens["type"].items():
        if name not in TYPE_RATIOS:
            lines.append(f'    "{name}": {value},')
    lines += ["}", ""]
    for name in TYPE_RATIOS:
        lines.append(f"{name.upper()} = {tokens['type'][name]}")
    lines += ["", "RADIUS_MM = {"]
    for name, value in tokens["radius"].items():
        if not name.startswith("squircle"):
            lines.append(f'    "{name}": {value},')
    lines += ["}", ""]
    for name, value in tokens["radius"].items():
        if name.startswith("squircle"):
            lines.append(f"{name.upper()} = {value}")
    lines += [""]
    for name, value in tokens["grid"].items():
        lines.append(f"GRID_{name.upper()} = {value}")
    lines += ["", "COLOURS = {"]
    for name, value in tokens["colour"].items():
        lines.append(f'    "{name.replace("_", "-")}": "{value}",')
    lines += [
        "}",
        "",
        "",
        "def _linear(channel: int) -> float:",
        '    """One channel of a colour, on the scale light is actually measured in.',
        "",
        "    A colour as it is written is not proportional to the light coming off the",
        "    glass, and everything about contrast is. Caliper carries the same",
        "    arithmetic, because a drawing and a device that disagreed about which ink",
        "    stands on the accent would disagree about what the product looks like.",
        '    """',
        "    share = channel / 255",
        "    return share / 12.92 if share <= 0.04045 else ((share + 0.055) / 1.055) ** 2.4",
        "",
        "",
        "def luminance(colour: str) -> float:",
        '    """How much light a colour gives off, from 0 for black to 1 for white."""',
        "    value = int(colour.lstrip('#'), 16)",
        "    return (0.2126 * _linear((value >> 16) & 0xFF)",
        "            + 0.7152 * _linear((value >> 8) & 0xFF)",
        "            + 0.0722 * _linear(value & 0xFF))",
        "",
        "",
        "def ink_on(colour: str) -> str:",
        '    """What a large word is set in when it stands on a colour.',
        "",
        "    White whilst it still reaches three to one, which is the floor a word set",
        "    this large holds, and the ground's own colour once it does not. The",
        "    design's accent lands at 3.03, which is why white stands on it.",
        '    """',
        "    return \"#ffffff\" if 1.05 / (luminance(colour) + 0.05) >= 3.0 else COLOURS[\"bg\"]",
        "",
        "",
        "def dimmed_accent(colour: str) -> str:",
        '    """A colour taken most of the way to the ground, for what is not now."""',
        "    value = int(colour.lstrip('#'), 16)",
        "    ground = int(COLOURS['bg'].lstrip('#'), 16)",
        "    mixed = 0",
        "    for shift in (16, 8, 0):",
        "        here = (value >> shift) & 0xFF",
        "        there = (ground >> shift) & 0xFF",
        "        mixed |= int(here + (there - here) * 0.55 + 0.5) << shift",
        "    return '#%06x' % mixed",
        "",
        "",
        "# The two that follow from the accent rather than standing beside it. Written",
        "# into the palette so a drawing asks for them the way it asks for any colour.",
        'COLOURS["accent-ink"] = ink_on(COLOURS["accent"])',
        'COLOURS["accent-dim"] = dimmed_accent(COLOURS["accent"])',
        "",
        "",
        "def _rounded(value: float) -> int:",
        '    """Rounds to the nearest whole number, halves away from zero.',
        "",
        "    Caliper's C++ side rounds this way, and the two have to agree: a value",
        "    converted here and the same value converted there would otherwise land",
        "    on different pixels at exactly one half. Python's own round breaks ties",
        "    towards even, which is the difference.",
        '    """',
        "    return int(value - 0.5) if value < 0 else int(value + 0.5)",
        "",
        "",
        "def points(millimetres: float, points_per_mm: float) -> int:",
        '    """Converts a physical size into points on a given panel.',
        "",
        "    @param millimetres The size on the glass.",
        "    @param points_per_mm The density of the panel it is drawn on.",
        "    @returns The size in whole points.",
        '    """',
        "    return _rounded(millimetres * points_per_mm)",
        "",
        "",
        "def type_size(reading_mm: float, points_per_mm: float) -> int:",
        '    """The type size a cap height needs to hold its angle at a distance.',
        "",
        "    A capital seen from further away has to be taller to appear the same",
        "    size, and the angle it appears under is what legibility follows. This",
        "    is why a grade states a distance rather than a number of points.",
        "",
        "    @param reading_mm How far away the text is meant to be read from.",
        "    @param points_per_mm The density of the panel it is drawn on.",
        "    @returns The type size in whole points.",
        '    """',
        "    cap_height_mm = CAP_ANGLE_MRAD / 1000.0 * reading_mm",
        "    return _rounded(cap_height_mm * points_per_mm / CAP_RATIO)",
        "",
        "",
        "def inner_radius(outer_mm: float, gap_mm: float | None = None) -> float:",
        '    """A corner inside another is the one around it less the space between.',
        "",
        "    Stated as a rule rather than as a second number, so the two cannot fall",
        "    out of step when either moves.",
        "",
        "    @param outer_mm The corner around it.",
        "    @param gap_mm The space between the two, the standard inset by default.",
        "    @returns The inner corner in millimetres.",
        '    """',
        '    return outer_mm - (SPACING_MM["inset"] if gap_mm is None else gap_mm)',
        "",
    ]

    MODULE.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    """Writes both outputs and says what came out."""
    tokens = read_tokens()
    write_header(tokens)
    write_module(tokens)

    counted = sum(len(group) for group in tokens.values())
    print(f"{counted} values from {SOURCE.name}")
    print(f"  {HEADER.relative_to(ROOT)}")
    print(f"  {MODULE.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
