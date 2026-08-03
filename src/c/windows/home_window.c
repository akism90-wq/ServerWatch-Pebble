#include "home_window.h"

#include <stdio.h>

#include "../services/server_status_service.h"

static SimpleMenuLayer *s_menu_layer;
static SimpleMenuSection s_menu_section;
static SimpleMenuItem s_menu_items[4];
static Window *s_attention_window;

static char s_storage_text[16];
static char s_downloads_text[20];

static void prv_attention_selected(int index, void *context)
{
    if (s_attention_window == NULL) {
        s_attention_window = attention_window_create();
    }

    window_stack_push(s_attention_window, true);
}

static void prv_window_load(Window *window)
{
    Layer *window_layer = window_get_root_layer(window);
    const GRect bounds = layer_get_bounds(window_layer);
    const ServerStatus status = server_status_service_get();

    snprintf(
        s_storage_text,
        sizeof(s_storage_text),
        "%d%% used",
        status.storage_used_percent
    );

    if (status.active_downloads == 0) {
        snprintf(
            s_downloads_text,
            sizeof(s_downloads_text),
            "None active"
        );
    } else {
        snprintf(
            s_downloads_text,
            sizeof(s_downloads_text),
            "%d active",
            status.active_downloads
        );
    }

    s_menu_items[0] = (SimpleMenuItem) {
        .title = "Attention",
        .subtitle = status.has_attention ? "Issues found" : "No issues",
        .callback = prv_attention_selected,
    };

    s_menu_items[1] = (SimpleMenuItem) {
        .title = "Server",
        .subtitle = status.server_online ? "Online" : "Offline",
    };

    s_menu_items[2] = (SimpleMenuItem) {
        .title = "Storage",
        .subtitle = s_storage_text,
    };

    s_menu_items[3] = (SimpleMenuItem) {
        .title = "Downloads",
        .subtitle = s_downloads_text,
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

    if (s_attention_window != NULL) {
        attention_window_destroy(s_attention_window);
        s_attention_window = NULL;
    }
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
