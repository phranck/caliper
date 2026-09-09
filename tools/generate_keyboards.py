#!/usr/bin/env python3
"""Turns the one keyboard file into the constants the firmware is built from.

`tokens/keyboards.json` holds everything the keyboard is: its sizes, its
colours, and each country's first layer. This writes `include/caliper/keyboards.h`
from it, and LEVEL's layout page reads the very same file, so no figure is ever
carried from one place to another by a person or by anything else.

The symbol rows are derived rather than decided. They are exactly what that
country's own layout cannot reach, which is a rule and not a judgement, and it
had been worked out by hand three times before this existed. The generator
works them out and writes them back into the file, so a row edited there by
hand is put right at the next build rather than shipping as a keyboard that can
type the same character from two different keys.

    python3 tools/generate_keyboards.py
"""

from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "tokens" / "keyboards.json"
HEADER = ROOT / "include" / "caliper" / "keyboards.h"

#: A value written as a quantity, such as "7.1 mm".
QUANTITY = re.compile(r"^\s*(-?\d+(?:\.\d+)?)\s*mm\s*$")

#: How wide each row of characters is, in keys. The rows are what settle the
#: keyboard's shape, so a layout that disagrees is a mistake in the file rather
#: than a keyboard with a short row.
WIDTHS = (12, 12, 12, 11)

#: What may stand at the end of a row, and what the firmware calls it.
ENDS = {"none": "kNothing", "filler": "kFiller", "shift": "kShift", "backspace": "kBackspace"}

#: The order the groups of extras are laid out in, one to a row.
GROUPS = ("punctuation", "money", "typography")


def millimetres(name: str, value: str) -> float:
    """Reads a quantity such as "7.1 mm" and returns the number in it.

    @param name The key it was written under, so a failure names the culprit.
    @param value The value as it stands in the file.
    @returns The size in millimetres.
    """
    match = QUANTITY.match(value)
    if match is None:
        raise SystemExit(f"{SOURCE.name}: {name} is not a millimetre quantity: {value!r}")
    return float(match.group(1))


def reaches(layout: dict) -> set[str]:
    """Every character this layout can already type.

    The characters on its keys, what shift makes of each of them, and the
    capitals shift makes of its letters. A letter with no capital of its own,
    such as the sharp s, is not counted as one: shift leaves it alone, so it
    reaches only itself.

    @param layout One country's entry in the file.
    @returns The characters, as a set.
    """
    found: set[str] = set()
    for line in layout["rows"]:
        for character in line:
            if character == " ":
                continue
            found.add(character)
            capital = character.upper()
            if len(capital) == 1:
                found.add(capital)
    for line in layout["shifted"]:
        found.update(character for character in line if character != " ")
    return found


def symbols_for(layout: dict, extras: dict) -> list[str]:
    """Works out the symbol rows for one layout.

    One row per group, so that a row means something: the punctuation a
    password wants, then the money, then the marks a language sets its sentences
    with. Each row holds only what this layout cannot reach already, and each is
    centred in its keys with spaces, because a space leaves a key alone whilst
    keeping its place.

    The fourth row carries nothing. Three groups hold everything any of these
    languages is missing, and a row that empties rather than vanishes keeps the
    keyboard the height it had.

    @param layout One country's entry in the file.
    @param extras The groups of characters, in the order they are laid out.
    @returns Four rows, the last of them empty.
    """
    within = reaches(layout)
    rows = []
    for index, group in enumerate(GROUPS):
        taken = [one for one in extras[group] if one not in within]
        if len(taken) > WIDTHS[index]:
            raise SystemExit(f"{SOURCE.name}: {layout['named']} has {len(taken)} {group} "
                             f"for a row of {WIDTHS[index]} keys")
        lead = (WIDTHS[index] - len(taken)) // 2
        rows.append(" " * lead + "".join(taken))
    rows.append("")
    return rows


def cpp_string(text: str) -> str:
    """A C++ string literal holding exactly this text."""
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def cpp_float(value: float) -> str:
    """A C++ float literal, always with a decimal point, because `9f` is not one."""
    text = f"{value:.6g}"
    if "." not in text and "e" not in text:
        text += ".0"
    return text + "f"


