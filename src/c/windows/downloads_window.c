#include "downloads_window.h"

#include <stdio.h>
#include <string.h>

#include "../services/server_status_service.h"
#include "../ui/download_card.h"
#include "../ui/updated_age_view.h"
#include "../ui/connection_status_view.h"

#define DOWNLOAD_CARD_START_Y 44
#define DOWNLOAD_CARD_STRIDE 170
#define DOWNLOAD_CARD_HEIGHT 162
#define DOWNLOAD_FOOTER_GAP 8
#define DOWNLOAD_OVERFLOW_HEIGHT 24
#define DOWNLOAD_UPDATED_HEIGHT 40
#define DOWNLOAD_SELECTION_NONE (-1)

#define DOWNLOAD_CARD_COUNT SERVER_DOWNLOAD_COUNT
#define DOWNLOADS_CONTENT_HEIGHT 612
#define DOWNLOAD_FEEDBACK_DISMISS_MS 1800

static ScrollLayer *s_scroll_layer;
static TextLayer *s_title_layer;
static TextLayer *s_empty_layer;
static TextLayer *s_overflow_layer;
static UpdatedAgeView *s_updated_age_view;
static ConnectionStatusView *s_connection_status_view;

static DownloadCard *s_download_cards[DOWNLOAD_CARD_COUNT];

static char s_overflow_text[32];
static char s_pending_delete_hash[DOWNLOAD_HASH_LENGTH];
static char s_pending_delete_name[DOWNLOAD_NAME_LENGTH];
static char s_feedback_title_text[32];
static char s_feedback_body_text[96];

static int s_selected_download_index =
    DOWNLOAD_SELECTION_NONE;

static bool s_footer_visible;
static bool s_delete_request_pending;

static Window *s_confirm_window;
static TextLayer *s_confirm_title_layer;
static TextLayer *s_confirm_name_layer;
static TextLayer *s_confirm_warning_layer;
static TextLayer *s_confirm_actions_layer;

static Window *s_feedback_window;
static TextLayer *s_feedback_title_layer;
static TextLayer *s_feedback_body_layer;
static AppTimer *s_feedback_timer;

static void prv_update_selection(int visible_downloads);
static void prv_scroll_to_selected(void);
static void prv_show_feedback(
    const char *title,
    const char *body);

static int prv_visible_download_count(
    const ServerStatus *status)
{
    if (status == NULL)
    {
        return 0;
    }

    return status->active_downloads < SERVER_DOWNLOAD_COUNT
        ? status->active_downloads
        : SERVER_DOWNLOAD_COUNT;
}

static bool prv_is_live_snapshot(
    const ServerStatus *status)
{
    return (status != NULL) &&
        status->has_received_snapshot &&
        (status->connection_state ==
         CONNECTION_STATE_CONNECTED);
}

static bool prv_hash_is_visible(
    const ServerStatus *status,
    const char *hash)
{
    if ((status == NULL) ||
        (hash == NULL) ||
        (hash[0] == '\0'))
    {
        return false;
    }

    const int visible_downloads =
        prv_visible_download_count(status);

    for (int index = 0;
         index < visible_downloads;
         ++index)
    {
        if (strncmp(
                status->downloads[index].hash,
                hash,
                DOWNLOAD_HASH_LENGTH) == 0)
        {
            return true;
        }
    }

    return false;
}

static bool prv_can_delete_selected(
    const ServerStatus *status)
{
    if (!prv_is_live_snapshot(status))
    {
        return false;
    }

    const int visible_downloads =
        prv_visible_download_count(status);

    return (s_selected_download_index >= 0) &&
        (s_selected_download_index < visible_downloads) &&
        (status->downloads[
            s_selected_download_index].hash[0] != '\0');
}

static void prv_clear_pending_delete(void)
{
    s_delete_request_pending = false;
    s_pending_delete_hash[0] = '\0';
    s_pending_delete_name[0] = '\0';
}

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

