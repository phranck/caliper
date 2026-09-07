#ifndef CALIPER_SHAPES_H_
#define CALIPER_SHAPES_H_

#include <cstdint>

#include "lvgl.h"

namespace cal {

/**
 * The stencil of a squircle at a given size.
 *
 * The design's shape is a superellipse, and the graphics library draws only
 * rectangles with a corner radius. So the shape is an image with nothing but an
 * alpha channel, and whatever colour is wanted is shown through it.
 *
 * The stencils are generated from `tokens/caliper.toml` by
 * `tools/generate_squircles.py`, one per size the design actually uses.
 *
 * @param size How many points across and down.
 * @returns The stencil, or nullptr for a size none was made for.
 */
const lv_image_dsc_t* SquircleMask(std::int32_t size);

/**
 * A squircle of one colour, as an object.
 *
 * Everything the design draws with this shape goes through here: a key of the
 * player, the end of the player pill, a tile. Where no stencil exists for the
 * size, it falls back to a fully rounded rectangle, which is the closest the
 * library can do on its own.
 *
 * @param parent What it goes into.
 * @param size How many points across and down.
 * @param colour What it is filled with.
 * @returns The object, so a caller can put something in it or attach an event.
 */
lv_obj_t* Squircle(lv_obj_t* parent, std::int32_t size, std::uint32_t colour);

/**
 * A picture in the shape of a squircle.
 *
 * The picture is the object's background rather than an image in it, because
 * the stencil is applied to what the object draws and an image inside it would
 * be drawn over the shape rather than inside it.
 *
 * @param parent What it goes into.
 * @param size How many points across and down. The picture is drawn at its own
 *             size, so it should be this size to fill the shape.
 * @param picture What shows through it.
 * @returns The object.
 */
lv_obj_t* SquircleImage(lv_obj_t* parent, std::int32_t size, const lv_image_dsc_t* picture);

}  // namespace cal

#endif  // CALIPER_SHAPES_H_
