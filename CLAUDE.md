# Caliper conventions

## The language is C++20, and it is not C

Every line of this library is C++20. There is no C part, no "plain C core", and no header written to be includable from C. Anyone reaching for C here has misread the point of the library.

The reason is the one thing Caliper exists to do. A millimeter and a pixel are different kinds of quantity, and in C both are a number, so nothing stops one being written where the other belongs. That is the mistake this project already made once on a real panel. As distinct types the compiler refuses it, and the error cannot be written at all. Take the types away and what is left is a naming convention with a build step.

The same choice buys the conversions at compile time. A panel that is known when the firmware is built costs nothing at run time, because `constexpr` does the arithmetic; a panel established at start-up runs the same line then.

LVGL is C and stays C. It is called as the C library it is, and its headers are included as they come. Wrapping it, replacing it, or building a C-shaped layer in front of it is not something this library does.

## How it is built

No exceptions, no RTTI, and no allocation after start-up. These are the settings ESP-IDF already defaults to for C++, so this is a statement of what holds rather than a set of flags to add.

The library builds twice: as an ESP-IDF component for the device, and for the host, where the tests run without a board attached. Both builds come from the same tree.

## The values are generated, not written

`tokens/caliper.toml` holds every design value once, and `tools/generate_tokens.py` writes `include/caliper/tokens.h` and `tools/caliper_tokens.py` from it. Neither output is edited, because the next run puts the old one back. A key under `[layout]` becomes `k<Name>`, the same as one under `[spacing]`.

`tools/caliper_tokens.py` has a reader outside this repository: `Papers/level/sources/build_screens.py` imports it directly. Before removing or renaming a key, check what that generator does with it, because nothing here would notice a broken import on its own.

## What the tests reach

The two host tests work on `Check` and on `Panel`, which need no board and no graphics library. Everything else, so `Inspect`, `Screen` and every component, needs a live object tree, and it is verified by building the screens in LEVEL's simulator and reading what caliper reports there.

## Where the rest is

`Papers/plan/plan.html`, identifier `PAP-CAL-001`, carries the design and is the source for the issues. It lives outside this repository and is synchronised separately, so it is not on GitHub.
