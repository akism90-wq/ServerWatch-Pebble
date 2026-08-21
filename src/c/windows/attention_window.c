#include "attention_window.h"

#include <stdio.h>

#include "../services/server_status_service.h"
#include "../ui/status_row.h"

#define ATTENTION_ITEMS_PER_PAGE 4
#define ATTENTION_MAX_PAGES 2

#define TITLE_Y 8
#define TITLE_HEIGHT 30

#define TITLE_DIVIDER_X 12
#define TITLE_DIVIDER_Y 39
#define TITLE_DIVIDER_HEIGHT 1

#define STATUS_Y 42
#define STATUS_HEIGHT 24

#define STATUS_DETAIL_Y 66
#define STATUS_DETAIL_HEIGHT 20

#define HEALTHY_CAT_X 18
#define HEALTHY_CAT_Y 96
#define HEALTHY_CAT_WIDTH 164
#define HEALTHY_CAT_HEIGHT 100

#define HEALTHY_Z_SMALL_X 82
#define HEALTHY_Z_SMALL_Y 78
#define HEALTHY_Z_SMALL_WIDTH 18
#define HEALTHY_Z_SMALL_HEIGHT 22

#define HEALTHY_Z_MEDIUM_X 100
#define HEALTHY_Z_MEDIUM_Y 68
#define HEALTHY_Z_MEDIUM_WIDTH 22
#define HEALTHY_Z_MEDIUM_HEIGHT 26

#define HEALTHY_Z_LARGE_X 122
#define HEALTHY_Z_LARGE_Y 55
#define HEALTHY_Z_LARGE_WIDTH 28
#define HEALTHY_Z_LARGE_HEIGHT 32

#define HEALTHY_BUBBLE_X 132
#define HEALTHY_BUBBLE_Y 54
#define HEALTHY_BUBBLE_SIZE 10

#define PAGE_VIEWPORT_Y 46
#define ROW_HEIGHT 24
#define ROW_STRIDE 28
#define PAGE_VIEWPORT_HEIGHT \
    (ATTENTION_ITEMS_PER_PAGE * ROW_STRIDE)

#define PAGE_INDICATOR_HEIGHT 20
#define PAGE_DOT_RADIUS 3
#define PAGE_DOT_SPACING 8

#define PAGE_ANIMATION_DURATION_MS 250
#define HEALTHY_Z_ANIMATION_PERIOD_MS 650

static Layer *s_page_viewport_layer;
static Layer *s_page_layers[ATTENTION_MAX_PAGES];
static Layer *s_page_indicator_layer;
static Layer *s_title_divider_layer;

static TextLayer *s_title_layer;
static TextLayer *s_status_detail_layer;

static GBitmap *s_healthy_cat_bitmap;
static BitmapLayer *s_healthy_cat_layer;
static TextLayer *s_healthy_z_small_layer;
static TextLayer *s_healthy_z_medium_layer;
static TextLayer *s_healthy_z_large_layer;
static Layer *s_healthy_bubble_layer;

static StatusRow *s_summary_status_row;

static StatusRow *s_attention_rows
    [ATTENTION_MAX_PAGES]
    [ATTENTION_ITEMS_PER_PAGE];

static int s_current_page;
static int s_target_page;

static bool s_is_animating;
static int s_healthy_sleep_phase;
static AppTimer *s_healthy_z_timer;

static char s_status_detail_text[48];

static StatusIndicatorState prv_state_from_attention(
    AttentionSeverity severity)
{
    switch (severity)
    {
    case ATTENTION_SEVERITY_WARNING:
        return STATUS_INDICATOR_WARNING;

    case ATTENTION_SEVERITY_CRITICAL:
        return STATUS_INDICATOR_CRITICAL;

    case ATTENTION_SEVERITY_INFO:
    default:
        return STATUS_INDICATOR_HEALTHY;
    }
}

