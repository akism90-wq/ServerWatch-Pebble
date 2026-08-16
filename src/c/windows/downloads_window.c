#include "downloads_window.h"

#include <stdio.h>

#include "../services/server_status_service.h"
#include "../ui/download_card.h"
#include "../ui/updated_age_view.h"

#define DOWNLOAD_CARD_START_Y 44
#define DOWNLOAD_CARD_STRIDE 170
#define DOWNLOAD_CARD_HEIGHT 162
#define DOWNLOAD_FOOTER_GAP 8
#define DOWNLOAD_OVERFLOW_HEIGHT 24
#define DOWNLOAD_UPDATED_HEIGHT 40
#define DOWNLOAD_SELECTION_NONE (-1)

#define DOWNLOAD_CARD_COUNT SERVER_DOWNLOAD_COUNT
#define DOWNLOADS_CONTENT_HEIGHT 612

static ScrollLayer *s_scroll_layer;
static TextLayer *s_title_layer;
static TextLayer *s_empty_layer;
static TextLayer *s_overflow_layer;
static UpdatedAgeView *s_updated_age_view;

static DownloadCard *s_download_cards[DOWNLOAD_CARD_COUNT];

static char s_overflow_text[32];

static int s_selected_download_index =
    DOWNLOAD_SELECTION_NONE;

static bool s_footer_visible;

static void prv_update_selection(
    int visible_downloads
);

static void prv_scroll_to_selected(void);

static void prv_up_click_handler(
    ClickRecognizerRef recognizer,
    void *context)
{
    (void)recognizer;
    (void)context;

    const ServerStatus *const status =
        server_status_service_get();

    const int visible_downloads =
        status->active_downloads < SERVER_DOWNLOAD_COUNT
            ? status->active_downloads
            : SERVER_DOWNLOAD_COUNT;

    if (visible_downloads <= 0)
    {
        return;
    }

    /*
     * If the viewport is currently showing the footer,
     * return to the selected final download first.
     */
    if (s_footer_visible)
    {
        s_footer_visible = false;

        prv_scroll_to_selected();
        return;
    }

    if (s_selected_download_index > 0)
    {
        --s_selected_download_index;

        prv_update_selection(
            visible_downloads);
    }
}

static void prv_down_click_handler(
    ClickRecognizerRef recognizer,
    void *context)
{
    (void)recognizer;
    (void)context;

    const ServerStatus *const status =
        server_status_service_get();

    const int visible_downloads =
        status->active_downloads < SERVER_DOWNLOAD_COUNT
            ? status->active_downloads
            : SERVER_DOWNLOAD_COUNT;

    if (visible_downloads <= 0)
    {
        return;
    }

    if (s_footer_visible)
    {
        return;
    }

    if (s_selected_download_index <
        (visible_downloads - 1))
    {
        ++s_selected_download_index;

        prv_update_selection(
            visible_downloads);

        return;
    }

    /*
     * Already on the final selectable download.
     * Keep it selected, but move the viewport down
     * to expose the non-selectable footer.
     */
    s_footer_visible = true;

    const GSize content_size =
        scroll_layer_get_content_size(
            s_scroll_layer);

    const GRect bounds =
        layer_get_bounds(
            scroll_layer_get_layer(
                s_scroll_layer));

    int16_t target_y =
        content_size.h - bounds.size.h;

    if (target_y < 0)
    {
        target_y = 0;
    }

    scroll_layer_set_content_offset(
        s_scroll_layer,
        GPoint(0, -target_y),
        true);
}

static void prv_click_config_provider(void *context)
{
    (void)context;

    window_single_click_subscribe(
        BUTTON_ID_UP,
        prv_up_click_handler);

    window_single_click_subscribe(
        BUTTON_ID_DOWN,
        prv_down_click_handler);
}

static void prv_window_load(Window *window)
{
    s_footer_visible = false;

    Layer *const window_layer =
        window_get_root_layer(window);

    const GRect bounds =
        layer_get_bounds(window_layer);

    const ServerStatus *const status = 
        server_status_service_get();

    s_scroll_layer = scroll_layer_create(bounds);

    scroll_layer_set_content_size(
        s_scroll_layer,
        GSize(bounds.size.w, DOWNLOADS_CONTENT_HEIGHT)
    );

    window_set_click_config_provider(
        window,
        prv_click_config_provider
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

    s_empty_layer = text_layer_create(
        GRect(12, 56, bounds.size.w - 24, 48)
    );

    text_layer_set_text(
        s_empty_layer,
        "No active downloads"
    );

    text_layer_set_font(
        s_empty_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_18)
    );

    text_layer_set_text_alignment(
        s_empty_layer,
        GTextAlignmentCenter
    );

    s_overflow_layer = text_layer_create(
        GRect(12, 536, bounds.size.w - 24, 24)
    );

    text_layer_set_font(
        s_overflow_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_14)
    );

    text_layer_set_text_alignment(
        s_overflow_layer,
        GTextAlignmentLeft
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
                44 + (index * 170),
                bounds.size.w - 24,
                162
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

    s_updated_age_view = updated_age_view_create(
        GRect(12, 564, bounds.size.w - 24, 40)
    );

    scroll_layer_add_child(
        s_scroll_layer,
        text_layer_get_layer(s_title_layer)
    );

    if (s_empty_layer != NULL)
    {
        scroll_layer_add_child(
            s_scroll_layer,
            text_layer_get_layer(s_empty_layer)
        );
    }

    if (s_overflow_layer != NULL)
    {
        scroll_layer_add_child(
            s_scroll_layer,
            text_layer_get_layer(s_overflow_layer)
        );
    }

    for (int index = 0;
        index < DOWNLOAD_CARD_COUNT;
        ++index)
    {
        if (s_download_cards[index] != NULL)
        {
            scroll_layer_add_child(
                s_scroll_layer,
                download_card_get_layer(
                    s_download_cards[index]
                )
            );
        }
    }

    if (s_updated_age_view != NULL)
    {
        scroll_layer_add_child(
            s_scroll_layer,
            updated_age_view_get_layer(
                s_updated_age_view)
        );
    }

    layer_add_child(
        window_layer,
        scroll_layer_get_layer(s_scroll_layer)
    );

    downloads_window_refresh();
}