static void prv_feedback_timer_callback(void *context)
{
    (void)context;

    s_feedback_timer = NULL;

    if (s_feedback_window != NULL)
    {
        window_stack_remove(
            s_feedback_window,
            true);
    }
}

static void prv_feedback_dismiss_handler(
    ClickRecognizerRef recognizer,
    void *context)
{
    (void)recognizer;
    (void)context;

    if (s_feedback_window != NULL)
    {
        window_stack_remove(
            s_feedback_window,
            true);
    }
}

static void prv_feedback_click_config_provider(
    void *context)
{
    (void)context;

    window_single_click_subscribe(
        BUTTON_ID_SELECT,
        prv_feedback_dismiss_handler);

    window_single_click_subscribe(
        BUTTON_ID_BACK,
        prv_feedback_dismiss_handler);
}

static void prv_feedback_window_load(Window *window)
{
    Layer *const window_layer =
        window_get_root_layer(window);

    const GRect bounds =
        layer_get_bounds(window_layer);

    s_feedback_title_layer =
        text_layer_create(
            GRect(8, 22, bounds.size.w - 16, 32));

    text_layer_set_text(
        s_feedback_title_layer,
        s_feedback_title_text);

    text_layer_set_font(
        s_feedback_title_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));

    text_layer_set_text_alignment(
        s_feedback_title_layer,
        GTextAlignmentCenter);

    s_feedback_body_layer =
        text_layer_create(
            GRect(10, 64, bounds.size.w - 20, 82));

    text_layer_set_text(
        s_feedback_body_layer,
        s_feedback_body_text);

    text_layer_set_font(
        s_feedback_body_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_18));

    text_layer_set_text_alignment(
        s_feedback_body_layer,
        GTextAlignmentCenter);

    layer_add_child(
        window_layer,
        text_layer_get_layer(s_feedback_title_layer));

    layer_add_child(
        window_layer,
        text_layer_get_layer(s_feedback_body_layer));

    s_feedback_timer =
        app_timer_register(
            DOWNLOAD_FEEDBACK_DISMISS_MS,
            prv_feedback_timer_callback,
            NULL);
}

static void prv_feedback_window_unload(Window *window)
{
    (void)window;

    if (s_feedback_timer != NULL)
    {
        app_timer_cancel(s_feedback_timer);
        s_feedback_timer = NULL;
    }

    text_layer_destroy(s_feedback_body_layer);
    s_feedback_body_layer = NULL;

    text_layer_destroy(s_feedback_title_layer);
    s_feedback_title_layer = NULL;

    window_destroy(s_feedback_window);
    s_feedback_window = NULL;
}

static void prv_show_feedback(
    const char *title,
    const char *body)
{
    if (s_feedback_window != NULL)
    {
        window_stack_remove(
            s_feedback_window,
            false);
    }

    snprintf(
        s_feedback_title_text,
        sizeof(s_feedback_title_text),
        "%s",
        title != NULL ? title : "");

    snprintf(
        s_feedback_body_text,
        sizeof(s_feedback_body_text),
        "%s",
        body != NULL ? body : "");

    s_feedback_window = window_create();

    if (s_feedback_window == NULL)
    {
        return;
    }

    window_set_window_handlers(
        s_feedback_window,
        (WindowHandlers) {
            .load = prv_feedback_window_load,
            .unload = prv_feedback_window_unload,
        });

    window_set_click_config_provider(
        s_feedback_window,
        prv_feedback_click_config_provider);

    window_stack_push(
        s_feedback_window,
        true);
}

static void prv_send_delete_request(void)
{
    DictionaryIterator *iterator = NULL;

    const AppMessageResult result =
        app_message_outbox_begin(&iterator);

    if ((result != APP_MSG_OK) ||
        (iterator == NULL))
    {
        prv_show_feedback(
            "Delete failed",
            "Could not contact phone.");
        prv_clear_pending_delete();
        return;
    }

    dict_write_cstring(
        iterator,
        MESSAGE_KEY_deleteDownloadHash,
        s_pending_delete_hash);

    s_delete_request_pending = true;

    const AppMessageResult send_result =
        app_message_outbox_send();

    if (send_result != APP_MSG_OK)
    {
        prv_show_feedback(
            "Delete failed",
            "Could not send request.");
        prv_clear_pending_delete();
        return;
    }

    prv_show_feedback(
        "Deleting",
        "Waiting for ServerWatch.");
}

