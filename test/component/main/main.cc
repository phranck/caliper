// Nothing runs here. Naming each part of the library is what makes the linker
// pull it in, which is the whole point: a component that compiles but does not
// link is not one anybody can use.

#include "caliper/checks.h"
#include "caliper/components.h"
#include "caliper/inspect.h"
#include "caliper/theme.h"

extern "C" void app_main(void) {
   cal::InstallTheme(lv_display_get_default(), cal::kJc8048w500, cal::typography());

   cal::Screen screen{cal::kJc8048w500};
   screen.status_bar();
   screen.Header("Nothing");

   cal::Object group = cal::List(screen.Content(), screen.panel());
   cal::Row(group, screen.panel(), "Nothing");
   cal::ChooseOne(group, 0);
   cal::Button(screen.Footer(), screen.panel(), "Nothing", true);

   cal::InspectAndLog(screen);
}
