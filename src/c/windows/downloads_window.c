#include "downloads_window.h"

#include <stdio.h>

#include "../services/server_status_service.h"
#include "../ui/download_card.h"

#define DOWNLOAD_CARD_COUNT SERVER_DOWNLOAD_COUNT
#define DOWNLOADS_CONTENT_HEIGHT 584

static ScrollLayer *s_scroll_layer;
static TextLayer *s_title_layer;
static TextLayer *s_updated_layer;

static DownloadCard *s_download_cards[DOWNLOAD_CARD_COUNT];

static char s_updated_text[48];

static void prv_window_load(Window *window)
{
    Layer *const window_layer =
        window_get_root_layer(window);

    const GRect bounds =
        layer_get_bounds(window_layer);

    const ServerStatus *const status = 
        server_status_service_get();

    snprintf(
        s_updated_text,
        sizeof(s_updated_text),
        "Updated\n%s",
        status->updated_text
    );

    s_scroll_layer = scroll_layer_create(bounds);

    scroll_layer_set_content_size(
        s_scroll_layer,
        GSize(bounds.size.w, DOWNLOADS_CONTENT_HEIGHT)
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
        "Downloads"
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
         index < DOWNLOAD_CARD_COUNT;
         ++index) {
        const DownloadStatus *const download =
            &status->downloads[index];

        s_download_cards[index] =
        download_card_create(
            GRect(
                12,
                44 + (index * 164),
                bounds.size.w - 24,
                156
            ),
            download->name,
            download->subtitle,
            download->state,
            download->progress_percent,
            download->speed_text,
            download->eta_text,
            download->size_mb,
            download->suspicious
        );
    }

    s_updated_layer = text_layer_create(
        GRect(12, 536, bounds.size.w - 24, 40)
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
         index < DOWNLOAD_CARD_COUNT;
         ++index) {
        if (s_download_cards[index] != NULL) {
            scroll_layer_add_child(
                s_scroll_layer,
                download_card_get_layer(
                    s_download_cards[index]
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
         index < DOWNLOAD_CARD_COUNT;
         ++index) {
        download_card_destroy(
            s_download_cards[index]
        );

        s_download_cards[index] = NULL;
    }

    text_layer_destroy(s_title_layer);
    s_title_layer = NULL;

    scroll_layer_destroy(s_scroll_layer);
    s_scroll_layer = NULL;
}

Window *downloads_window_create(void)
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

void downloads_window_refresh(void)
{
    if (s_title_layer == NULL)
    {
        return;
    }

    const ServerStatus *const status =
        server_status_service_get();

    if (status == NULL)
    {
        return;
    }

    for (int index = 0;
         index < SERVER_DOWNLOAD_COUNT;
         ++index)
    {
        DownloadCard *const card =
            s_download_cards[index];

        if (card == NULL)
        {
            continue;
        }

        const DownloadStatus *const download =
            &status->downloads[index];

        download_card_update(
            card,
            download->name,
            download->subtitle,
            download->state,
            download->progress_percent,
            download->speed_text,
            download->eta_text,
            download->size_mb,
            download->suspicious
        );
    }
}

void downloads_window_destroy(Window *window)
{
    if (window != NULL) {
        window_destroy(window);
    }
}