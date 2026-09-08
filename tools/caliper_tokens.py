"""The design values, generated from tokens/caliper.toml.

Do not edit: tools/generate_tokens.py overwrites this file. Change
tokens/caliper.toml and run it again.
"""

SPACING_MM = {
    "edge": 2.18,
    "group": 3.27,
    "inset": 2.72,
    "line_gap": 1.63,
    "floating": 1.09,
    "keyboard_gap": 1.09,
    "player_inset": 1.09,
    "status_edge": 1.09,
    "status_gap": 2.18,
}

LAYOUT_MM = {
    "status_bar": 4.35,
    "header": 9.25,
    "footer": 10.88,
    "button": 6.53,
    "keyboard_key": 8.98,
    "keyboard_shift": 11.42,
    "keyboard_switch": 13.6,
    "media_key": 6.8,
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
    "key_label": 468.0,
}

CAP_ANGLE_MRAD = 6.1
CAP_RATIO = 0.7
X_HEIGHT_RATIO = 0.506
DESCENDER_RATIO = 0.2

RADIUS_MM = {
    "panel": 4.35,
    "tile": 2.18,
    "artwork": 1.63,
    "button": 1.63,
}

SQUIRCLE = 3.2
SQUIRCLE_ARTWORK = 8.0

GRID_UNIT = 2

COLOURS = {
    "bg": "#111111",
    "surface": "#1b1b1b",
    "raised": "#262626",
    "key": "#3d3d3d",
    "key-edge": "#4b4b4b",
    "ink": "#ececec",
    "status-ink": "#f0f0f0",
    "muted": "#939393",
    "faint": "#757575",
    "accent": "#009ce9",
    "accent-ink": "#ffffff",
    "accent-dim": "#0a5570",
    "line": "#353535",
    "overlay": "#323232",
    "danger": "#f85149",
    "danger-ink": "#ffffff",
}


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