static int prv_get_page_count(
    const ServerStatus *status)
{
    if ((status == NULL) ||
        (status->attention_item_count <=
         ATTENTION_ITEMS_PER_PAGE))
    {
        return 1;
    }

    return ATTENTION_MAX_PAGES;
}

static void prv_destroy_page_rows(
    int page)
{
    if ((page < 0) ||
        (page >= ATTENTION_MAX_PAGES))
    {
        return;
    }

    for (int row = 0;
         row < ATTENTION_ITEMS_PER_PAGE;
         ++row)
    {
        status_row_destroy(
            s_attention_rows[page][row]);

        s_attention_rows[page][row] = NULL;
    }
}

static void prv_destroy_all_rows(void)
{
    for (int page = 0;
         page < ATTENTION_MAX_PAGES;
         ++page)
    {
        prv_destroy_page_rows(page);
    }
}

static void prv_title_divider_update_proc(
    Layer *layer,
    GContext *context)
{
    const GRect bounds =
        layer_get_bounds(layer);

    graphics_context_set_fill_color(
        context,
        GColorBlack);

    graphics_fill_rect(
        context,
        bounds,
        0,
        GCornerNone);
}

static void prv_page_indicator_update_proc(
    Layer *layer,
    GContext *context)
{
    const ServerStatus *const status =
        server_status_service_get();

    if ((status == NULL) ||
        (!status->has_received_snapshot) ||
        (status->attention_item_count <= 0) ||
        (prv_get_page_count(status) <= 1))
    {
        return;
    }

    const GRect bounds =
        layer_get_bounds(layer);

    const int16_t centre_x =
        bounds.size.w / 2;

    const int16_t centre_y =
        bounds.size.h / 2;

    graphics_context_set_stroke_color(
        context,
        GColorBlack);

    graphics_context_set_fill_color(
        context,
        GColorBlack);

    for (int page = 0;
         page < ATTENTION_MAX_PAGES;
         ++page)
    {
        const int16_t x =
            centre_x +
            ((page == 0)
                 ? -PAGE_DOT_SPACING
                 : PAGE_DOT_SPACING);

        if (page == s_current_page)
        {
            graphics_fill_circle(
                context,
                GPoint(x, centre_y),
                PAGE_DOT_RADIUS);
        }
        else
        {
            graphics_draw_circle(
                context,
                GPoint(x, centre_y),
                PAGE_DOT_RADIUS);
        }
    }
}

static void prv_update_page_indicator(void)
{
    const ServerStatus *const status =
        server_status_service_get();

    if (s_page_indicator_layer == NULL)
    {
        return;
    }

    const bool visible =
        (status != NULL) &&
        status->has_received_snapshot &&
        (status->attention_item_count > 0) &&
        (prv_get_page_count(status) > 1);

    layer_set_hidden(
        s_page_indicator_layer,
        !visible);

    if (visible)
    {
        layer_mark_dirty(
            s_page_indicator_layer);
    }
}

static void prv_set_viewport_page(
    int page)
{
    if (s_page_viewport_layer == NULL)
    {
        return;
    }

    const GRect bounds =
        layer_get_bounds(
            s_page_viewport_layer);

    const int16_t width =
        bounds.size.w;

    layer_set_bounds(
        s_page_viewport_layer,
        GRect(
            -(page * width),
            0,
            width,
            PAGE_VIEWPORT_HEIGHT));
}

