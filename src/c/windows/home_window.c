#include "home_window.h"

#include <stdio.h>

#include "../services/server_status_service.h"

#define LANDING_TITLE_Y 8
#define LANDING_TITLE_HEIGHT 28

#define LANDING_SERVER_Y 34
#define LANDING_SERVER_HEIGHT 22

#define LANDING_CAT_Y 78
#define LANDING_CAT_WIDTH 164
#define LANDING_CAT_HEIGHT 100

#define LANDING_Z_SMALL_OFFSET_X -18
#define LANDING_Z_SMALL_Y 65
#define LANDING_Z_SMALL_WIDTH 18
#define LANDING_Z_SMALL_HEIGHT 22

#define LANDING_Z_MEDIUM_OFFSET_X 0
#define LANDING_Z_MEDIUM_Y 57
#define LANDING_Z_MEDIUM_WIDTH 22
#define LANDING_Z_MEDIUM_HEIGHT 26

#define LANDING_Z_LARGE_OFFSET_X 22
#define LANDING_Z_LARGE_Y 57
#define LANDING_Z_LARGE_WIDTH 28
#define LANDING_Z_LARGE_HEIGHT 32

#define LANDING_BUBBLE_OFFSET_X 32
#define LANDING_BUBBLE_Y 58
#define LANDING_BUBBLE_SIZE 10

#define LANDING_MESSAGE_HEIGHT 28
#define LANDING_HINT_HEIGHT 18

#define HEALTHY_Z_ANIMATION_PERIOD_MS 650

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

static Window *s_window;

static SimpleMenuLayer *s_menu_layer;
static SimpleMenuSection s_menu_section;
static SimpleMenuItem s_menu_items[HOME_DESTINATION_COUNT];

static Layer *s_landing_layer;
static TextLayer *s_landing_title_layer;
static TextLayer *s_landing_server_layer;
static TextLayer *s_landing_message_layer;
static TextLayer *s_landing_hint_layer;
static GBitmap *s_healthy_cat_bitmap;
static BitmapLayer *s_healthy_cat_layer;
static TextLayer *s_healthy_z_small_layer;
static TextLayer *s_healthy_z_medium_layer;
static TextLayer *s_healthy_z_large_layer;
static Layer *s_healthy_bubble_layer;
static AppTimer *s_healthy_z_timer;

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

static bool s_user_opened_menu;
static bool s_showing_landing;
static int s_selected_menu_index;
static int s_healthy_sleep_phase;

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
        if (!status->services[index].monitored)
        {
            continue;
        }

        if (!status->services[index].online)
        {
            return false;
        }
    }

    return true;
}

