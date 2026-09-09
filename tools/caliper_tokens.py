"""The design values, generated from tokens/caliper.toml.

Do not edit: tools/generate_tokens.py overwrites this file. Change
tokens/caliper.toml and run it again.
"""

# The panels, each one its width and height in pixels and how many of
# them a millimetre holds.
PANELS = {
    "jc8048w500": {"width": 800, "height": 480, "points_per_mm": 7.35},
}

SPACING_MM = {
    "edge": 2.18,
    "group": 3.27,
    "inset": 2.72,
    "line_gap": 1.63,
    "floating": 1.09,
    "keyboard_gap": 1.09,
    "player_inset": 1.09,
    "status_edge": 1.09,
    "slider_label": 2.18,
    "sub_text_gap": 0.82,
    "status_gap": 2.18,
}

LAYOUT_MM = {
    "status_bar": 4.35,
    "header": 9.25,
    "footer": 10.88,
    "button": 6.53,
    "card": 68.0,
    "card_symbol": 5.44,
    "content_unavailable_symbol": 7.07,
    "keyboard_key": 7.1,
    "keyboard_key_high": 6.8,
    "keyboard_edge": 1.36,
    "keyboard_symbols": 11.4,
    "keyboard_confirm": 20.9,
    "keyboard_escape": 12.0,
    "keyboard_char": 2.18,
    "keyboard_shifted": 1.63,
    "keyboard_stack": 1.09,
    "keyboard_mark": 2.2,
    "keyboard_corner": 1.1,
    "media_key": 6.8,
    "track": 2.18,
    "slider_knob": 5.99,
    "sidebar": 11.97,
    "sidebar_item": 10.88,
}

TOUCH_MM = {
    "fingertip": 9.0,
    "thumb": 12.0,
    "minimum": 6.53,
}

READING_MM = {
    "title": 935.0,
    "heading": 560.0,
    "body": 405.0,
    "small": 345.0,
    "status": 265.0,
    "caption": 203.0,
    "key_label": 405.0,
}

CAP_ANGLE_MRAD = 6.1
CAP_RATIO = 0.7
X_HEIGHT_RATIO = 0.506
DESCENDER_RATIO = 0.2

RADIUS_MM = {
    "card": 3.81,
    "artwork": 1.63,
}

SQUIRCLE = 3.2
SQUIRCLE_ARTWORK = 8.0

GRID_UNIT = 2

COLOURS = {
    "bg": "#111111",
    "surface": "#1b1b1b",
    "raised": "#262626",
    "key-tray": "#1b1b1b",
    "key-filler": "#282828",
    "key-quiet": "#3a3a3a",
    "key": "#626262",
    "key-ink": "#ececec",
    "key-shifted": "#c0c0c0",
    "key-mark": "#c0c0c0",
    "ink": "#ececec",
    "status-ink": "#f0f0f0",
    "muted": "#939393",
    "faint": "#757575",
    "accent": "#009ce9",
    "line": "#353535",
    "overlay": "#323232",
    "danger": "#f85149",
    "danger-ink": "#ffffff",
}


def _linear(channel: int) -> float:
    """One channel of a colour, on the scale light is actually measured in.

    A colour as it is written is not proportional to the light coming off the
    glass, and everything about contrast is. Caliper carries the same
    arithmetic, because a drawing and a device that disagreed about which ink
    stands on the accent would disagree about what the product looks like.
    """
    share = channel / 255
    return share / 12.92 if share <= 0.04045 else ((share + 0.055) / 1.055) ** 2.4


def luminance(colour: str) -> float:
    """How much light a colour gives off, from 0 for black to 1 for white."""
    value = int(colour.lstrip('#'), 16)
    return (0.2126 * _linear((value >> 16) & 0xFF)
            + 0.7152 * _linear((value >> 8) & 0xFF)
            + 0.0722 * _linear(value & 0xFF))


def ink_on(colour: str) -> str:
    """What a large word is set in when it stands on a colour.

    White whilst it still reaches three to one, which is the floor a word set
    this large holds, and the ground's own colour once it does not. The
    design's accent lands at 3.03, which is why white stands on it.
    """
    return "#ffffff" if 1.05 / (luminance(colour) + 0.05) >= 3.0 else COLOURS["bg"]


def dimmed_accent(colour: str) -> str:
    """A colour taken most of the way to the ground, for what is not now."""
    value = int(colour.lstrip('#'), 16)
    ground = int(COLOURS['bg'].lstrip('#'), 16)
    mixed = 0
    for shift in (16, 8, 0):
        here = (value >> shift) & 0xFF
        there = (ground >> shift) & 0xFF
        mixed |= int(here + (there - here) * 0.55 + 0.5) << shift
    return '#%06x' % mixed


# The two that follow from the accent rather than standing beside it. Written
# into the palette so a drawing asks for them the way it asks for any colour.
COLOURS["accent-ink"] = ink_on(COLOURS["accent"])
COLOURS["accent-dim"] = dimmed_accent(COLOURS["accent"])


def _rounded(value: float) -> int:
    """Rounds to the nearest whole number, halves away from zero.

    Caliper's C++ side rounds this way, and the two have to agree: a value
    converted here and the same value converted there would otherwise land
    on different pixels at exactly one half. Python's own round breaks ties
    towards even, which is the difference.
    """
    return int(value - 0.5) if value < 0 else int(value + 0.5)


def points(millimetres: float, points_per_mm: float) -> int:
    """Converts a physical size into points on a given panel.

    @param millimetres The size on the glass.
    @param points_per_mm The density of the panel it is drawn on.
    @returns The size in whole points.
    """
    return _rounded(millimetres * points_per_mm)


def type_size(reading_mm: float, points_per_mm: float) -> int:
    """The type size a cap height needs to hold its angle at a distance.

    A capital seen from further away has to be taller to appear the same
    size, and the angle it appears under is what legibility follows. This
    is why a grade states a distance rather than a number of points.

    @param reading_mm How far away the text is meant to be read from.
    @param points_per_mm The density of the panel it is drawn on.
    @returns The type size in whole points.
    """
    cap_height_mm = CAP_ANGLE_MRAD / 1000.0 * reading_mm
    return _rounded(cap_height_mm * points_per_mm / CAP_RATIO)


def inner_radius(outer_mm: float, gap_mm: float | None = None) -> float:
    """A corner inside another is the one around it less the space between.

    Stated as a rule rather than as a second number, so the two cannot fall
    out of step when either moves.

    @param outer_mm The corner around it.
    @param gap_mm The space between the two, the standard inset by default.
    @returns The inner corner in millimetres.
    """
    return outer_mm - (SPACING_MM["inset"] if gap_mm is None else gap_mm)
