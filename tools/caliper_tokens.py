"""The design values, generated from tokens/caliper.toml.

Do not edit: tools/generate_tokens.py overwrites this file. Change
tokens/caliper.toml and run it again.
"""

SPACING_MM = {
    "edge": 3.27,
    "group": 3.27,
    "inset": 2.72,
    "line_gap": 1.63,
}

TOUCH_MM = {
    "fingertip": 9.0,
    "thumb": 12.0,
}

READING_MM = {
    "title": 945.0,
    "heading": 565.0,
    "body": 410.0,
    "small": 345.0,
}

CAP_ANGLE_MRAD = 6.1
CAP_RATIO = 0.705
X_HEIGHT_RATIO = 0.505
DESCENDER_RATIO = 0.24

RADIUS_MM = {
    "panel": 4.35,
    "tile": 2.72,
    "artwork": 1.63,
}

SQUIRCLE = 3.2
SQUIRCLE_ARTWORK = 8.0

COLOURS = {
    "bg": "#17120e",
    "surface": "#221a14",
    "raised": "#30251b",
    "key": "#3d3126",
    "key-edge": "#45382b",
    "ink": "#faf4ec",
    "muted": "#c3b1a0",
    "faint": "#8d7c6c",
    "accent": "#f0a85c",
    "accent-ink": "#2b1806",
    "accent-dim": "#6b4415",
    "line": "#3d3128",
    "overlay": "#3f3226",
    "danger": "#e8705a",
    "danger-ink": "#2b0c06",
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