static void prv_build_page(
    int page,
    const ServerStatus *status)
{
    if ((status == NULL) ||
        (page < 0) ||
        (page >= ATTENTION_MAX_PAGES) ||
        (s_page_layers[page] == NULL))
    {
        return;
    }

    prv_destroy_page_rows(page);

    const int first_item =
        page * ATTENTION_ITEMS_PER_PAGE;

    for (int row = 0;
         row < ATTENTION_ITEMS_PER_PAGE;
         ++row)
    {
        const int item_index =
            first_item + row;

        if (item_index >=
            status->attention_item_count)
        {
            break;
        }

        const AttentionItem *const item =
            &status->attention_items[item_index];

        const GRect page_bounds =
            layer_get_bounds(
                s_page_layers[page]);

        s_attention_rows[page][row] =
            status_row_create(
                GRect(
                    12,
                    row * ROW_STRIDE,
                    page_bounds.size.w - 24,
                    ROW_HEIGHT),
                item->text,
                prv_state_from_attention(
                    item->severity));

        if (s_attention_rows[page][row] != NULL)
        {
            layer_add_child(
                s_page_layers[page],
                status_row_get_layer(
                    s_attention_rows[page][row]));
        }
    }
}

static void prv_update_summary(
    const ServerStatus *status)
{
    if ((status == NULL) ||
        (s_summary_status_row == NULL) ||
        (s_status_detail_layer == NULL))
    {
        return;
    }

    Layer *const summary_layer =
        status_row_get_layer(
            s_summary_status_row);

    Layer *const detail_layer =
        text_layer_get_layer(
            s_status_detail_layer);

    /*
     * Cold-start failure:
     * no genuine snapshot has ever been received.
     */
    if (!status->has_received_snapshot)
    {
        layer_set_hidden(
            summary_layer,
            false);

        layer_set_hidden(
            detail_layer,
            true);

        status_row_set_text(
            s_summary_status_row,
            "Connection failed");

        status_row_set_state(
            s_summary_status_row,
            STATUS_INDICATOR_CRITICAL);

        return;
    }

    /*
     * A genuine snapshot exists but communication with the
     * Agent has subsequently failed. Cached attention data
     * remains visible below.
     */
    if (status->connection_state ==
        CONNECTION_STATE_FAILED)
    {
        layer_set_hidden(
            summary_layer,
            false);

        layer_set_hidden(
            detail_layer,
            false);

        status_row_set_text(
            s_summary_status_row,
            "Connection lost");

        status_row_set_state(
            s_summary_status_row,
            STATUS_INDICATOR_CRITICAL);

        char age_text[24];

        server_status_service_format_update_age_value(
            age_text,
            sizeof(age_text));

        snprintf(
            s_status_detail_text,
            sizeof(s_status_detail_text),
            "Cached - %s",
            age_text);

        text_layer_set_text(
            s_status_detail_layer,
            s_status_detail_text);

        return;
    }

    /*
     * With no attention items, the summary row becomes the
     * healthy content for the screen.
     */
    if (status->attention_item_count <= 0)
    {
        layer_set_hidden(
            summary_layer,
            false);

        layer_set_hidden(
            detail_layer,
            true);

        status_row_set_text(
            s_summary_status_row,
            "No issues");

        status_row_set_state(
            s_summary_status_row,
            STATUS_INDICATOR_HEALTHY);

        return;
    }

    /*
     * Live attention items communicate their own severity.
     * Do not duplicate that information with another summary
     * row above them.
     */
    layer_set_hidden(
        summary_layer,
        true);

    layer_set_hidden(
        detail_layer,
        true);
}

static void prv_refresh_pages(void)
{
    const ServerStatus *const status =
        server_status_service_get();

    if (status == NULL)
    {
        return;
    }

    if (!status->has_received_snapshot)
    {
        layer_set_hidden(
            s_page_layers[0],
            true);

        layer_set_hidden(
            s_page_layers[1],
            true);

        layer_set_hidden(
            s_page_indicator_layer,
            true);

        s_current_page = 0;
        s_target_page = 0;

        prv_set_viewport_page(0);

        return;
    }

    const bool has_attention =
        status->attention_item_count > 0;

    layer_set_hidden(
        s_page_layers[0],
        !has_attention);

    layer_set_hidden(
        s_page_layers[1],
        !has_attention);

    if (!has_attention)
    {
        s_current_page = 0;
        s_target_page = 0;

        prv_set_viewport_page(0);

        layer_set_hidden(
            s_page_indicator_layer,
            true);

        return;
    }

    const int page_count =
        prv_get_page_count(status);

    /*
     * If the live alert count shrinks while page two is
     * displayed, return safely to page one.
     */
    if (s_current_page >= page_count)
    {
        s_current_page = 0;
        s_target_page = 0;
    }

    prv_build_page(
        0,
        status);

    prv_build_page(
        1,
        status);

    if (!s_is_animating)
    {
        prv_set_viewport_page(
            s_current_page);
    }

    prv_update_page_indicator();
}