static void prv_confirm_select_handler(
    ClickRecognizerRef recognizer,
    void *context)
{
    (void)recognizer;
    (void)context;

    const ServerStatus *const status =
        server_status_service_get();

    if (!prv_is_live_snapshot(status))
    {
        window_stack_pop(true);
        prv_show_feedback(
            "Delete blocked",
            "Connection is not live.");
        prv_clear_pending_delete();
        return;
    }

    if (!prv_hash_is_visible(
            status,
            s_pending_delete_hash))
    {
        window_stack_pop(true);
        prv_show_feedback(
            "Delete blocked",
            "Download changed.");
        prv_clear_pending_delete();
        return;
    }

    window_stack_pop(true);
    prv_send_delete_request();
}

static void prv_confirm_cancel_handler(
    ClickRecognizerRef recognizer,
    void *context)
{
    (void)recognizer;
    (void)context;

    prv_clear_pending_delete();
    window_stack_pop(true);
}

static void prv_confirm_click_config_provider(void *context)
{
    (void)context;

    window_single_click_subscribe(
        BUTTON_ID_SELECT,
        prv_confirm_select_handler);

    window_single_click_subscribe(
        BUTTON_ID_BACK,
        prv_confirm_cancel_handler);

    window_single_click_subscribe(
        BUTTON_ID_DOWN,
        prv_confirm_cancel_handler);
}

static void prv_confirm_window_load(Window *window)
{
    Layer *const window_layer =
        window_get_root_layer(window);

    const GRect bounds =
        layer_get_bounds(window_layer);

    s_confirm_title_layer =
        text_layer_create(
            GRect(6, 8, bounds.size.w - 12, 30));

    text_layer_set_text(
        s_confirm_title_layer,
        "Kill Download?");

    text_layer_set_font(
        s_confirm_title_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));

    text_layer_set_text_alignment(
        s_confirm_title_layer,
        GTextAlignmentCenter);

    s_confirm_name_layer =
        text_layer_create(
            GRect(10, 42, bounds.size.w - 20, 48));

    text_layer_set_text(
        s_confirm_name_layer,
        s_pending_delete_name);

    text_layer_set_font(
        s_confirm_name_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));

    text_layer_set_text_alignment(
        s_confirm_name_layer,
        GTextAlignmentCenter);

    s_confirm_warning_layer =
        text_layer_create(
            GRect(10, 92, bounds.size.w - 20, 44));

    text_layer_set_text(
        s_confirm_warning_layer,
        "Deletes torrent AND downloaded data.");

    text_layer_set_font(
        s_confirm_warning_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_18));

    text_layer_set_text_alignment(
        s_confirm_warning_layer,
        GTextAlignmentCenter);

    s_confirm_actions_layer =
        text_layer_create(
            GRect(10, 142, bounds.size.w - 20, 28));

    text_layer_set_text(
        s_confirm_actions_layer,
        "SELECT Confirm   BACK Cancel");

    text_layer_set_font(
        s_confirm_actions_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_14));

    text_layer_set_text_alignment(
        s_confirm_actions_layer,
        GTextAlignmentCenter);

    layer_add_child(
        window_layer,
        text_layer_get_layer(s_confirm_title_layer));

    layer_add_child(
        window_layer,
        text_layer_get_layer(s_confirm_name_layer));

    layer_add_child(
        window_layer,
        text_layer_get_layer(s_confirm_warning_layer));

    layer_add_child(
        window_layer,
        text_layer_get_layer(s_confirm_actions_layer));
}

