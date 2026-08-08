#include "home_window.h"

#include <stdio.h>

#include "../services/server_status_service.h"

typedef enum
{
    HOME_DESTINATION_ATTENTION = 0,
    HOME_DESTINATION_SERVER,
    HOME_DESTINATION_STORAGE,
    HOME_DESTINATION_DOWNLOADS,
    HOME_DESTINATION_COUNT
} HomeDestination;

typedef Window *(*WindowCreateFunction)(void);
typedef void (*WindowDestroyFunction)(Window *window);

typedef struct
{
    Window *window;
    WindowCreateFunction create;
    WindowDestroyFunction destroy;
} WindowDescriptor;

static SimpleMenuLayer *s_menu_layer;
static SimpleMenuSection s_menu_section;
static SimpleMenuItem s_menu_items[HOME_DESTINATION_COUNT];

static WindowDescriptor s_destinations[HOME_DESTINATION_COUNT] = {
    [HOME_DESTINATION_ATTENTION] = {
        .window = NULL,
        .create = attention_window_create,
        .destroy = attention_window_destroy,
    },
    [HOME_DESTINATION_SERVER] = {
        .window = NULL,
        .create = server_window_create,
        .destroy = server_window_destroy,
    },
    [HOME_DESTINATION_STORAGE] = {
        .window = NULL,
        .create = storage_window_create,
        .destroy = storage_window_destroy,
    },
    [HOME_DESTINATION_DOWNLOADS] = {
        .window = NULL,
        .create = downloads_window_create,
        .destroy = downloads_window_destroy,
    },
};

static char s_storage_text[16];
static char s_downloads_text[20];

static void prv_destination_selected(int index, void *context)
{
    if ((index < 0) || (index >= HOME_DESTINATION_COUNT)) {
        return;
    }

    WindowDescriptor *const destination = &s_destinations[index];

    if (destination->window == NULL) {
        destination->window = destination->create();
    }

    if (destination->window != NULL) {
        window_stack_push(destination->window, true);
    }
}

static void prv_window_load(Window *window)
{
    Layer *const window_layer = window_get_root_layer(window);
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
            "%d active downloads",
            status.active_downloads
        );
    }

    s_menu_items[HOME_DESTINATION_ATTENTION] = (SimpleMenuItem) {
        .title = "Attention",
        .subtitle = status.has_attention ? "Attention required" : "No issues",
        .callback = prv_destination_selected,
    };

    s_menu_items[HOME_DESTINATION_SERVER] = (SimpleMenuItem) {
        .title = "Server",
        .subtitle = status.server_online ? "Online" : "Offline",
        .callback = prv_destination_selected,
    };

    s_menu_items[HOME_DESTINATION_STORAGE] = (SimpleMenuItem) {
        .title = "Storage",
        .subtitle = s_storage_text,
        .callback = prv_destination_selected,
    };

    s_menu_items[HOME_DESTINATION_DOWNLOADS] = (SimpleMenuItem) {
        .title = "Downloads",
        .subtitle = s_downloads_text,
        .callback = prv_destination_selected,
    };

    s_menu_section = (SimpleMenuSection) {
        .title = "ServerWatch",
        .num_items = HOME_DESTINATION_COUNT,
        .items = s_menu_items,
    };

    s_menu_layer = simple_menu_layer_create(
        bounds,
        window,
        &s_menu_section,
        1,
        NULL
    );

    if (s_menu_layer != NULL) {
        layer_add_child(
            window_layer,
            simple_menu_layer_get_layer(s_menu_layer)
        );
    }
}

static void prv_window_unload(Window *window)
{
    if (s_menu_layer != NULL) {
        simple_menu_layer_destroy(s_menu_layer);
        s_menu_layer = NULL;
    }

    for (int index = 0;
         index < HOME_DESTINATION_COUNT;
         ++index) {
        WindowDescriptor *const destination = &s_destinations[index];

        if (destination->window != NULL) {
            destination->destroy(destination->window);
            destination->window = NULL;
        }
    }
}

Window *home_window_create(void)
{
    Window *const window = window_create();

    if (window != NULL) {
        window_set_window_handlers(
            window,
            (WindowHandlers) {
                .load = prv_window_load,
                .unload = prv_window_unload,
            }
        );
    }

    return window;
}

void home_window_destroy(Window *window)
{
    if (window != NULL) {
        window_destroy(window);
    }
}
