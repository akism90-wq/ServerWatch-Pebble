#include <pebble.h>

#include "windows/home_window.h"

static Window *s_home_window;

static void prv_init(void)
{
    s_home_window = home_window_create();
    window_stack_push(s_home_window, true);
}

static void prv_deinit(void)
{
    home_window_destroy(s_home_window);
    s_home_window = NULL;
}

int main(void)
{
    prv_init();
    app_event_loop();
    prv_deinit();
}