static void prv_update_page_layout(
    const ServerStatus *status)
{
    if ((status == NULL) ||
        (s_page_viewport_layer == NULL))
    {
        return;
    }

    /*
     * Cached connection status occupies the top of the screen,
     * so move cached alert pages below it. Live alert pages use
     * the normal higher position.
     */
    const bool show_connection_status =
        status->has_received_snapshot &&
        (status->connection_state ==
         CONNECTION_STATE_FAILED);

    Layer *const viewport_layer =
        s_page_viewport_layer;

    GRect frame =
        layer_get_frame(
            viewport_layer);

    frame.origin.y =
        show_connection_status
            ? 90
            : PAGE_VIEWPORT_Y;

    layer_set_frame(
        viewport_layer,
        frame);
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

    /*
     * Three-stage travelling sleep effect:
     *
     * 0: small z at the ear
     * 1: medium Z higher/right
     * 2: small hollow bubble at the top
     *
     * Then the cycle resets to phase 0.
     */
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

static void prv_update_healthy_cat(
    const ServerStatus *status)
{
    if ((status == NULL) ||
        (s_healthy_cat_layer == NULL))
    {
        return;
    }

    const bool visible =
        status->has_received_snapshot &&
        (status->connection_state !=
         CONNECTION_STATE_FAILED) &&
        (status->attention_item_count <= 0);

    layer_set_hidden(
        bitmap_layer_get_layer(
            s_healthy_cat_layer),
        !visible);

    if (visible)
    {
        /*
         * The animation phase exclusively owns visibility of
         * the z/Z/bubble layers. Poll refreshes must not unhide
         * them independently or multiple phases can flash at once.
         */
        if (s_healthy_z_timer == NULL)
        {
            prv_start_healthy_z_animation();
        }
        else
        {
            prv_apply_healthy_sleep_phase();
        }
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

static void prv_refresh(void)
{
    const ServerStatus *const status =
        server_status_service_get();

    if (status == NULL)
    {
        return;
    }

    prv_update_summary(status);
    prv_update_page_layout(status);
    prv_refresh_pages();
    prv_update_healthy_cat(status);
}

static void prv_page_animation_stopped(
    Animation *animation,
    bool finished,
    void *context)
{
    (void)animation;
    (void)context;

    const ServerStatus *const status =
        server_status_service_get();

    if (finished &&
        (status != NULL))
    {
        const int page_count =
            prv_get_page_count(status);

        if (s_target_page < page_count)
        {
            s_current_page =
                s_target_page;
        }
        else
        {
            s_current_page = 0;
        }
    }

    s_is_animating = false;

    prv_set_viewport_page(
        s_current_page);

    prv_update_page_indicator();
}

static void prv_animate_to_page(
    int target_page)
{
    const ServerStatus *const status =
        server_status_service_get();

    if ((status == NULL) ||
        (!status->has_received_snapshot) ||
        (status->attention_item_count <= 0) ||
        s_is_animating)
    {
        return;
    }

    const int page_count =
        prv_get_page_count(status);

    if ((target_page < 0) ||
        (target_page >= page_count) ||
        (target_page == s_current_page))
    {
        return;
    }

    const GRect viewport_bounds =
        layer_get_bounds(
            s_page_viewport_layer);

    const int16_t width =
        viewport_bounds.size.w;

    GPoint from_origin =
        GPoint(
            -(s_current_page * width),
            0);

    GPoint to_origin =
        GPoint(
            -(target_page * width),
            0);

    PropertyAnimation *const property_animation =
        property_animation_create_bounds_origin(
            s_page_viewport_layer,
            &from_origin,
            &to_origin);

    if (property_animation == NULL)
    {
        s_current_page =
            target_page;

        s_target_page =
            target_page;

        s_is_animating = false;

        prv_set_viewport_page(
            s_current_page);

        prv_update_page_indicator();

        return;
    }

    Animation *const animation =
        property_animation_get_animation(
            property_animation);

    animation_set_duration(
        animation,
        PAGE_ANIMATION_DURATION_MS);

    animation_set_curve(
        animation,
        AnimationCurveEaseInOut);

    animation_set_handlers(
        animation,
        (AnimationHandlers){
            .stopped =
                prv_page_animation_stopped,
        },
        NULL);

    s_target_page =
        target_page;

    s_is_animating = true;

    animation_schedule(
        animation);
}

static void prv_up_click_handler(
    ClickRecognizerRef recognizer,
    void *context)
{
    (void)recognizer;
    (void)context;

    prv_animate_to_page(
        s_current_page - 1);
}

static void prv_down_click_handler(
    ClickRecognizerRef recognizer,
    void *context)
{
    (void)recognizer;
    (void)context;

    prv_animate_to_page(
        s_current_page + 1);
}

static void prv_back_click_handler(
    ClickRecognizerRef recognizer,
    void *context)
{
    (void)recognizer;
    (void)context;

    if (s_is_animating)
    {
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

    s_current_page = 0;
    s_target_page = 0;
    s_is_animating = false;
    s_healthy_sleep_phase = 0;
    s_healthy_z_timer = NULL;

    s_title_layer =
        text_layer_create(
            GRect(
                0,
                TITLE_Y,
                bounds.size.w,
                TITLE_HEIGHT));

    text_layer_set_text(
        s_title_layer,
        "Attention");

    text_layer_set_font(
        s_title_layer,
        fonts_get_system_font(
            FONT_KEY_GOTHIC_24_BOLD));

    text_layer_set_text_alignment(
        s_title_layer,
        GTextAlignmentCenter);

    layer_add_child(
        window_layer,
        text_layer_get_layer(
            s_title_layer));

    s_title_divider_layer =
        layer_create(
            GRect(
                TITLE_DIVIDER_X,
                TITLE_DIVIDER_Y,
                bounds.size.w -
                    (2 * TITLE_DIVIDER_X),
                TITLE_DIVIDER_HEIGHT));

    if (s_title_divider_layer != NULL)
    {
        layer_set_update_proc(
            s_title_divider_layer,
            prv_title_divider_update_proc);

        layer_add_child(
            window_layer,
            s_title_divider_layer);
    }

    /*
     * Top-level status used only for healthy/no-attention
     * state and connection failure/loss.
     */
    s_summary_status_row =
        status_row_create(
            GRect(
                12,
                STATUS_Y,
                bounds.size.w - 24,
                STATUS_HEIGHT),
            "Connection failed",
            STATUS_INDICATOR_CRITICAL);

    if (s_summary_status_row != NULL)
    {
        status_row_set_emphasized(
            s_summary_status_row,
            true);

        layer_add_child(
            window_layer,
            status_row_get_layer(
                s_summary_status_row));
    }

    s_status_detail_layer =
        text_layer_create(
            GRect(
                32,
                STATUS_DETAIL_Y,
                bounds.size.w - 44,
                STATUS_DETAIL_HEIGHT));

    text_layer_set_font(
        s_status_detail_layer,
        fonts_get_system_font(
            FONT_KEY_GOTHIC_14));

    text_layer_set_text_alignment(
        s_status_detail_layer,
        GTextAlignmentLeft);

    layer_add_child(
        window_layer,
        text_layer_get_layer(
            s_status_detail_layer));

    s_healthy_cat_bitmap =
        gbitmap_create_with_resource(
            RESOURCE_ID_IMAGE_HEALTHY_CAT);

    if (s_healthy_cat_bitmap != NULL)
    {
        s_healthy_cat_layer =
            bitmap_layer_create(
                GRect(
                    HEALTHY_CAT_X,
                    HEALTHY_CAT_Y,
                    HEALTHY_CAT_WIDTH,
                    HEALTHY_CAT_HEIGHT));

        if (s_healthy_cat_layer != NULL)
        {
            bitmap_layer_set_bitmap(
                s_healthy_cat_layer,
                s_healthy_cat_bitmap);

            /*
             * Preserve the PNG transparency so the cat/basket
             * sits directly on the Attention background.
             */
            bitmap_layer_set_compositing_mode(
                s_healthy_cat_layer,
                GCompOpSet);

            bitmap_layer_set_alignment(
                s_healthy_cat_layer,
                GAlignCenter);

            layer_add_child(
                window_layer,
                bitmap_layer_get_layer(
                    s_healthy_cat_layer));
        }
    }

    s_healthy_z_small_layer =
        text_layer_create(
            GRect(
                HEALTHY_Z_SMALL_X,
                HEALTHY_Z_SMALL_Y,
                HEALTHY_Z_SMALL_WIDTH,
                HEALTHY_Z_SMALL_HEIGHT));

    if (s_healthy_z_small_layer != NULL)
    {
        text_layer_set_text(
            s_healthy_z_small_layer,
            "z");

        text_layer_set_font(
            s_healthy_z_small_layer,
            fonts_get_system_font(
                FONT_KEY_GOTHIC_18_BOLD));

        text_layer_set_background_color(
            s_healthy_z_small_layer,
            GColorClear);

        text_layer_set_text_alignment(
            s_healthy_z_small_layer,
            GTextAlignmentCenter);

        layer_add_child(
            window_layer,
            text_layer_get_layer(
                s_healthy_z_small_layer));
    }

    s_healthy_z_medium_layer =
        text_layer_create(
            GRect(
                HEALTHY_Z_MEDIUM_X,
                HEALTHY_Z_MEDIUM_Y,
                HEALTHY_Z_MEDIUM_WIDTH,
                HEALTHY_Z_MEDIUM_HEIGHT));

    if (s_healthy_z_medium_layer != NULL)
    {
        text_layer_set_text(
            s_healthy_z_medium_layer,
            "Z");

        text_layer_set_font(
            s_healthy_z_medium_layer,
            fonts_get_system_font(
                FONT_KEY_GOTHIC_24_BOLD));

        text_layer_set_background_color(
            s_healthy_z_medium_layer,
            GColorClear);

        text_layer_set_text_alignment(
            s_healthy_z_medium_layer,
            GTextAlignmentCenter);

        layer_add_child(
            window_layer,
            text_layer_get_layer(
                s_healthy_z_medium_layer));
    }

    s_healthy_z_large_layer =
        text_layer_create(
            GRect(
                HEALTHY_Z_LARGE_X,
                HEALTHY_Z_LARGE_Y,
                HEALTHY_Z_LARGE_WIDTH,
                HEALTHY_Z_LARGE_HEIGHT));

    if (s_healthy_z_large_layer != NULL)
    {
        text_layer_set_text(
            s_healthy_z_large_layer,
            "Z");

        text_layer_set_font(
            s_healthy_z_large_layer,
            fonts_get_system_font(
                FONT_KEY_GOTHIC_28_BOLD));

        text_layer_set_background_color(
            s_healthy_z_large_layer,
            GColorClear);

        text_layer_set_text_alignment(
            s_healthy_z_large_layer,
            GTextAlignmentCenter);

        layer_add_child(
            window_layer,
            text_layer_get_layer(
                s_healthy_z_large_layer));
    }

    s_healthy_bubble_layer =
        layer_create(
            GRect(
                HEALTHY_BUBBLE_X,
                HEALTHY_BUBBLE_Y,
                HEALTHY_BUBBLE_SIZE,
                HEALTHY_BUBBLE_SIZE));

    if (s_healthy_bubble_layer != NULL)
    {
        layer_set_update_proc(
            s_healthy_bubble_layer,
            prv_healthy_bubble_update_proc);

        layer_add_child(
            window_layer,
            s_healthy_bubble_layer);
    }

    /*
     * The viewport is fixed on screen during normal live
     * operation. Its bounds origin moves horizontally across
     * the two side-by-side pages.
     */
    s_page_viewport_layer =
        layer_create(
            GRect(
                0,
                PAGE_VIEWPORT_Y,
                bounds.size.w,
                PAGE_VIEWPORT_HEIGHT));

    layer_set_clips(
        s_page_viewport_layer,
        true);

    layer_add_child(
        window_layer,
        s_page_viewport_layer);

    for (int page = 0;
         page < ATTENTION_MAX_PAGES;
         ++page)
    {
        s_page_layers[page] =
            layer_create(
                GRect(
                    page * bounds.size.w,
                    0,
                    bounds.size.w,
                    PAGE_VIEWPORT_HEIGHT));

        if (s_page_layers[page] != NULL)
        {
            layer_add_child(
                s_page_viewport_layer,
                s_page_layers[page]);
        }
    }

    s_page_indicator_layer =
        layer_create(
            GRect(
                0,
                bounds.size.h -
                    PAGE_INDICATOR_HEIGHT,
                bounds.size.w,
                PAGE_INDICATOR_HEIGHT));

    if (s_page_indicator_layer != NULL)
    {
        layer_set_update_proc(
            s_page_indicator_layer,
            prv_page_indicator_update_proc);

        layer_add_child(
            window_layer,
            s_page_indicator_layer);
    }

    prv_set_viewport_page(0);
    prv_refresh();
}

static void prv_window_unload(
    Window *window)
{
    (void)window;

    s_is_animating = false;

    prv_stop_healthy_z_animation();

    prv_destroy_all_rows();

    status_row_destroy(
        s_summary_status_row);

    s_summary_status_row = NULL;

    if (s_status_detail_layer != NULL)
    {
        text_layer_destroy(
            s_status_detail_layer);

        s_status_detail_layer = NULL;
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

    if (s_page_indicator_layer != NULL)
    {
        layer_destroy(
            s_page_indicator_layer);

        s_page_indicator_layer = NULL;
    }

    for (int page = 0;
         page < ATTENTION_MAX_PAGES;
         ++page)
    {
        if (s_page_layers[page] != NULL)
        {
            layer_destroy(
                s_page_layers[page]);

            s_page_layers[page] = NULL;
        }
    }

    if (s_page_viewport_layer != NULL)
    {
        layer_destroy(
            s_page_viewport_layer);

        s_page_viewport_layer = NULL;
    }

    if (s_title_divider_layer != NULL)
    {
        layer_destroy(
            s_title_divider_layer);

        s_title_divider_layer = NULL;
    }

    if (s_title_layer != NULL)
    {
        text_layer_destroy(
            s_title_layer);

        s_title_layer = NULL;
    }
}

Window *attention_window_create(void)
{
    Window *const window =
        window_create();

    if (window != NULL)
    {
        window_set_window_handlers(
            window,
            (WindowHandlers){
                .load = prv_window_load,
                .unload = prv_window_unload,
            });

        window_set_click_config_provider(
            window,
            prv_click_config_provider);
    }

    return window;
}

void attention_window_destroy(
    Window *window)
{
    if (window != NULL)
    {
        window_destroy(window);
    }
}

void attention_window_refresh(void)
{
    if (s_page_viewport_layer == NULL)
    {
        return;
    }

    prv_refresh();
}