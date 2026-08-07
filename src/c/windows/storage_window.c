#include "storage_window.h"

#include <stdio.h>

#include "../services/server_status_service.h"
#include "../ui/metric_row.h"

#define STORAGE_METRIC_COUNT 8
#define STORAGE_CONTENT_HEIGHT 420

typedef enum
{
    STORAGE_USED = 0,
    STORAGE_FREE,
    STORAGE_TOTAL,

    STORAGE_MOVIES,
    STORAGE_TV,
    STORAGE_IMMICH,
    STORAGE_DOWNLOADS,
    STORAGE_OTHER
} StorageMetric;

static ScrollLayer *s_scroll_layer;

static TextLayer *s_storage_title_layer;
static TextLayer *s_media_title_layer;
static TextLayer *s_updated_layer;

static MetricRow *s_metric_rows[STORAGE_METRIC_COUNT];

static char s_used_text[16];
static char s_free_text[16];
static char s_total_text[16];

static char s_movies_text[16];
static char s_tv_text[16];
static char s_immich_text[16];
static char s_downloads_text[16];
static char s_other_text[16];

static char s_updated_text[48];

static void prv_format_metric_values(
    const ServerStatus *status
)
{
    const int used_tenths =
        (int)(status->storage_used_tb * 10.0f);
    const int free_tenths =
        (int)(status->storage_free_tb * 10.0f);
    const int total_tenths =
        (int)(status->storage_total_tb * 10.0f);

    snprintf(
        s_used_text,
        sizeof(s_used_text),
        "%d.%d TB",
        used_tenths / 10,
        used_tenths % 10
    );

    snprintf(
        s_free_text,
        sizeof(s_free_text),
        "%d.%d TB",
        free_tenths / 10,
        free_tenths % 10
    );

    snprintf(
        s_total_text,
        sizeof(s_total_text),
        "%d.%d TB",
        total_tenths / 10,
        total_tenths % 10
    );

    snprintf(
        s_movies_text,
        sizeof(s_movies_text),
        "%d GB",
        (int)status->movies_gb
    );

    snprintf(
        s_tv_text,
        sizeof(s_tv_text),
        "%d GB",
        (int)status->tv_gb
    );

    snprintf(
        s_immich_text,
        sizeof(s_immich_text),
        "%d GB",
        (int)status->immich_gb
    );

    snprintf(
        s_downloads_text,
        sizeof(s_downloads_text),
        "%d GB",
        (int)status->downloads_gb
    );

    snprintf(
        s_other_text,
        sizeof(s_other_text),
        "%d GB",
        (int)status->other_gb
    );
}

static void prv_window_load(Window *window)
{
    Layer *const window_layer =
        window_get_root_layer(window);
    const GRect bounds =
        layer_get_bounds(window_layer);
    const ServerStatus status =
        server_status_service_get();

    prv_format_metric_values(&status);

    snprintf(
        s_updated_text,
        sizeof(s_updated_text),
        "Updated\n%s",
        status.updated_text
    );

    s_scroll_layer = scroll_layer_create(bounds);

    scroll_layer_set_content_size(
        s_scroll_layer,
        GSize(bounds.size.w, STORAGE_CONTENT_HEIGHT)
    );

    scroll_layer_set_click_config_onto_window(
        s_scroll_layer,
        window
    );

    s_storage_title_layer = text_layer_create(
        GRect(0, 8, bounds.size.w, 30)
    );

    text_layer_set_text(
        s_storage_title_layer,
        "Storage"
    );

    text_layer_set_font(
        s_storage_title_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD)
    );

    text_layer_set_text_alignment(
        s_storage_title_layer,
        GTextAlignmentCenter
    );

    s_metric_rows[STORAGE_USED] = metric_row_create(
        GRect(12, 42, bounds.size.w - 24, 24),
        "Used",
        s_used_text
    );

    s_metric_rows[STORAGE_FREE] = metric_row_create(
        GRect(12, 66, bounds.size.w - 24, 24),
        "Free",
        s_free_text
    );

    s_metric_rows[STORAGE_TOTAL] = metric_row_create(
        GRect(12, 90, bounds.size.w - 24, 24),
        "Total",
        s_total_text
    );

    s_media_title_layer = text_layer_create(
        GRect(0, 124, bounds.size.w, 30)
    );

    text_layer_set_text(
        s_media_title_layer,
        "Media"
    );

    text_layer_set_font(
        s_media_title_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD)
    );

    text_layer_set_text_alignment(
        s_media_title_layer,
        GTextAlignmentCenter
    );

    s_metric_rows[STORAGE_MOVIES] = metric_row_create(
        GRect(12, 158, bounds.size.w - 24, 24),
        "Movies",
        s_movies_text
    );

    s_metric_rows[STORAGE_TV] = metric_row_create(
        GRect(12, 182, bounds.size.w - 24, 24),
        "TV",
        s_tv_text
    );

    s_metric_rows[STORAGE_IMMICH] = metric_row_create(
        GRect(12, 206, bounds.size.w - 24, 24),
        "Immich",
        s_immich_text
    );

    s_metric_rows[STORAGE_DOWNLOADS] = metric_row_create(
        GRect(12, 230, bounds.size.w - 24, 24),
        "Downloads",
        s_downloads_text
    );

    s_metric_rows[STORAGE_OTHER] = metric_row_create(
        GRect(12, 254, bounds.size.w - 24, 24),
        "Other",
        s_other_text
    );

    s_updated_layer = text_layer_create(
        GRect(12, 298, bounds.size.w - 24, 40)
    );

    text_layer_set_text(
        s_updated_layer,
        s_updated_text
    );

    text_layer_set_font(
        s_updated_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_14)
    );

    text_layer_set_text_alignment(
        s_updated_layer,
        GTextAlignmentLeft
    );

    scroll_layer_add_child(
        s_scroll_layer,
        text_layer_get_layer(s_storage_title_layer)
    );

    for (int index = STORAGE_USED;
         index <= STORAGE_TOTAL;
         ++index) {
        if (s_metric_rows[index] != NULL) {
            scroll_layer_add_child(
                s_scroll_layer,
                metric_row_get_layer(s_metric_rows[index])
            );
        }
    }

    scroll_layer_add_child(
        s_scroll_layer,
        text_layer_get_layer(s_media_title_layer)
    );

    for (int index = STORAGE_MOVIES;
         index < STORAGE_METRIC_COUNT;
         ++index) {
        if (s_metric_rows[index] != NULL) {
            scroll_layer_add_child(
                s_scroll_layer,
                metric_row_get_layer(s_metric_rows[index])
            );
        }
    }

    scroll_layer_add_child(
        s_scroll_layer,
        text_layer_get_layer(s_updated_layer)
    );

    layer_add_child(
        window_layer,
        scroll_layer_get_layer(s_scroll_layer)
    );
}

static void prv_window_unload(Window *window)
{
    text_layer_destroy(s_updated_layer);
    s_updated_layer = NULL;

    for (int index = 0;
         index < STORAGE_METRIC_COUNT;
         ++index) {
        metric_row_destroy(s_metric_rows[index]);
        s_metric_rows[index] = NULL;
    }

    text_layer_destroy(s_media_title_layer);
    s_media_title_layer = NULL;

    text_layer_destroy(s_storage_title_layer);
    s_storage_title_layer = NULL;

    scroll_layer_destroy(s_scroll_layer);
    s_scroll_layer = NULL;
}

Window *storage_window_create(void)
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

void storage_window_destroy(Window *window)
{
    if (window != NULL) {
        window_destroy(window);
    }
}