// Nothing runs here. Naming each part of the library is what makes the linker
// pull it in, which is the whole point: a component that compiles but does not
// link is not one anybody can use.

#include "caliper/checks.hpp"
#include "caliper/components.hpp"
#include "caliper/inspect.hpp"
#include "caliper/theme.hpp"

extern "C" void app_main(void)
{
    cal::install_theme(lv_display_get_default(), cal::jc8048w500, cal::typography());

    cal::Screen screen{cal::jc8048w500};
    screen.status_bar();
    screen.header("Nothing");

    cal::Object group = cal::list(screen.content(), screen.panel());
    cal::row(group, screen.panel(), "Nothing");
    cal::choose_one(group, 0);
    cal::button(screen.footer(), screen.panel(), "Nothing", true);

    cal::inspect_and_log(screen);
}
