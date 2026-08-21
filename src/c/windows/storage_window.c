#include "storage_window.h"

#include <stdio.h>

#include "../services/server_status_service.h"
#include "../ui/metric_row.h"
#include "../ui/status_row.h"

#define STORAGE_PAGE_COUNT 2

#define CAPACITY_METRIC_COUNT 4
#define BREAKDOWN_METRIC_COUNT 5

#define TITLE_Y 8
#define TITLE_HEIGHT 30

#define PAGE_VIEWPORT_Y 42
#define PAGE_VIEWPORT_HEIGHT 164

#define SUMMARY_Y 0
#define SUMMARY_HEIGHT 24

#define SUMMARY_DETAIL_Y 24
#define SUMMARY_DETAIL_HEIGHT 20

#define CAPACITY_START_Y 48
#define CAPACITY_HEIGHT 22
#define CAPACITY_STRIDE 22

#define BREAKDOWN_TITLE_Y 0
#define BREAKDOWN_TITLE_HEIGHT 28

#define BREAKDOWN_START_Y 32
#define BREAKDOWN_HEIGHT 22
#define BREAKDOWN_STRIDE 22

#define PAGE_INDICATOR_HEIGHT 20
#define PAGE_DOT_RADIUS 3
#define PAGE_DOT_SPACING 8

#define PAGE_ANIMATION_DURATION_MS 250

typedef enum
{
    STORAGE_PAGE_CAPACITY = 0,
    STORAGE_PAGE_BREAKDOWN
} StoragePage;

typedef enum
{
    CAPACITY_USAGE = 0,
    CAPACITY_USED,
    CAPACITY_FREE,
    CAPACITY_TOTAL
} CapacityMetric;

typedef enum
{
    BREAKDOWN_MOVIES = 0,
    BREAKDOWN_TV,
    BREAKDOWN_IMMICH,
    BREAKDOWN_DOWNLOADS,
    BREAKDOWN_OTHER
} BreakdownMetric;

static Layer *s_page_viewport_layer;
static Layer *s_page_layers[STORAGE_PAGE_COUNT];
static Layer *s_page_indicator_layer;
static Layer *s_title_divider_layer;

static TextLayer *s_title_layer;
static TextLayer *s_summary_detail_layer;
static TextLayer *s_breakdown_title_layer;

static StatusRow *s_storage_status_row;

static MetricRow *s_capacity_rows[CAPACITY_METRIC_COUNT];
static MetricRow *s_breakdown_rows[BREAKDOWN_METRIC_COUNT];

static int s_current_page;
static int s_target_page;

static bool s_is_animating;

static char s_usage_text[16];
static char s_used_text[24];
static char s_free_text[24];
static char s_total_text[24];

static char s_movies_text[16];
static char s_tv_text[16];
static char s_immich_text[16];
static char s_downloads_text[16];
static char s_other_text[16];

static char s_summary_detail_text[48];

static void prv_title_divider_update_proc(
    Layer *layer,
    GContext *context)
{
    graphics_context_set_fill_color(
        context,
        GColorBlack);

    graphics_fill_rect(
        context,
        layer_get_bounds(layer),
        0,
        GCornerNone);
}

static void prv_format_metric_values(
    const ServerStatus *status)
{
    if (status == NULL)
    {
        return;
    }

    const int used_tenths =
        (int)(status->storage_used_tb * 10.0f);

    const int free_tenths =
        (int)(status->storage_free_tb * 10.0f);

    const int total_tenths =
        (int)(status->storage_total_tb * 10.0f);

    snprintf(
        s_usage_text,
        sizeof(s_usage_text),
        "%d%%",
        status->storage_used_percent);

    snprintf(
        s_used_text,
        sizeof(s_used_text),
        "%d.%d TB",
        used_tenths / 10,
        used_tenths % 10);

    snprintf(
        s_free_text,
        sizeof(s_free_text),
        "%d.%d TB",
        free_tenths / 10,
        free_tenths % 10);

    snprintf(
        s_total_text,
        sizeof(s_total_text),
        "%d.%d TB",
        total_tenths / 10,
        total_tenths % 10);

    snprintf(
        s_movies_text,
        sizeof(s_movies_text),
        "%d GB",
        (int)status->movies_gb);

    snprintf(
        s_tv_text,
        sizeof(s_tv_text),
        "%d GB",
        (int)status->tv_gb);

    snprintf(
        s_immich_text,
        sizeof(s_immich_text),
        "%d GB",
        (int)status->immich_gb);

    snprintf(
        s_downloads_text,
        sizeof(s_downloads_text),
        "%d GB",
        (int)status->downloads_gb);

    snprintf(
        s_other_text,
        sizeof(s_other_text),
        "%d GB",
        (int)status->other_gb);
}

