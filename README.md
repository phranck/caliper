# Caliper

An extension for [LVGL](https://lvgl.io) that makes an interface on a controller board measurable.

It answers four questions that a graphics toolkit deliberately leaves open, and that every panel project ends up answering again from scratch.

**A measurement is either physical or spatial, and the two do not scale alike.** A fingertip is 9 mm wherever it is. On an 800 by 480 panel across five inches that is 66 points, because the panel carries 7.35 pixels per millimetre. On a 1024 by 600 panel across seven inches the density is lower and the same fingertip is 60 points, whilst a factor taken from the width would say 84. Caliper takes a description of the panel and converts millimetres and reading distances through its density, so a fingertip stays a fingertip and a body text stays readable from 40 cm.

**Anything carrying text has a minimum width, never a fixed one.** A fixed width truncates a longer word the moment the interface carries a second language. A control sizes to its label plus its padding, and never below the minimum the layout asks for, so a row of buttons stays even and a translation stays whole.

**The design values live in one place.** Spacings, radii, the type scale, the palette and the touch minima are written once, as data. The C header and anything else that needs them is generated from it, so a device and the documents describing it cannot drift apart.

**Every build is checked.** Nothing smaller than a fingertip, nothing closer to an edge than the margin, no text over its box in any language, every nested corner concentric, every edge on the grid, and nothing hidden behind a band. The checks run over the finished object tree, on the host as well as on the device, and a failure stops the build.

## Status

Early. The design is written down in the plan; the code is being built in stages. The first user is [LEVEL](https://github.com/phranck/LEVEL), a control panel for Sonos systems, and until it is done Caliper adds nothing that LEVEL does not need.

## Requirements

Caliper is bound to LVGL and does not stand on its own. It is built and used as an ESP-IDF component, and the core carries no dependency on ESP-IDF itself, so it also builds for the host, which is where the checks run in tests.

The panel it is designed for is 800 by 480 or larger. Below that a keyboard of ten keys at fingertip size no longer fits between the margins, and the answer there is a different keyboard rather than a smaller one.

## License

This repository has been published under the [MIT](https://layered.mit-license.org) license.