static bool prv_is_healthy_landing_state(
    const ServerStatus *status)
{
    return (status != NULL) &&
           status->has_received_snapshot &&
           (status->connection_state ==
            CONNECTION_STATE_CONNECTED) &&
           status->server_online &&
           (status->attention_item_count <= 0) &&
           prv_all_services_online(status);
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

static TextLayer *prv_create_centered_text_layer(
    GRect frame,
    const char *text,
    const char *font_key)
{
    TextLayer *const layer =
        text_layer_create(frame);

    if (layer == NULL)
    {
        return NULL;
    }

    text_layer_set_text(
        layer,
        text);

    text_layer_set_font(
        layer,
        fonts_get_system_font(
            font_key));

    text_layer_set_text_alignment(
        layer,
        GTextAlignmentCenter);

    text_layer_set_background_color(
        layer,
        GColorClear);

    return layer;
}

static void prv_healthy_bubble_update_proc(
    Layer *layer,
    GContext *context)
{
    const GRect bounds =
        layer_get_bounds(layer);

    graphics_context_set_stroke_color(
        context,
        GColorBlack);

    graphics_draw_circle(
        context,
        GPoint(
            bounds.size.w / 2,
            bounds.size.h / 2),
        (bounds.size.w / 2) - 1);
}

static void prv_apply_healthy_sleep_phase(void)
{
    if ((s_healthy_z_small_layer == NULL) ||
        (s_healthy_z_medium_layer == NULL) ||
        (s_healthy_z_large_layer == NULL) ||
        (s_healthy_bubble_layer == NULL))
    {
        return;
    }

    layer_set_hidden(
        text_layer_get_layer(
            s_healthy_z_small_layer),
        s_healthy_sleep_phase != 0);

    layer_set_hidden(
        text_layer_get_layer(
            s_healthy_z_medium_layer),
        s_healthy_sleep_phase != 1);

    layer_set_hidden(
        text_layer_get_layer(
            s_healthy_z_large_layer),
        true);

    layer_set_hidden(
        s_healthy_bubble_layer,
        s_healthy_sleep_phase != 2);
}

static void prv_healthy_z_timer_callback(
    void *context)
{
    (void)context;

    s_healthy_z_timer = NULL;

    s_healthy_sleep_phase =
        (s_healthy_sleep_phase + 1) % 3;

    prv_apply_healthy_sleep_phase();

    s_healthy_z_timer =
        app_timer_register(
            HEALTHY_Z_ANIMATION_PERIOD_MS,
            prv_healthy_z_timer_callback,
            NULL);
}

static void prv_stop_healthy_z_animation(void)
{
    if (s_healthy_z_timer != NULL)
    {
        app_timer_cancel(
            s_healthy_z_timer);

        s_healthy_z_timer = NULL;
    }

    s_healthy_sleep_phase = 0;
    prv_apply_healthy_sleep_phase();
}

static void prv_start_healthy_z_animation(void)
{
    if (s_healthy_z_timer != NULL)
    {
        return;
    }

    s_healthy_sleep_phase = 0;
    prv_apply_healthy_sleep_phase();

    s_healthy_z_timer =
        app_timer_register(
            HEALTHY_Z_ANIMATION_PERIOD_MS,
            prv_healthy_z_timer_callback,
            NULL);
}

static void prv_set_healthy_animation_visible(
    bool visible)
{
    if (visible)
    {
        prv_start_healthy_z_animation();
    }
    else
    {
        prv_stop_healthy_z_animation();

        if (s_healthy_z_small_layer != NULL)
        {
            layer_set_hidden(
                text_layer_get_layer(
                    s_healthy_z_small_layer),
                true);
        }

        if (s_healthy_z_medium_layer != NULL)
        {
            layer_set_hidden(
                text_layer_get_layer(
                    s_healthy_z_medium_layer),
                true);
        }

        if (s_healthy_z_large_layer != NULL)
        {
            layer_set_hidden(
                text_layer_get_layer(
                    s_healthy_z_large_layer),
                true);
        }

        if (s_healthy_bubble_layer != NULL)
        {
            layer_set_hidden(
                s_healthy_bubble_layer,
                true);
        }
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

static void prv_show_menu(void)
{
    if (s_landing_layer != NULL)
    {
        layer_set_hidden(
            s_landing_layer,
            true);
    }

    if (s_menu_layer != NULL)
    {
        Layer *const menu_layer =
            simple_menu_layer_get_layer(
                s_menu_layer);

        layer_set_hidden(
            menu_layer,
            false);

        layer_mark_dirty(
            menu_layer);

        simple_menu_layer_set_selected_index(
            s_menu_layer,
            s_selected_menu_index,
            false);
    }

    s_showing_landing = false;
    prv_set_healthy_animation_visible(false);
}

static void prv_show_landing(
    const ServerStatus *status)
{
    if ((s_landing_layer == NULL) ||
        (s_landing_server_layer == NULL) ||
        (status == NULL))
    {
        return;
    }

    if (s_menu_layer != NULL)
    {
        layer_set_hidden(
            simple_menu_layer_get_layer(
                s_menu_layer),
            true);
    }

    text_layer_set_text(
        s_landing_server_layer,
        status->server_name);

    layer_set_hidden(
        s_landing_layer,
        false);

    s_showing_landing = true;
    prv_set_healthy_animation_visible(true);
}

static void prv_update_presentation(void)
{
    const ServerStatus *const status =
        server_status_service_get();

    const bool show_landing =
        prv_is_healthy_landing_state(status) &&
        !s_user_opened_menu;

    if (show_landing)
    {
        prv_show_landing(status);
    }
    else
    {
        prv_show_menu();
    }
}

static void prv_menu_up_click_handler(
    ClickRecognizerRef recognizer,
    void *context)
{
    (void)recognizer;
    (void)context;

    if (s_menu_layer == NULL)
    {
        return;
    }

    if (s_selected_menu_index > 0)
    {
        --s_selected_menu_index;

        simple_menu_layer_set_selected_index(
            s_menu_layer,
            s_selected_menu_index,
            true);
    }
}

static void prv_menu_down_click_handler(
    ClickRecognizerRef recognizer,
    void *context)
{
    (void)recognizer;
    (void)context;

    if (s_menu_layer == NULL)
    {
        return;
    }

    if (s_selected_menu_index <
        HOME_DESTINATION_COUNT - 1)
    {
        ++s_selected_menu_index;

        simple_menu_layer_set_selected_index(
            s_menu_layer,
            s_selected_menu_index,
            true);
    }
}

static void prv_menu_select_click_handler(
    ClickRecognizerRef recognizer,
    void *context)
{
    (void)recognizer;
    (void)context;

    prv_destination_selected(
        s_selected_menu_index,
        NULL);
}

static void prv_down_click_handler(
    ClickRecognizerRef recognizer,
    void *context)
{
    (void)recognizer;
    (void)context;

    if (s_showing_landing)
    {
        s_user_opened_menu = true;
        prv_show_menu();
        return;
    }

    prv_menu_down_click_handler(
        recognizer,
        context);
}

static void prv_up_click_handler(
    ClickRecognizerRef recognizer,
    void *context)
{
    if (!s_showing_landing)
    {
        prv_menu_up_click_handler(
            recognizer,
            context);
    }
}

static void prv_select_click_handler(
    ClickRecognizerRef recognizer,
    void *context)
{
    if (!s_showing_landing)
    {
        prv_menu_select_click_handler(
            recognizer,
            context);
    }
}

static void prv_back_click_handler(
    ClickRecognizerRef recognizer,
    void *context)
{
    (void)recognizer;
    (void)context;

    const ServerStatus *const status =
        server_status_service_get();

    if (!s_showing_landing &&
        s_user_opened_menu &&
        prv_is_healthy_landing_state(status))
    {
        s_user_opened_menu = false;
        prv_show_landing(status);
        return;
    }

    window_stack_pop(true);
}

static void prv_click_config_provider(
    void *context)
{
    (void)context;

    window_single_click_subscribe(
        BUTTON_ID_UP,
        prv_up_click_handler);

    window_single_click_subscribe(
        BUTTON_ID_DOWN,
        prv_down_click_handler);

    window_single_click_subscribe(
        BUTTON_ID_SELECT,
        prv_select_click_handler);

    window_single_click_subscribe(
        BUTTON_ID_BACK,
        prv_back_click_handler);
}

static void prv_window_load(
    Window *window)
{
    Layer *const window_layer =
        window_get_root_layer(window);

    const GRect bounds =
        layer_get_bounds(window_layer);

    s_user_opened_menu = false;
    s_showing_landing = false;
    s_selected_menu_index = 0;
    s_healthy_sleep_phase = 0;
    s_healthy_z_timer = NULL;

    prv_update_menu_text();

    s_landing_layer =
        layer_create(bounds);

    if (s_landing_layer != NULL)
    {
        s_landing_title_layer =
            prv_create_centered_text_layer(
                GRect(
                    0,
                    LANDING_TITLE_Y,
                    bounds.size.w,
                    LANDING_TITLE_HEIGHT),
                "ServerWatch",
                FONT_KEY_GOTHIC_24_BOLD);

        s_landing_server_layer =
            prv_create_centered_text_layer(
                GRect(
                    0,
                    LANDING_SERVER_Y,
                    bounds.size.w,
                    LANDING_SERVER_HEIGHT),
                "",
                FONT_KEY_GOTHIC_18);

        s_healthy_cat_bitmap =
            gbitmap_create_with_resource(
                RESOURCE_ID_IMAGE_HEALTHY_CAT);

        if (s_healthy_cat_bitmap != NULL)
        {
            const int16_t cat_width =
                bounds.size.w < LANDING_CAT_WIDTH
                    ? bounds.size.w
                    : LANDING_CAT_WIDTH;

            s_healthy_cat_layer =
                bitmap_layer_create(
                    GRect(
                        (bounds.size.w - cat_width) / 2,
                        LANDING_CAT_Y,
                        cat_width,
                        LANDING_CAT_HEIGHT));

            if (s_healthy_cat_layer != NULL)
            {
                bitmap_layer_set_bitmap(
                    s_healthy_cat_layer,
                    s_healthy_cat_bitmap);

                bitmap_layer_set_compositing_mode(
                    s_healthy_cat_layer,
                    GCompOpSet);

                bitmap_layer_set_alignment(
                    s_healthy_cat_layer,
                    GAlignCenter);
            }
        }

        const int16_t cat_center_x =
            bounds.size.w / 2;

        s_healthy_z_small_layer =
            prv_create_centered_text_layer(
                GRect(
                    cat_center_x +
                        LANDING_Z_SMALL_OFFSET_X,
                    LANDING_Z_SMALL_Y,
                    LANDING_Z_SMALL_WIDTH,
                    LANDING_Z_SMALL_HEIGHT),
                "z",
                FONT_KEY_GOTHIC_18_BOLD);

        s_healthy_z_medium_layer =
            prv_create_centered_text_layer(
                GRect(
                    cat_center_x +
                        LANDING_Z_MEDIUM_OFFSET_X,
                    LANDING_Z_MEDIUM_Y,
                    LANDING_Z_MEDIUM_WIDTH,
                    LANDING_Z_MEDIUM_HEIGHT),
                "Z",
                FONT_KEY_GOTHIC_24_BOLD);

        s_healthy_z_large_layer =
            prv_create_centered_text_layer(
                GRect(
                    cat_center_x +
                        LANDING_Z_LARGE_OFFSET_X,
                    LANDING_Z_LARGE_Y,
                    LANDING_Z_LARGE_WIDTH,
                    LANDING_Z_LARGE_HEIGHT),
                "Z",
                FONT_KEY_GOTHIC_28_BOLD);

        s_healthy_bubble_layer =
            layer_create(
                GRect(
                    cat_center_x +
                        LANDING_BUBBLE_OFFSET_X,
                    LANDING_BUBBLE_Y,
                    LANDING_BUBBLE_SIZE,
                    LANDING_BUBBLE_SIZE));

        if (s_healthy_bubble_layer != NULL)
        {
            layer_set_update_proc(
                s_healthy_bubble_layer,
                prv_healthy_bubble_update_proc);
        }

        s_landing_message_layer =
            prv_create_centered_text_layer(
                GRect(
                    0,
                    LANDING_CAT_Y +
                        LANDING_CAT_HEIGHT,
                    bounds.size.w,
                    LANDING_MESSAGE_HEIGHT),
                "All good!",
                FONT_KEY_GOTHIC_24_BOLD);

        s_landing_hint_layer =
            prv_create_centered_text_layer(
                GRect(
                    0,
                    bounds.size.h -
                        LANDING_HINT_HEIGHT - 4,
                    bounds.size.w,
                    LANDING_HINT_HEIGHT),
                "DOWN for menu",
                FONT_KEY_GOTHIC_14);

        if (s_landing_title_layer != NULL)
        {
            layer_add_child(
                s_landing_layer,
                text_layer_get_layer(
                    s_landing_title_layer));
        }

        if (s_landing_server_layer != NULL)
        {
            layer_add_child(
                s_landing_layer,
                text_layer_get_layer(
                    s_landing_server_layer));
        }

        if (s_healthy_cat_layer != NULL)
        {
            layer_add_child(
                s_landing_layer,
                bitmap_layer_get_layer(
                    s_healthy_cat_layer));
        }

        if (s_healthy_z_small_layer != NULL)
        {
            layer_add_child(
                s_landing_layer,
                text_layer_get_layer(
                    s_healthy_z_small_layer));
        }

        if (s_healthy_z_medium_layer != NULL)
        {
            layer_add_child(
                s_landing_layer,
                text_layer_get_layer(
                    s_healthy_z_medium_layer));
        }

        if (s_healthy_z_large_layer != NULL)
        {
            layer_add_child(
                s_landing_layer,
                text_layer_get_layer(
                    s_healthy_z_large_layer));
        }

        if (s_healthy_bubble_layer != NULL)
        {
            layer_add_child(
                s_landing_layer,
                s_healthy_bubble_layer);
        }

        if (s_landing_message_layer != NULL)
        {
            layer_add_child(
                s_landing_layer,
                text_layer_get_layer(
                    s_landing_message_layer));
        }

        if (s_landing_hint_layer != NULL)
        {
            layer_add_child(
                s_landing_layer,
                text_layer_get_layer(
                    s_landing_hint_layer));
        }

        layer_add_child(
            window_layer,
            s_landing_layer);
    }

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

    window_set_click_config_provider(
        window,
        prv_click_config_provider);

    prv_update_presentation();
}

static void prv_window_unload(
    Window *window)
{
    (void)window;

    prv_stop_healthy_z_animation();

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

    if (s_landing_hint_layer != NULL)
    {
        text_layer_destroy(
            s_landing_hint_layer);

        s_landing_hint_layer = NULL;
    }

    if (s_landing_message_layer != NULL)
    {
        text_layer_destroy(
            s_landing_message_layer);

        s_landing_message_layer = NULL;
    }

    if (s_healthy_bubble_layer != NULL)
    {
        layer_destroy(
            s_healthy_bubble_layer);

        s_healthy_bubble_layer = NULL;
    }

    if (s_healthy_z_small_layer != NULL)
    {
        text_layer_destroy(
            s_healthy_z_small_layer);

        s_healthy_z_small_layer = NULL;
    }

    if (s_healthy_z_medium_layer != NULL)
    {
        text_layer_destroy(
            s_healthy_z_medium_layer);

        s_healthy_z_medium_layer = NULL;
    }

    if (s_healthy_z_large_layer != NULL)
    {
        text_layer_destroy(
            s_healthy_z_large_layer);

        s_healthy_z_large_layer = NULL;
    }

    if (s_healthy_cat_layer != NULL)
    {
        bitmap_layer_destroy(
            s_healthy_cat_layer);

        s_healthy_cat_layer = NULL;
    }

    if (s_healthy_cat_bitmap != NULL)
    {
        gbitmap_destroy(
            s_healthy_cat_bitmap);

        s_healthy_cat_bitmap = NULL;
    }

    if (s_landing_server_layer != NULL)
    {
        text_layer_destroy(
            s_landing_server_layer);

        s_landing_server_layer = NULL;
    }

    if (s_landing_title_layer != NULL)
    {
        text_layer_destroy(
            s_landing_title_layer);

        s_landing_title_layer = NULL;
    }

    if (s_landing_layer != NULL)
    {
        layer_destroy(
            s_landing_layer);

        s_landing_layer = NULL;
    }
}

Window *home_window_create(void)
{
    Window *const window =
        window_create();

    if (window != NULL)
    {
        s_window = window;

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

        if (s_window == window)
        {
            s_window = NULL;
        }
    }
}

void home_window_refresh(void)
{
    if ((s_window == NULL) ||
        (s_menu_layer == NULL))
    {
        return;
    }

    prv_update_menu_text();

    prv_update_presentation();
}
