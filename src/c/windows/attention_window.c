#include "attention_window.h"

#include <stdio.h>

#include "../services/server_status_service.h"
#include "../ui/status_row.h"

#define ATTENTION_CONTENT_HEIGHT 190

static ScrollLayer *s_scroll_layer;

static TextLayer *s_title_layer;
static TextLayer *s_updated_layer;

static StatusRow *s_attention_rows[SERVER_ATTENTION_ITEM_COUNT];

static char s_updated_text[48];

static StatusIndicatorState prv_state_from_attention(
    AttentionSeverity severity
)
{
    switch (severity) {
        case ATTENTION_SEVERITY_INFO:
            return STATUS_INDICATOR_HEALTHY;

        case ATTENTION_SEVERITY_WARNING:
            return STATUS_INDICATOR_WARNING;

        case ATTENTION_SEVERITY_CRITICAL:
            return STATUS_INDICATOR_CRITICAL;

        default:
            return STATUS_INDICATOR_HEALTHY;
    }
}

static void prv_window_load(Window *window)
{
    Layer *const window_layer =
        window_get_root_layer(window);

    const GRect bounds =
        layer_get_bounds(window_layer);

    const ServerStatus status =
        server_status_service_get();

    snprintf(
        s_updated_text,
        sizeof(s_updated_text),
        "Updated\n%s",
        status.updated_text
    );

    s_scroll_layer = scroll_layer_create(bounds);

    scroll_layer_set_content_size(
        s_scroll_layer,
        GSize(bounds.size.w, ATTENTION_CONTENT_HEIGHT)
    );

    scroll_layer_set_click_config_onto_window(
        s_scroll_layer,
        window
    );

    s_title_layer = text_layer_create(
        GRect(0, 8, bounds.size.w, 30)
    );

    text_layer_set_text(
        s_title_layer,
        "Attention"
    );

    text_layer_set_font(
        s_title_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD)
    );

    text_layer_set_text_alignment(
        s_title_layer,
        GTextAlignmentCenter
    );

    for (int index = 0;
         index < status.attention_item_count;
         ++index) {
        const AttentionItem *const item =
            &status.attention_items[index];

        s_attention_rows[index] =
            status_row_create(
                GRect(
                    12,
                    46 + (index * 30),
                    bounds.size.w - 24,
                    24
                ),
                item->text,
                prv_state_from_attention(
                    item->severity
                )
            );
    }

    s_updated_layer = text_layer_create(
        GRect(12, 146, bounds.size.w - 24, 40)
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
        text_layer_get_layer(s_title_layer)
    );

    for (int index = 0;
         index < status.attention_item_count;
         ++index) {
        if (s_attention_rows[index] != NULL) {
            scroll_layer_add_child(
                s_scroll_layer,
                status_row_get_layer(
                    s_attention_rows[index]
                )
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
         index < SERVER_ATTENTION_ITEM_COUNT;
         ++index) {
        status_row_destroy(
            s_attention_rows[index]
        );

        s_attention_rows[index] = NULL;
    }

    text_layer_destroy(s_title_layer);
    s_title_layer = NULL;

    scroll_layer_destroy(s_scroll_layer);
    s_scroll_layer = NULL;
}

Window *attention_window_create(void)
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

void attention_window_destroy(Window *window)
{
    if (window != NULL) {
        window_destroy(window);
    }
}