static void prv_window_unload(Window *window)
{
    s_footer_visible = false;

    updated_age_view_destroy(
        s_updated_age_view);

    s_updated_age_view = NULL;

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

static void prv_update_layout(
    int visible_downloads,
    int overflow_count)
{
    int16_t footer_y =
        DOWNLOAD_CARD_START_Y +
        (visible_downloads * DOWNLOAD_CARD_STRIDE);

    if (overflow_count > 0)
    {
        Layer *const overflow_layer =
            text_layer_get_layer(s_overflow_layer);

        GRect overflow_frame =
            layer_get_frame(overflow_layer);

        overflow_frame.origin.y =
            footer_y + DOWNLOAD_FOOTER_GAP;

        layer_set_frame(
            overflow_layer,
            overflow_frame);

        footer_y =
            overflow_frame.origin.y +
            DOWNLOAD_OVERFLOW_HEIGHT;
    }

    if (s_updated_age_view != NULL)
    {
        Layer *const updated_layer =
            updated_age_view_get_layer(
                s_updated_age_view);

        GRect updated_frame =
            layer_get_frame(updated_layer);

        updated_frame.origin.y =
            footer_y + DOWNLOAD_FOOTER_GAP;

        layer_set_frame(
            updated_layer,
            updated_frame);

        footer_y =
            updated_frame.origin.y +
            DOWNLOAD_UPDATED_HEIGHT;
    }

    const GRect scroll_bounds =
        layer_get_bounds(
            scroll_layer_get_layer(
                s_scroll_layer));

    scroll_layer_set_content_size(
        s_scroll_layer,
        GSize(
            scroll_bounds.size.w,
            footer_y + DOWNLOAD_FOOTER_GAP));
}

static void prv_update_selection(
    int visible_downloads
)
{
    if (visible_downloads <= 0)
    {
        s_selected_download_index =
            DOWNLOAD_SELECTION_NONE;
    }
    else if ((s_selected_download_index < 0) ||
             (s_selected_download_index >= visible_downloads))
    {
        s_selected_download_index = 0;
    }

    for (int index = 0;
         index < DOWNLOAD_CARD_COUNT;
         ++index)
    {
        if (s_download_cards[index] == NULL)
        {
            continue;
        }

        download_card_set_selected(
            s_download_cards[index],
            index == s_selected_download_index
        );
    }

    if (!s_footer_visible)
    {
        prv_scroll_to_selected();
    }
}

static void prv_scroll_to_selected(void)
{
    if ((s_scroll_layer == NULL) ||
        (s_selected_download_index < 0))
    {
        return;
    }

    const int16_t card_y =
        DOWNLOAD_CARD_START_Y +
        (s_selected_download_index *
         DOWNLOAD_CARD_STRIDE);

    /*
     * Keep a little space above the selected card
     * instead of pinning it hard against the top edge.
     */
    int16_t target_y =
        card_y - 8;

    if (target_y < 0)
    {
        target_y = 0;
    }

    scroll_layer_set_content_offset(
        s_scroll_layer,
        GPoint(0, -target_y),
        true
    );
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

    const int active_downloads =
        status->active_downloads;

    const int visible_downloads =
        active_downloads < SERVER_DOWNLOAD_COUNT
            ? active_downloads
            : SERVER_DOWNLOAD_COUNT;

    const int overflow_count =
        active_downloads > SERVER_DOWNLOAD_COUNT
            ? active_downloads - SERVER_DOWNLOAD_COUNT
            : 0;

    /*
     * Show the empty-state message only when there
     * are no active downloads.
     */
    if (s_empty_layer != NULL)
    {
        layer_set_hidden(
            text_layer_get_layer(s_empty_layer),
            active_downloads != 0
        );
    }

    /*
     * Update the currently visible download cards.
     * Cards beyond the current active count remain
     * allocated but are hidden.
     */
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

        Layer *const card_layer =
            download_card_get_layer(card);

        const bool visible =
            index < visible_downloads;

        layer_set_hidden(
            card_layer,
            !visible
        );

        if (!visible)
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

    /*
     * Show overflow information when the Agent
     * reports more downloads than Pebble can display.
     */
    if (s_overflow_layer != NULL)
    {
        if (overflow_count > 0)
        {
            snprintf(
                s_overflow_text,
                sizeof(s_overflow_text),
                "+%d more active",
                overflow_count
            );

            text_layer_set_text(
                s_overflow_layer,
                s_overflow_text
            );

            layer_set_hidden(
                text_layer_get_layer(
                    s_overflow_layer
                ),
                false
            );
        }
        else
        {
            layer_set_hidden(
                text_layer_get_layer(
                    s_overflow_layer
                ),
                true
            );
        }
    }

    /*
     * Keep the shared snapshot age current whenever
     * a new Agent snapshot arrives.
     */
    updated_age_view_refresh(
        s_updated_age_view
    );

    prv_update_selection(
        visible_downloads
    );

    /*
     * Position the overflow and Updated footer below
     * however many download cards are currently shown.
     */
    prv_update_layout(
        visible_downloads,
        overflow_count
    );
}

void downloads_window_destroy(Window *window)
{
    if (window != NULL) {
        window_destroy(window);
    }
}