#include "caliper/shapes.h"

namespace cal {

lv_obj_t* Squircle(lv_obj_t* parent, std::int32_t size, std::uint32_t colour) {
   lv_obj_t* shape = lv_obj_create(parent);
   lv_obj_remove_style_all(shape);
   lv_obj_set_size(shape, size, size);
   lv_obj_set_style_bg_color(shape, lv_color_hex(colour), 0);
   lv_obj_set_style_bg_opa(shape, LV_OPA_COVER, 0);

   const lv_image_dsc_t* stencil = SquircleMask(size);
   if (stencil != nullptr) {
      lv_obj_set_style_bitmap_mask_src(shape, stencil, 0);
   } else {
      // No stencil at this size, so the closest the library can do on its own.
      // Visible beside a real one, which is why the sizes are generated from
      // the places that use them rather than guessed.
      lv_obj_set_style_radius(shape, LV_RADIUS_CIRCLE, 0);
   }
   return shape;
}

lv_obj_t* SquircleImage(lv_obj_t* parent, std::int32_t size, const lv_image_dsc_t* picture) {
   lv_obj_t* shape = lv_obj_create(parent);
   lv_obj_remove_style_all(shape);
   lv_obj_set_size(shape, size, size);
   lv_obj_set_style_bg_image_src(shape, picture, 0);

   const lv_image_dsc_t* stencil = SquircleMask(size);
   if (stencil != nullptr) {
      lv_obj_set_style_bitmap_mask_src(shape, stencil, 0);
   } else {
      lv_obj_set_style_radius(shape, LV_RADIUS_CIRCLE, 0);
      lv_obj_set_style_clip_corner(shape, true, 0);
   }
   return shape;
}

}  // namespace cal