def write_header(data: dict) -> None:
    """Writes the header the firmware includes.

    Every size carries its unit as a type rather than being a bare number, the
    same as the design values do, so a millimetre cannot reach a place that
    wants points without going through a panel.
    """
    # `note` and anything else written for a reader is not a measurement.
    geometry = {name: millimetres(f"geometry.{name}", value)
                for name, value in data["geometry"].items() if QUANTITY.match(value)}
    colours = {name: value for name, value in data["colours"].items() if name != "note"}
    layouts = {name: one for name, one in data["layouts"].items() if name != "note"}

    lines = [
        "// Generated from tokens/keyboards.json by tools/generate_keyboards.py.",
        "// Do not edit: the next build overwrites this file.",
        "",
        "#ifndef CALIPER_KEYBOARDS_H_",
        "#define CALIPER_KEYBOARDS_H_",
        "",
        "#include <cstdint>",
        "",
        '#include "caliper/units.h"',
        "",
        "// The sizes and the colours stand in the namespace every design value",
        "// stands in, under the names they had whilst they lived in caliper.toml,",
        "// so moving them here changed nothing at the places that read them.",
        "namespace cal::token {",
        "",
    ]

    for name, value in geometry.items():
        lines.append(f"inline constexpr Millimeter kKeyboard{''.join(part.capitalize() for part in name.split('_'))}"
                     f"{{{cpp_float(value)}}};")
    lines.append("")
    for name, value in colours.items():
        wanted = "kKey" if name == "key" else f"kKey{name.capitalize()}"
        lines.append(f"inline constexpr std::uint32_t {wanted} = 0x{value.lstrip('#')};")

    lines += [
        "",
        "}  // namespace cal::token",
        "",
        "// The layouts stand apart from them, because a keyboard is the one design",
        "// value that is a shape rather than a number.",
        "namespace cal::keyboards {",
        "",
        "/// How many rows of characters a keyboard has.",
        "inline constexpr int kRowCount = 4;",
        "",
        "/// How many keys the rows hold between them, which is how many labels a",
        "/// keyboard needs however few characters a layer puts on them.",
        f"inline constexpr int kCharacterKeys = {sum(WIDTHS)};",
        "",
        "/// What stands at the end of a row.",
        "enum class End { kNothing, kFiller, kShift, kBackspace };",
        "",
        "/**",
        " * One country's first layer.",
        " *",
        " * `rows` is what the keys say unshifted, and `shifted` says what shift makes",
        " * of each of them, position by position, with a space where it makes nothing",
        " * new. A letter has no entry there, because shift raises the whole row.",
        " *",
        " * `symbols` holds what that country's layout cannot reach, and nothing else.",
        " * A tablet keyboard needs two layers of them because its letters carry no",
        " * punctuation at all; this one carries a whole national layout with a second",
        " * case, so it reaches nearly everything already. A character that can be",
        " * typed in two places, and in two different places, is the one thing a",
        " * keyboard must not do, so what is reachable does not appear there.",
        " *",
        " * One row per kind, so a row means something and a character is looked for",
        " * rather than hunted. Each row is centred, and a space leaves a key alone",
        " * whilst keeping its place, which is what centres it and what lets a row end",
        " * short without the rest closing up. The fourth row carries nothing, because",
        " * three kinds hold everything any of these languages is missing and a row",
        " * that empties rather than vanishes keeps the keyboard the height it had.",
        " */",
        "struct Alphabet {",
        "   const char* rows[kRowCount];",
        "   const char* shifted[kRowCount];",
        "   const char* symbols[kRowCount];",
        "   End ends[kRowCount][2];",
        "   Millimeter lead[kRowCount];",
        "};",
        "",
    ]

    for name, one in layouts.items():
        ends = ", ".join("{End::%s, End::%s}" % (ENDS[pair.split("/")[0]], ENDS[pair.split("/")[1]])
                         for pair in one["ends"])
        lead = ", ".join(f"Millimeter{{{cpp_float(millimetres(f'{name}.lead', at))}}}" for at in one["lead"])
        lines += [
            f"/// {one['named']}, the layout of the keyboard its country writes on.",
            f"inline constexpr Alphabet k{name.capitalize()}{{",
            "    {" + ", ".join(cpp_string(row) for row in one["rows"]) + "},",
            "    {" + ", ".join(cpp_string(row) for row in one["shifted"]) + "},",
            "    {" + ", ".join(cpp_string(row) for row in one["symbols"]) + "},",
            "    {" + ends + "},",
            "    {" + lead + "}};",
            "",
        ]

    lines += ["}  // namespace cal::keyboards", "", "#endif  // CALIPER_KEYBOARDS_H_", ""]
    HEADER.write_text("\n".join(lines))


def tidy(text: str) -> str:
    """Puts every quantity in the file into one shape.

    The layout page writes this file out as well, and it writes `12 mm` where a
    hand might have written `12.0 mm`. The two mean the same and would differ
    on every round trip, so the generator settles the shape rather than leaving
    a difference that reads like a change.

    @param text The file.
    @returns The file, with every quantity written the one way.
    """
    def once(match: re.Match) -> str:
        said = f"{float(match.group(1)):.2f}".rstrip("0").rstrip(".")
        return f'"{said} mm"'

    return re.sub(r'"(-?\d+(?:\.\d+)?)\s*mm"', once, text)


def main() -> None:
    """Derives what the file does not decide, then writes the header."""
    tidied = tidy(SOURCE.read_text())
    if tidied != SOURCE.read_text():
        SOURCE.write_text(tidied)
        print("  written the one way: the quantities")
    data = json.loads(SOURCE.read_text())
    extras = data["extras"]
    corrected = []

    for name, layout in data["layouts"].items():
        if name == "note":
            continue
        counts = [len(row) for row in layout["rows"]]
        if counts != list(WIDTHS):
            raise SystemExit(f"{SOURCE.name}: {layout['named']} has rows of {counts}, wanted {list(WIDTHS)}")

        derived = symbols_for(layout, extras)
        if layout["symbols"] != derived:
            corrected.append(f"{layout['named']}: {layout['symbols']} became {derived}")
            layout["symbols"] = derived

    if corrected:
        # Only the derived lines are rewritten, so the file keeps the shape a
        # person gave it. The geometry carries a key of the same name, which is
        # why this matches on the indent a layout's line stands at.
        taken = iter([layout["symbols"] for name, layout in data["layouts"].items() if name != "note"])
        body = re.sub(r'(\n      "symbols": )\[[^\n]*\]',
                      lambda match: match.group(1) + json.dumps(next(taken), ensure_ascii=False),
                      SOURCE.read_text())
        SOURCE.write_text(body)
        for line in corrected:
            print(f"  derived again: {line}")

    write_header(data)
    print(f"{len([one for one in data['layouts'] if one != 'note'])} layouts from {SOURCE.name}")
    print(f"  {HEADER.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
