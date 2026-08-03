#include "downloads_window.h"

static TextLayer *s_title_layer;
static TextLayer *s_status_layer;

static void prv_window_load(Window *window)
{
    Layer *window_layer = window_get_root_layer(window);
    const GRect bounds = layer_get_bounds(window_layer);

    s_title_layer = text_layer_create(
        GRect(0, 20, bounds.size.w, 30)
    );
    text_layer_set_text(s_title_layer, "Downloads");
    text_layer_set_text_alignment(
        s_title_layer,
        GTextAlignmentCenter
    );
    text_layer_set_font(
        s_title_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD)
    );

    s_status_layer = text_layer_create(
        GRect(0, 70, bounds.size.w, 30)
    );
    text_layer_set_text(s_status_layer, "None active");
    text_layer_set_text_alignment(
        s_status_layer,
        GTextAlignmentCenter
    );

    layer_add_child(
        window_layer,
        text_layer_get_layer(s_title_layer)
    );

    layer_add_child(
        window_layer,
        text_layer_get_layer(s_status_layer)
    );
}

static void prv_window_unload(Window *window)
{
    text_layer_destroy(s_status_layer);
    s_status_layer = NULL;

    text_layer_destroy(s_title_layer);
    s_title_layer = NULL;
}

Window *downloads_window_create(void)
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

void downloads_window_destroy(Window *window)
{
    window_destroy(window);
}