static void prv_page_indicator_update_proc(
    Layer *layer,
    GContext *context)
{
    const ServerStatus *const status =
        server_status_service_get();

    if ((status == NULL) ||
        (!status->has_received_snapshot))
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
         page < STORAGE_PAGE_COUNT;
         ++page)
    {
        const int16_t x =
            centre_x +
            ((page == STORAGE_PAGE_CAPACITY)
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
        status->has_received_snapshot;

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

static void prv_update_summary(
    const ServerStatus *status)
{
    if ((status == NULL) ||
        (s_storage_status_row == NULL) ||
        (s_summary_detail_layer == NULL))
    {
        return;
    }

    /*
     * Never expose seeded development storage values before
     * the first genuine ServerWatch snapshot.
     */
    if (!status->has_received_snapshot)
    {
        status_row_set_text(
            s_storage_status_row,
            "Connection failed");

        status_row_set_state(
            s_storage_status_row,
            STATUS_INDICATOR_CRITICAL);

        snprintf(
            s_summary_detail_text,
            sizeof(s_summary_detail_text),
            "No storage data");

        text_layer_set_text(
            s_summary_detail_layer,
            s_summary_detail_text);

        return;
    }

    /*
     * Connection failure takes precedence over storage health.
     * Last-known capacity and breakdown values remain visible.
     */
    if (status->connection_state ==
        CONNECTION_STATE_FAILED)
    {
        status_row_set_text(
            s_storage_status_row,
            "Connection lost");

        status_row_set_state(
            s_storage_status_row,
            STATUS_INDICATOR_CRITICAL);

        char age_text[24];

        server_status_service_format_update_age_value(
            age_text,
            sizeof(age_text));

        snprintf(
            s_summary_detail_text,
            sizeof(s_summary_detail_text),
            "Cached - %s",
            age_text);

        text_layer_set_text(
            s_summary_detail_layer,
            s_summary_detail_text);

        return;
    }

    if (status->storage_warning)
    {
        status_row_set_text(
            s_storage_status_row,
            "Warning");

        status_row_set_state(
            s_storage_status_row,
            STATUS_INDICATOR_WARNING);

        snprintf(
            s_summary_detail_text,
            sizeof(s_summary_detail_text),
            "Storage running low");

        text_layer_set_text(
            s_summary_detail_layer,
            s_summary_detail_text);

        return;
    }

    status_row_set_text(
        s_storage_status_row,
        "Healthy");

    status_row_set_state(
        s_storage_status_row,
        STATUS_INDICATOR_HEALTHY);

    s_summary_detail_text[0] = '\0';

    text_layer_set_text(
        s_summary_detail_layer,
        s_summary_detail_text);
}

static void prv_update_capacity(
    const ServerStatus *status)
{
    if ((status == NULL) ||
        (!status->has_received_snapshot))
    {
        return;
    }

    prv_format_metric_values(status);

    metric_row_set_value(
        s_capacity_rows[CAPACITY_USAGE],
        s_usage_text);

    metric_row_set_value(
        s_capacity_rows[CAPACITY_USED],
        s_used_text);

    metric_row_set_value(
        s_capacity_rows[CAPACITY_FREE],
        s_free_text);

    metric_row_set_value(
        s_capacity_rows[CAPACITY_TOTAL],
        s_total_text);
}

static void prv_update_breakdown(
    const ServerStatus *status)
{
    if ((status == NULL) ||
        (!status->has_received_snapshot))
    {
        return;
    }

    /*
     * Formatting is shared with page 1 so both pages always
     * represent the same ServerStatus snapshot.
     */
    prv_format_metric_values(status);

    metric_row_set_value(
        s_breakdown_rows[BREAKDOWN_MOVIES],
        s_movies_text);

    metric_row_set_value(
        s_breakdown_rows[BREAKDOWN_TV],
        s_tv_text);

    metric_row_set_value(
        s_breakdown_rows[BREAKDOWN_IMMICH],
        s_immich_text);

    metric_row_set_value(
        s_breakdown_rows[BREAKDOWN_DOWNLOADS],
        s_downloads_text);

    metric_row_set_value(
        s_breakdown_rows[BREAKDOWN_OTHER],
        s_other_text);
}

static void prv_update_visibility(
    const ServerStatus *status)
{
    if (status == NULL)
    {
        return;
    }

    const bool has_snapshot =
        status->has_received_snapshot;

    for (int index = 0;
         index < CAPACITY_METRIC_COUNT;
         ++index)
    {
        if (s_capacity_rows[index] != NULL)
        {
            layer_set_hidden(
                metric_row_get_layer(
                    s_capacity_rows[index]),
                !has_snapshot);
        }
    }

    if (s_page_layers[STORAGE_PAGE_BREAKDOWN] != NULL)
    {
        layer_set_hidden(
            s_page_layers[STORAGE_PAGE_BREAKDOWN],
            !has_snapshot);
    }

    if (!has_snapshot)
    {
        s_current_page =
            STORAGE_PAGE_CAPACITY;

        s_target_page =
            STORAGE_PAGE_CAPACITY;

        prv_set_viewport_page(
            STORAGE_PAGE_CAPACITY);
    }

    prv_update_page_indicator();
}

void storage_window_refresh(void)
{
    const ServerStatus *const status =
        server_status_service_get();

    if ((status == NULL) ||
        (s_title_layer == NULL))
    {
        return;
    }

    text_layer_set_text(
        s_title_layer,
        "Storage");

    prv_update_summary(status);
    prv_update_visibility(status);

    if (status->has_received_snapshot)
    {
        prv_update_capacity(status);
        prv_update_breakdown(status);
    }
}

static void prv_page_animation_stopped(
    Animation *animation,
    bool finished,
    void *context)
{
    (void)animation;
    (void)context;

    if (finished)
    {
        s_current_page =
            s_target_page;
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
        s_is_animating)
    {
        return;
    }

    if ((target_page < 0) ||
        (target_page >= STORAGE_PAGE_COUNT) ||
        (target_page == s_current_page))
    {
        return;
    }

    const GRect bounds =
        layer_get_bounds(
            s_page_viewport_layer);

    const int16_t width =
        bounds.size.w;

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
        (AnimationHandlers) {
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

    s_current_page =
        STORAGE_PAGE_CAPACITY;

    s_target_page =
        STORAGE_PAGE_CAPACITY;

    s_is_animating = false;

    s_title_layer =
        text_layer_create(
            GRect(
                0,
                TITLE_Y,
                bounds.size.w,
                TITLE_HEIGHT));

    text_layer_set_text(
        s_title_layer,
        "Storage");

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
                12,
                39,
                bounds.size.w - 24,
                1));

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
     * Same proven viewport architecture as Server:
     * two side-by-side pages with an animated bounds origin.
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
         page < STORAGE_PAGE_COUNT;
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

    /*
     * Page 1 — storage health and overall capacity.
     */
    s_storage_status_row =
        status_row_create(
            GRect(
                12,
                SUMMARY_Y,
                bounds.size.w - 24,
                SUMMARY_HEIGHT),
            "Connection failed",
            STATUS_INDICATOR_CRITICAL);

    if (s_storage_status_row != NULL)
    {
        status_row_set_emphasized(
            s_storage_status_row,
            true);

        layer_add_child(
            s_page_layers[STORAGE_PAGE_CAPACITY],
            status_row_get_layer(
                s_storage_status_row));
    }

    s_summary_detail_layer =
        text_layer_create(
            GRect(
                32,
                SUMMARY_DETAIL_Y,
                bounds.size.w - 44,
                SUMMARY_DETAIL_HEIGHT));

    text_layer_set_font(
        s_summary_detail_layer,
        fonts_get_system_font(
            FONT_KEY_GOTHIC_14));

    text_layer_set_text_alignment(
        s_summary_detail_layer,
        GTextAlignmentLeft);

    layer_add_child(
        s_page_layers[STORAGE_PAGE_CAPACITY],
        text_layer_get_layer(
            s_summary_detail_layer));

    s_capacity_rows[CAPACITY_USAGE] =
        metric_row_create(
            GRect(
                12,
                CAPACITY_START_Y +
                    (CAPACITY_USAGE *
                     CAPACITY_STRIDE),
                bounds.size.w - 24,
                CAPACITY_HEIGHT),
            "Usage",
            "");

    s_capacity_rows[CAPACITY_USED] =
        metric_row_create(
            GRect(
                12,
                CAPACITY_START_Y +
                    (CAPACITY_USED *
                     CAPACITY_STRIDE),
                bounds.size.w - 24,
                CAPACITY_HEIGHT),
            "Used",
            "");

    s_capacity_rows[CAPACITY_FREE] =
        metric_row_create(
            GRect(
                12,
                CAPACITY_START_Y +
                    (CAPACITY_FREE *
                     CAPACITY_STRIDE),
                bounds.size.w - 24,
                CAPACITY_HEIGHT),
            "Free",
            "");

    s_capacity_rows[CAPACITY_TOTAL] =
        metric_row_create(
            GRect(
                12,
                CAPACITY_START_Y +
                    (CAPACITY_TOTAL *
                     CAPACITY_STRIDE),
                bounds.size.w - 24,
                CAPACITY_HEIGHT),
            "Total",
            "");

    for (int index = 0;
         index < CAPACITY_METRIC_COUNT;
         ++index)
    {
        if (s_capacity_rows[index] != NULL)
        {
            layer_add_child(
                s_page_layers[STORAGE_PAGE_CAPACITY],
                metric_row_get_layer(
                    s_capacity_rows[index]));
        }
    }

    /*
     * Page 2 — storage category breakdown.
     */
    s_breakdown_title_layer =
        text_layer_create(
            GRect(
                0,
                BREAKDOWN_TITLE_Y,
                bounds.size.w,
                BREAKDOWN_TITLE_HEIGHT));

    text_layer_set_text(
        s_breakdown_title_layer,
        "Breakdown");

    text_layer_set_font(
        s_breakdown_title_layer,
        fonts_get_system_font(
            FONT_KEY_GOTHIC_18_BOLD));

    text_layer_set_text_alignment(
        s_breakdown_title_layer,
        GTextAlignmentCenter);

    layer_add_child(
        s_page_layers[STORAGE_PAGE_BREAKDOWN],
        text_layer_get_layer(
            s_breakdown_title_layer));

    s_breakdown_rows[BREAKDOWN_MOVIES] =
        metric_row_create(
            GRect(
                12,
                BREAKDOWN_START_Y +
                    (BREAKDOWN_MOVIES *
                     BREAKDOWN_STRIDE),
                bounds.size.w - 24,
                BREAKDOWN_HEIGHT),
            "Movies",
            "");

    s_breakdown_rows[BREAKDOWN_TV] =
        metric_row_create(
            GRect(
                12,
                BREAKDOWN_START_Y +
                    (BREAKDOWN_TV *
                     BREAKDOWN_STRIDE),
                bounds.size.w - 24,
                BREAKDOWN_HEIGHT),
            "TV",
            "");

    s_breakdown_rows[BREAKDOWN_IMMICH] =
        metric_row_create(
            GRect(
                12,
                BREAKDOWN_START_Y +
                    (BREAKDOWN_IMMICH *
                     BREAKDOWN_STRIDE),
                bounds.size.w - 24,
                BREAKDOWN_HEIGHT),
            "Immich",
            "");

    s_breakdown_rows[BREAKDOWN_DOWNLOADS] =
        metric_row_create(
            GRect(
                12,
                BREAKDOWN_START_Y +
                    (BREAKDOWN_DOWNLOADS *
                     BREAKDOWN_STRIDE),
                bounds.size.w - 24,
                BREAKDOWN_HEIGHT),
            "Downloads",
            "");

    s_breakdown_rows[BREAKDOWN_OTHER] =
        metric_row_create(
            GRect(
                12,
                BREAKDOWN_START_Y +
                    (BREAKDOWN_OTHER *
                     BREAKDOWN_STRIDE),
                bounds.size.w - 24,
                BREAKDOWN_HEIGHT),
            "Other",
            "");

    for (int index = 0;
         index < BREAKDOWN_METRIC_COUNT;
         ++index)
    {
        if (s_breakdown_rows[index] != NULL)
        {
            layer_add_child(
                s_page_layers[STORAGE_PAGE_BREAKDOWN],
                metric_row_get_layer(
                    s_breakdown_rows[index]));
        }
    }

    /*
     * Fixed page indicator.
     */
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

    prv_set_viewport_page(
        STORAGE_PAGE_CAPACITY);

    storage_window_refresh();
}

static void prv_window_unload(
    Window *window)
{
    (void)window;

    s_is_animating = false;

    for (int index = 0;
         index < BREAKDOWN_METRIC_COUNT;
         ++index)
    {
        metric_row_destroy(
            s_breakdown_rows[index]);

        s_breakdown_rows[index] = NULL;
    }

    if (s_breakdown_title_layer != NULL)
    {
        text_layer_destroy(
            s_breakdown_title_layer);

        s_breakdown_title_layer = NULL;
    }

    for (int index = 0;
         index < CAPACITY_METRIC_COUNT;
         ++index)
    {
        metric_row_destroy(
            s_capacity_rows[index]);

        s_capacity_rows[index] = NULL;
    }

    status_row_destroy(
        s_storage_status_row);

    s_storage_status_row = NULL;

    if (s_summary_detail_layer != NULL)
    {
        text_layer_destroy(
            s_summary_detail_layer);

        s_summary_detail_layer = NULL;
    }

    if (s_title_divider_layer != NULL)
    {
        layer_destroy(
            s_title_divider_layer);

        s_title_divider_layer = NULL;
    }

    if (s_page_indicator_layer != NULL)
    {
        layer_destroy(
            s_page_indicator_layer);

        s_page_indicator_layer = NULL;
    }

    for (int page = 0;
         page < STORAGE_PAGE_COUNT;
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

    if (s_title_layer != NULL)
    {
        text_layer_destroy(
            s_title_layer);

        s_title_layer = NULL;
    }
}

Window *storage_window_create(void)
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

        window_set_click_config_provider(
            window,
            prv_click_config_provider);
    }

    return window;
}

void storage_window_destroy(
    Window *window)
{
    if (window != NULL)
    {
        window_destroy(window);
    }
}