static void prv_confirm_window_unload(Window *window)
{
    (void)window;

    text_layer_destroy(s_confirm_actions_layer);
    s_confirm_actions_layer = NULL;

    text_layer_destroy(s_confirm_warning_layer);
    s_confirm_warning_layer = NULL;

    text_layer_destroy(s_confirm_name_layer);
    s_confirm_name_layer = NULL;

    text_layer_destroy(s_confirm_title_layer);
    s_confirm_title_layer = NULL;

    window_destroy(s_confirm_window);
    s_confirm_window = NULL;
}

static void prv_select_click_handler(
    ClickRecognizerRef recognizer,
    void *context)
{
    (void)recognizer;
    (void)context;

    const ServerStatus *const status =
        server_status_service_get();

    if (!prv_can_delete_selected(status))
    {
        prv_show_feedback(
            "Delete blocked",
            "Live download required.");
        prv_clear_pending_delete();
        return;
    }

    const DownloadStatus *const download =
        &status->downloads[s_selected_download_index];

    snprintf(
        s_pending_delete_hash,
        sizeof(s_pending_delete_hash),
        "%s",
        download->hash);

    snprintf(
        s_pending_delete_name,
        sizeof(s_pending_delete_name),
        "%s",
        download->name);

    s_confirm_window = window_create();

    if (s_confirm_window == NULL)
    {
        prv_clear_pending_delete();
        return;
    }

    window_set_window_handlers(
        s_confirm_window,
        (WindowHandlers) {
            .load = prv_confirm_window_load,
            .unload = prv_confirm_window_unload,
        });

    window_set_click_config_provider(
        s_confirm_window,
        prv_confirm_click_config_provider);

    window_stack_push(
        s_confirm_window,
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

    window_single_click_subscribe(
        BUTTON_ID_SELECT,
        prv_select_click_handler);
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
            download->quality,
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

    s_connection_status_view =
        connection_status_view_create(
            GRect(12, 604, bounds.size.w - 24, 44)
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

    if (s_connection_status_view != NULL)
    {
        scroll_layer_add_child(
            s_scroll_layer,
            connection_status_view_get_layer(
                s_connection_status_view)
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

    connection_status_view_destroy(
        s_connection_status_view);
    s_connection_status_view = NULL;

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
    int16_t footer_y;

    if (visible_downloads == 0)
    {
        footer_y = 104;
    }
    else
    {
        footer_y =
            DOWNLOAD_CARD_START_Y +
            (visible_downloads * DOWNLOAD_CARD_STRIDE);
    }

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

    const bool has_snapshot =
        status->has_received_snapshot;

    /*
     * No successful snapshot has ever been received.
     * Never expose development seed data as genuine data.
     */
    if (!has_snapshot)
    {
        if (s_empty_layer != NULL)
        {
            layer_set_hidden(
                text_layer_get_layer(s_empty_layer),
                true
            );
        }

        if (s_overflow_layer != NULL)
        {
            layer_set_hidden(
                text_layer_get_layer(s_overflow_layer),
                true
            );
        }

        for (int index = 0;
             index < SERVER_DOWNLOAD_COUNT;
             ++index)
        {
            if (s_download_cards[index] == NULL)
            {
                continue;
            }

            layer_set_hidden(
                download_card_get_layer(
                    s_download_cards[index]),
                true
            );
        }

        if (s_updated_age_view != NULL)
        {
            layer_set_hidden(
                updated_age_view_get_layer(
                    s_updated_age_view),
                true
            );
        }

        prv_update_selection(0);

        connection_status_view_refresh(
            s_connection_status_view
        );

        /*
         * Connection failure is the only meaningful
         * content before the first valid snapshot.
         */
        if (s_connection_status_view != NULL)
        {
            Layer *const connection_layer =
                connection_status_view_get_layer(
                    s_connection_status_view);

            GRect connection_frame =
                layer_get_frame(connection_layer);

            connection_frame.origin.y = 52;

            layer_set_frame(
                connection_layer,
                connection_frame
            );
        }

        const GRect scroll_bounds =
            layer_get_bounds(
                scroll_layer_get_layer(
                    s_scroll_layer));

        scroll_layer_set_content_size(
            s_scroll_layer,
            GSize(
                scroll_bounds.size.w,
                scroll_bounds.size.h)
        );

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
     * A genuine zero-download snapshot may show the
     * normal empty state.
     */
    if (s_empty_layer != NULL)
    {
        layer_set_hidden(
            text_layer_get_layer(s_empty_layer),
            active_downloads != 0
        );
    }

    /*
     * Update and expose the cards represented by the
     * last-good snapshot.
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
            download->quality,
            download->state,
            download->progress_percent,
            download->speed_text,
            download->eta_text,
            download->size_mb,
            download->suspicious
        );
    }

    /*
     * Preserve the real Agent count even though Pebble
     * displays at most three download cards.
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
                    s_overflow_layer),
                false
            );
        }
        else
        {
            layer_set_hidden(
                text_layer_get_layer(
                    s_overflow_layer),
                true
            );
        }
    }

    const bool connection_lost =
        status->connection_state ==
        CONNECTION_STATE_FAILED;

    /*
     * Connected:
     *     Updated
     *     Just now
     *
     * Connection lost:
     *     Connection lost
     *     Cached - 2 min ago
     *
     * Never show both blocks simultaneously.
     */
    if (s_updated_age_view != NULL)
    {
        layer_set_hidden(
            updated_age_view_get_layer(
                s_updated_age_view),
            connection_lost
        );

        if (!connection_lost)
        {
            updated_age_view_refresh(
                s_updated_age_view
            );
        }
    }

    connection_status_view_refresh(
        s_connection_status_view
    );

    prv_update_selection(
        visible_downloads
    );

    /*
     * Establish the normal footer position. The connection
     * footer will reuse the Updated footer position when stale.
     */
    prv_update_layout(
        visible_downloads,
        overflow_count
    );

    if (connection_lost &&
        (s_connection_status_view != NULL) &&
        (s_updated_age_view != NULL))
    {
        Layer *const updated_layer =
            updated_age_view_get_layer(
                s_updated_age_view);

        Layer *const connection_layer =
            connection_status_view_get_layer(
                s_connection_status_view);

        const GRect updated_frame =
            layer_get_frame(updated_layer);

        GRect connection_frame =
            layer_get_frame(connection_layer);

        /*
         * Replace UpdatedAgeView in-place rather than
         * appending another footer underneath it.
         */
        connection_frame.origin.y =
            updated_frame.origin.y;

        layer_set_frame(
            connection_layer,
            connection_frame
        );

        const GRect scroll_bounds =
            layer_get_bounds(
                scroll_layer_get_layer(
                    s_scroll_layer));

        const int16_t content_height =
            connection_frame.origin.y +
            connection_frame.size.h +
            DOWNLOAD_FOOTER_GAP;

        scroll_layer_set_content_size(
            s_scroll_layer,
            GSize(
                scroll_bounds.size.w,
                content_height)
        );

        /*
         * Background polling must not disturb footer mode.
         */
        if (s_footer_visible)
        {
            int16_t target_y =
                content_height -
                scroll_bounds.size.h;

            if (target_y < 0)
            {
                target_y = 0;
            }

            scroll_layer_set_content_offset(
                s_scroll_layer,
                GPoint(0, -target_y),
                false
            );
        }
    }
}

void downloads_window_destroy(Window *window)
{
    if (window != NULL) {
        window_destroy(window);
    }
}

void downloads_window_handle_delete_result(
    bool success,
    const char *error_message)
{
    if (!s_delete_request_pending)
    {
        return;
    }

    prv_clear_pending_delete();

    if (success)
    {
        prv_show_feedback(
            "Delete sent",
            "Refreshing downloads.");
    }
    else
    {
        prv_show_feedback(
            "Delete failed",
            (error_message != NULL) &&
                (error_message[0] != '\0')
                    ? error_message
                    : "ServerWatch kept the current data.");
    }
}
