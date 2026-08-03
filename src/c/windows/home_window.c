#include "home_window.h"

static SimpleMenuLayer *s_menu_layer;
static SimpleMenuSection s_menu_section;
static SimpleMenuItem s_menu_items[4];

static void prv_window_load(Window *window)
{
    Layer *window_layer = window_get_root_layer(window);
    const GRect bounds = layer_get_bounds(window_layer);

    s_menu_items[0] = (SimpleMenuItem) {
        .title = "Attention",
        .subtitle = "No issues",
    };

    s_menu_items[1] = (SimpleMenuItem) {
        .title = "Server",
        .subtitle = "Online",
    };

    s_menu_items[2] = (SimpleMenuItem) {
        .title = "Storage",
        .subtitle = "16% used",
    };

    s_menu_items[3] = (SimpleMenuItem) {
        .title = "Downloads",
        .subtitle = "None active",
    };

    s_menu_section = (SimpleMenuSection) {
        .title = "ServerWatch",
        .num_items = 4,
        .items = s_menu_items,
    };

    s_menu_layer = simple_menu_layer_create(
        bounds,
        window,
        &s_menu_section,
        1,
        NULL
    );

    layer_add_child(
        window_layer,
        simple_menu_layer_get_layer(s_menu_layer)
    );
}

static void prv_window_unload(Window *window)
{
    simple_menu_layer_destroy(s_menu_layer);
    s_menu_layer = NULL;
}

Window *home_window_create(void)
{
    Window *window = window_create();

    window_set_window_handlers(
        window,
        (WindowHandlers) {
            .load = prv_window_load,
            .unload = prv_window_unload,
        }
    );

    return window;
}

void home_window_destroy(Window *window)
{
    window_destroy(window);
}
