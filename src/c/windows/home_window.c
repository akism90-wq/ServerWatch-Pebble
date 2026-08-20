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

static char s_attention_text[40];
static char s_server_text[24];
static char s_storage_text[24];
static char s_downloads_text[32];

static bool prv_all_services_online(
    const ServerStatus *status)
{
    if (status == NULL)
    {
        return false;
    }

    for (int index = 0;
         index < SERVER_SERVICE_COUNT;
         ++index)
    {
        if (!status->services[index].online)
        {
            return false;
        }
    }

    return true;
}

static bool prv_has_suspicious_download(
    const ServerStatus *status)
{
    if (status == NULL)
    {
        return false;
    }

    for (int index = 0;
         index < SERVER_DOWNLOAD_COUNT;
         ++index)
    {
        if (status->downloads[index].suspicious)
        {
            return true;
        }
    }

    return false;
}

static void prv_update_menu_text(void)
{
    const ServerStatus *const status =
        server_status_service_get();

    if (status == NULL)
    {
        return;
    }

    /*
     * Never expose the seeded development model before
     * the first genuine ServerWatch snapshot.
     */
    if (!status->has_received_snapshot)
    {
        snprintf(
            s_attention_text,
            sizeof(s_attention_text),
            "Unavailable");

        snprintf(
            s_server_text,
            sizeof(s_server_text),
            "Connection failed");

        snprintf(
            s_storage_text,
            sizeof(s_storage_text),
            "Unavailable");

        snprintf(
            s_downloads_text,
            sizeof(s_downloads_text),
            "Unavailable");

        return;
    }

    /*
     * Attention
     */
    if (status->attention_item_count <= 0)
    {
        snprintf(
            s_attention_text,
            sizeof(s_attention_text),
            "No issues");
    }
    else if (status->attention_item_count == 1)
    {
        snprintf(
            s_attention_text,
            sizeof(s_attention_text),
            "1 item requires attention");
    }
    else
    {
        snprintf(
            s_attention_text,
            sizeof(s_attention_text),
            "%d items require attention",
            status->attention_item_count);
    }

    /*
     * Server
     *
     * Connection state takes precedence over cached
     * server/service state.
     */
    if (status->connection_state ==
        CONNECTION_STATE_FAILED)
    {
        snprintf(
            s_server_text,
            sizeof(s_server_text),
            "Connection Lost");
    }
    else if (!status->server_online)
    {
        snprintf(
            s_server_text,
            sizeof(s_server_text),
            "Warning");
    }
    else if (!prv_all_services_online(status))
    {
        snprintf(
            s_server_text,
            sizeof(s_server_text),
            "Warning");
    }
    else
    {
        snprintf(
            s_server_text,
            sizeof(s_server_text),
            "Online");
    }

    /*
     * Storage
     */
    if (status->storage_warning)
    {
        snprintf(
            s_storage_text,
            sizeof(s_storage_text),
            "Warning - %d%% used",
            status->storage_used_percent);
    }
    else
    {
        snprintf(
            s_storage_text,
            sizeof(s_storage_text),
            "%d%% used",
            status->storage_used_percent);
    }

    /*
     * Downloads
     */
    if (status->active_downloads <= 0)
    {
        snprintf(
            s_downloads_text,
            sizeof(s_downloads_text),
            "None active");
    }
    else if (prv_has_suspicious_download(status))
    {
        snprintf(
            s_downloads_text,
            sizeof(s_downloads_text),
            "%d active - warning",
            status->active_downloads);
    }
    else
    {
        snprintf(
            s_downloads_text,
            sizeof(s_downloads_text),
            "%d active",
            status->active_downloads);
    }
}

static void prv_destination_selected(
    int index,
    void *context)
{
    (void)context;

    if ((index < 0) ||
        (index >= HOME_DESTINATION_COUNT))
    {
        return;
    }

    WindowDescriptor *const destination =
        &s_destinations[index];

    if (destination->window == NULL)
    {
        destination->window =
            destination->create();
    }

    if (destination->window != NULL)
    {
        window_stack_push(
            destination->window,
            true);
    }
}

static void prv_window_load(
    Window *window)
{
    Layer *const window_layer =
        window_get_root_layer(window);

    const GRect bounds =
        layer_get_bounds(window_layer);

    prv_update_menu_text();

    s_menu_items[HOME_DESTINATION_ATTENTION] =
        (SimpleMenuItem) {
            .title = "Attention",
            .subtitle = s_attention_text,
            .callback = prv_destination_selected,
        };

    s_menu_items[HOME_DESTINATION_SERVER] =
        (SimpleMenuItem) {
            .title = "Server",
            .subtitle = s_server_text,
            .callback = prv_destination_selected,
        };

    s_menu_items[HOME_DESTINATION_STORAGE] =
        (SimpleMenuItem) {
            .title = "Storage",
            .subtitle = s_storage_text,
            .callback = prv_destination_selected,
        };

    s_menu_items[HOME_DESTINATION_DOWNLOADS] =
        (SimpleMenuItem) {
            .title = "Downloads",
            .subtitle = s_downloads_text,
            .callback = prv_destination_selected,
        };

    s_menu_section =
        (SimpleMenuSection) {
            .title = "ServerWatch",
            .num_items =
                HOME_DESTINATION_COUNT,
            .items =
                s_menu_items,
        };

    s_menu_layer =
        simple_menu_layer_create(
            bounds,
            window,
            &s_menu_section,
            1,
            NULL);

    if (s_menu_layer != NULL)
    {
        layer_add_child(
            window_layer,
            simple_menu_layer_get_layer(
                s_menu_layer));
    }
}

static void prv_window_unload(
    Window *window)
{
    (void)window;

    if (s_menu_layer != NULL)
    {
        simple_menu_layer_destroy(
            s_menu_layer);

        s_menu_layer = NULL;
    }

    for (int index = 0;
         index < HOME_DESTINATION_COUNT;
         ++index)
    {
        WindowDescriptor *const destination =
            &s_destinations[index];

        if (destination->window != NULL)
        {
            destination->destroy(
                destination->window);

            destination->window = NULL;
        }
    }
}

Window *home_window_create(void)
{
    Window *const window =
        window_create();

    if (window != NULL)
    {
        window_set_window_handlers(
            window,
            (WindowHandlers) {
                .load = prv_window_load,
                .unload = prv_window_unload,
            });
    }

    return window;
}

void home_window_destroy(
    Window *window)
{
    if (window != NULL)
    {
        window_destroy(window);
    }
}

void home_window_refresh(void)
{
    if (s_menu_layer == NULL)
    {
        return;
    }

    prv_update_menu_text();

    layer_mark_dirty(
        simple_menu_layer_get_layer(
            s_menu_layer));
}