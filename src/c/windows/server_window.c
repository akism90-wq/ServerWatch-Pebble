#include "server_window.h"

#include <stdio.h>

#include "../services/server_status_service.h"
#include "../ui/status_row.h"

#define SERVER_CONTENT_HEIGHT 380

static ScrollLayer *s_scroll_layer;
static TextLayer *s_server_title_layer;
static TextLayer *s_metrics_layer;
static TextLayer *s_services_title_layer;
static TextLayer *s_updated_layer;

static StatusRow *s_server_status_row;
static StatusRow *s_service_rows[SERVER_SERVICE_COUNT];

static char s_metrics_text[160];
static char s_updated_text[48];

static void prv_format_metrics(const ServerStatus *status)
{
    const int cpu_tenths = (int)(status->cpu_percent * 10.0f);
    const int ram_tenths = (int)(status->ram_percent * 10.0f);
    const int temperature_tenths =
        (int)(status->temperature_celsius * 10.0f);
    const int load_hundredths =
        (int)(status->load_average * 100.0f);

    snprintf(
        s_metrics_text,
        sizeof(s_metrics_text),
        "CPU        %d.%d%%\n"
        "RAM       %d.%d%%\n"
        "Temp      %d.%d C\n"
        "Load       %d.%02d\n"
        "Uptime    %s",
        cpu_tenths / 10,
        cpu_tenths % 10,
        ram_tenths / 10,
        ram_tenths % 10,
        temperature_tenths / 10,
        temperature_tenths % 10,
        load_hundredths / 100,
        load_hundredths % 100,
        status->uptime_text
    );
}

static StatusIndicatorState prv_state_from_online(bool online)
{
    return online
        ? STATUS_INDICATOR_HEALTHY
        : STATUS_INDICATOR_CRITICAL;
}

static void prv_window_load(Window *window)
{
    Layer *const window_layer = window_get_root_layer(window);
    const GRect bounds = layer_get_bounds(window_layer);
    const ServerStatus status = server_status_service_get();

    prv_format_metrics(&status);

    snprintf(
        s_updated_text,
        sizeof(s_updated_text),
        "Updated\n%s",
        status.updated_text
    );

    s_scroll_layer = scroll_layer_create(bounds);

    scroll_layer_set_content_size(
        s_scroll_layer,
        GSize(bounds.size.w, SERVER_CONTENT_HEIGHT)
    );

    scroll_layer_set_click_config_onto_window(
        s_scroll_layer,
        window
    );

    s_server_title_layer = text_layer_create(
        GRect(0, 8, bounds.size.w, 30)
    );

    text_layer_set_text(
        s_server_title_layer,
        "Server Status"
    );

    text_layer_set_font(
        s_server_title_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD)
    );

    text_layer_set_text_alignment(
        s_server_title_layer,
        GTextAlignmentCenter
    );

    s_server_status_row = status_row_create(
        GRect(12, 42, bounds.size.w - 24, 24),
        status.server_online ? "Online" : "Offline",
        prv_state_from_online(status.server_online)
    );

    status_row_set_emphasized(
        s_server_status_row,
        true
    );

    s_metrics_layer = text_layer_create(
        GRect(12, 72, bounds.size.w - 24, 110)
    );

    text_layer_set_text(
        s_metrics_layer,
        s_metrics_text
    );

    text_layer_set_font(
        s_metrics_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_18)
    );

    text_layer_set_text_alignment(
        s_metrics_layer,
        GTextAlignmentLeft
    );

    s_services_title_layer = text_layer_create(
        GRect(0, 184, bounds.size.w, 30)
    );

    text_layer_set_text(
        s_services_title_layer,
        "Service Status"
    );

    text_layer_set_font(
        s_services_title_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD)
    );

    text_layer_set_text_alignment(
        s_services_title_layer,
        GTextAlignmentCenter
    );

    for (int index = 0;
         index < SERVER_SERVICE_COUNT;
         ++index) {
        s_service_rows[index] = status_row_create(
            GRect(
                12,
                218 + (index * 24),
                bounds.size.w - 24,
                24
            ),
            status.services[index].name,
            prv_state_from_online(status.services[index].online)
        );
    }

    s_updated_layer = text_layer_create(
        GRect(12, 364, bounds.size.w - 24, 40)
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
        text_layer_get_layer(s_server_title_layer)
    );

    if (s_server_status_row != NULL) {
        scroll_layer_add_child(
            s_scroll_layer,
            status_row_get_layer(s_server_status_row)
        );
    }

    scroll_layer_add_child(
        s_scroll_layer,
        text_layer_get_layer(s_metrics_layer)
    );

    scroll_layer_add_child(
        s_scroll_layer,
        text_layer_get_layer(s_services_title_layer)
    );

    for (int index = 0;
         index < SERVER_SERVICE_COUNT;
         ++index) {
        if (s_service_rows[index] != NULL) {
            scroll_layer_add_child(
                s_scroll_layer,
                status_row_get_layer(s_service_rows[index])
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
         index < SERVER_SERVICE_COUNT;
         ++index) {
        status_row_destroy(s_service_rows[index]);
        s_service_rows[index] = NULL;
    }

    text_layer_destroy(s_services_title_layer);
    s_services_title_layer = NULL;

    text_layer_destroy(s_metrics_layer);
    s_metrics_layer = NULL;

    status_row_destroy(s_server_status_row);
    s_server_status_row = NULL;

    text_layer_destroy(s_server_title_layer);
    s_server_title_layer = NULL;

    scroll_layer_destroy(s_scroll_layer);
    s_scroll_layer = NULL;
}

Window *server_window_create(void)
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

void server_window_destroy(Window *window)
{
    if (window != NULL) {
        window_destroy(window);
    }
}
