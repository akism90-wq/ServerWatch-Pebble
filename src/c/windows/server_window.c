#include "server_window.h"

#include <stdio.h>

#include "../services/server_status_service.h"
#include "../ui/metric_row.h"
#include "../ui/status_row.h"

#define SERVER_PAGE_COUNT 3
#define SERVER_METRIC_COUNT 5
#define SERVER_SERVICES_PER_PAGE 4
#define SERVER_SERVICE_PAGE_COUNT 2

#define TITLE_Y 8
#define TITLE_HEIGHT 30

#define PAGE_VIEWPORT_Y 42
#define PAGE_VIEWPORT_HEIGHT 164

#define SUMMARY_Y 0
#define SUMMARY_HEIGHT 24

#define SUMMARY_DETAIL_Y 24
#define SUMMARY_DETAIL_HEIGHT 20

#define METRIC_START_Y 40
#define METRIC_HEIGHT 22
#define METRIC_STRIDE 22

#define SERVICES_TITLE_Y 0
#define SERVICES_TITLE_HEIGHT 28

#define SERVICE_START_Y 32
#define SERVICE_HEIGHT 22
#define SERVICE_STRIDE 22

#define PAGE_INDICATOR_HEIGHT 20
#define PAGE_DOT_RADIUS 3
#define PAGE_DOT_SPACING 16

#define PAGE_ANIMATION_DURATION_MS 250

typedef enum
{
    SERVER_PAGE_METRICS = 0,
    SERVER_PAGE_SERVICES_FIRST,
    SERVER_PAGE_SERVICES_SECOND
} ServerPage;

typedef enum
{
    SERVER_METRIC_CPU = 0,
    SERVER_METRIC_RAM,
    SERVER_METRIC_TEMPERATURE,
    SERVER_METRIC_LOAD,
    SERVER_METRIC_UPTIME
} ServerMetric;

static Layer *s_page_viewport_layer;
static Layer *s_page_layers[SERVER_PAGE_COUNT];
static Layer *s_page_indicator_layer;
static Layer *s_diagnostic_layer;

static TextLayer *s_title_layer;
static TextLayer *s_summary_detail_layer;
static TextLayer *s_services_title_layers
    [SERVER_SERVICE_PAGE_COUNT];

static StatusRow *s_server_status_row;
static StatusRow *s_service_rows[SERVER_SERVICE_COUNT];

static MetricRow *s_metric_rows[SERVER_METRIC_COUNT];

static int s_current_page;
static int s_target_page;

static bool s_is_animating;

static char s_cpu_text[16];
static char s_ram_text[16];
static char s_temperature_text[16];
static char s_load_text[16];
static char s_summary_detail_text[48];

static void prv_diagnostic_update_proc(
    Layer *layer,
    GContext *context)
{
    (void)layer;

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

    const int cpu_tenths =
        (int)(status->cpu_percent * 10.0f);

    const int ram_tenths =
        (int)(status->ram_percent * 10.0f);

    const int temperature_tenths =
        (int)(status->temperature_celsius * 10.0f);

    const int load_hundredths =
        (int)(status->load_average * 100.0f);

    snprintf(
        s_cpu_text,
        sizeof(s_cpu_text),
        "%d.%d%%",
        cpu_tenths / 10,
        cpu_tenths % 10);

    snprintf(
        s_ram_text,
        sizeof(s_ram_text),
        "%d.%d%%",
        ram_tenths / 10,
        ram_tenths % 10);

    snprintf(
        s_temperature_text,
        sizeof(s_temperature_text),
        "%d.%d C",
        temperature_tenths / 10,
        temperature_tenths % 10);

    snprintf(
        s_load_text,
        sizeof(s_load_text),
        "%d.%02d",
        load_hundredths / 100,
        load_hundredths % 100);
}

static int prv_get_offline_service_count(
    const ServerStatus *status)
{
    if (status == NULL)
    {
        return 0;
    }

    int offline_count = 0;

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
            ++offline_count;
        }
    }

    return offline_count;
}

static const char *prv_get_first_offline_service(
    const ServerStatus *status)
{
    if (status == NULL)
    {
        return NULL;
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
            return status->services[index].name;
        }
    }

    return NULL;
}

static StatusIndicatorState prv_state_from_online(
    bool online)
{
    return online
        ? STATUS_INDICATOR_HEALTHY
        : STATUS_INDICATOR_CRITICAL;
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

    const int16_t first_dot_x =
        centre_x -
        (((SERVER_PAGE_COUNT - 1) *
          PAGE_DOT_SPACING) / 2);

    for (int page = 0;
         page < SERVER_PAGE_COUNT;
         ++page)
    {
        const int16_t x =
            first_dot_x +
            (page * PAGE_DOT_SPACING);

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
        (s_server_status_row == NULL) ||
        (s_summary_detail_layer == NULL))
    {
        return;
    }

    /*
     * No genuine snapshot exists yet.
     *
     * Do not expose the seeded development model.
     */
    if (!status->has_received_snapshot)
    {
        status_row_set_text(
            s_server_status_row,
            "Connection failed");

        status_row_set_state(
            s_server_status_row,
            STATUS_INDICATOR_CRITICAL);

        snprintf(
            s_summary_detail_text,
            sizeof(s_summary_detail_text),
            "No server data");

        text_layer_set_text(
            s_summary_detail_layer,
            s_summary_detail_text);

        return;
    }

    /*
     * Connection state takes precedence over cached server
     * and service health. Cached values remain visible below.
     */
    if (status->connection_state ==
        CONNECTION_STATE_FAILED)
    {
        status_row_set_text(
            s_server_status_row,
            "Connection lost");

        status_row_set_state(
            s_server_status_row,
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

    const int offline_count =
        prv_get_offline_service_count(status);

    if (offline_count <= 0)
    {
        status_row_set_text(
            s_server_status_row,
            "Online");

        status_row_set_state(
            s_server_status_row,
            STATUS_INDICATOR_HEALTHY);

        s_summary_detail_text[0] = '\0';

        text_layer_set_text(
            s_summary_detail_layer,
            s_summary_detail_text);

        return;
    }

    status_row_set_text(
        s_server_status_row,
        "Warning");

    status_row_set_state(
        s_server_status_row,
        STATUS_INDICATOR_WARNING);

    if (offline_count == 1)
    {
        const char *const service_name =
            prv_get_first_offline_service(status);

        snprintf(
            s_summary_detail_text,
            sizeof(s_summary_detail_text),
            "%s is down",
            service_name != NULL
                ? service_name
                : "Service");
    }
    else
    {
        snprintf(
            s_summary_detail_text,
            sizeof(s_summary_detail_text),
            "%d services are down",
            offline_count);
    }

    text_layer_set_text(
        s_summary_detail_layer,
        s_summary_detail_text);
}

static void prv_update_metrics(
    const ServerStatus *status)
{
    if ((status == NULL) ||
        (!status->has_received_snapshot))
    {
        return;
    }

    prv_format_metric_values(status);

    metric_row_set_value(
        s_metric_rows[SERVER_METRIC_CPU],
        s_cpu_text);

    metric_row_set_value(
        s_metric_rows[SERVER_METRIC_RAM],
        s_ram_text);

    metric_row_set_value(
        s_metric_rows[SERVER_METRIC_TEMPERATURE],
        s_temperature_text);

    metric_row_set_value(
        s_metric_rows[SERVER_METRIC_LOAD],
        s_load_text);

    metric_row_set_value(
        s_metric_rows[SERVER_METRIC_UPTIME],
        status->uptime_text);
}

static void prv_update_services(
    const ServerStatus *status)
{
    if ((status == NULL) ||
        (!status->has_received_snapshot))
    {
        return;
    }

    for (int index = 0;
         index < SERVER_SERVICE_COUNT;
         ++index)
    {
        if (s_service_rows[index] == NULL)
        {
            continue;
        }

        layer_set_hidden(
            status_row_get_layer(
                s_service_rows[index]),
            !status->services[index].monitored);

        status_row_set_state(
            s_service_rows[index],
            prv_state_from_online(
                status->services[index].online));
    }
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

    /*
     * Page 1 always contains the summary row.
     * Detailed data is hidden until a genuine snapshot exists.
     */
    layer_set_hidden(
        metric_row_get_layer(
            s_metric_rows[SERVER_METRIC_CPU]),
        !has_snapshot);

    layer_set_hidden(
        metric_row_get_layer(
            s_metric_rows[SERVER_METRIC_RAM]),
        !has_snapshot);

    layer_set_hidden(
        metric_row_get_layer(
            s_metric_rows[SERVER_METRIC_TEMPERATURE]),
        !has_snapshot);

    layer_set_hidden(
        metric_row_get_layer(
            s_metric_rows[SERVER_METRIC_LOAD]),
        !has_snapshot);

    layer_set_hidden(
        metric_row_get_layer(
            s_metric_rows[SERVER_METRIC_UPTIME]),
        !has_snapshot);

    layer_set_hidden(
        s_page_layers[SERVER_PAGE_SERVICES_FIRST],
        !has_snapshot);

    layer_set_hidden(
        s_page_layers[SERVER_PAGE_SERVICES_SECOND],
        !has_snapshot);

    if (!has_snapshot)
    {
        s_current_page =
            SERVER_PAGE_METRICS;

        s_target_page =
            SERVER_PAGE_METRICS;

        prv_set_viewport_page(
            SERVER_PAGE_METRICS);
    }

    prv_update_page_indicator();
}

void server_window_refresh(void)
{
    const ServerStatus *const status =
        server_status_service_get();

    if ((status == NULL) ||
    (s_title_layer == NULL))
    {
        return;
    }

    /*
     * Use the product title rather than the environment
     * hostname as the fixed screen identity.
     */
    text_layer_set_text(
    s_title_layer,
    "ServerWatch");

    prv_update_summary(status);
    prv_update_visibility(status);

    if (status->has_received_snapshot)
    {
        prv_update_metrics(status);
        prv_update_services(status);
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
        (target_page >= SERVER_PAGE_COUNT) ||
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

    /*
     * Keep the subject layer alive for the short duration
     * of an active page animation.
     */
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
        SERVER_PAGE_METRICS;

    s_target_page =
        SERVER_PAGE_METRICS;

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
        "ServerWatch");

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

    /*
     * The viewport remains fixed. Its bounds origin moves
     * horizontally across two side-by-side page layers.
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
         page < SERVER_PAGE_COUNT;
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
     * Page 1 — machine summary and system metrics.
     */
    s_server_status_row =
        status_row_create(
            GRect(
                12,
                SUMMARY_Y,
                bounds.size.w - 24,
                SUMMARY_HEIGHT),
            "Connection failed",
            STATUS_INDICATOR_CRITICAL);

    if (s_server_status_row != NULL)
    {
        status_row_set_emphasized(
            s_server_status_row,
            true);

        layer_add_child(
            s_page_layers[SERVER_PAGE_METRICS],
            status_row_get_layer(
                s_server_status_row));
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
        s_page_layers[SERVER_PAGE_METRICS],
        text_layer_get_layer(
            s_summary_detail_layer));

    s_metric_rows[SERVER_METRIC_CPU] =
        metric_row_create(
            GRect(
                12,
                METRIC_START_Y +
                    (SERVER_METRIC_CPU *
                     METRIC_STRIDE),
                bounds.size.w - 24,
                METRIC_HEIGHT),
            "CPU",
            "");

    s_metric_rows[SERVER_METRIC_RAM] =
        metric_row_create(
            GRect(
                12,
                METRIC_START_Y +
                    (SERVER_METRIC_RAM *
                     METRIC_STRIDE),
                bounds.size.w - 24,
                METRIC_HEIGHT),
            "RAM",
            "");

    s_metric_rows[SERVER_METRIC_TEMPERATURE] =
        metric_row_create(
            GRect(
                12,
                METRIC_START_Y +
                    (SERVER_METRIC_TEMPERATURE *
                     METRIC_STRIDE),
                bounds.size.w - 24,
                METRIC_HEIGHT),
            "Temp",
            "");

    s_metric_rows[SERVER_METRIC_LOAD] =
        metric_row_create(
            GRect(
                12,
                METRIC_START_Y +
                    (SERVER_METRIC_LOAD *
                     METRIC_STRIDE),
                bounds.size.w - 24,
                METRIC_HEIGHT),
            "Load",
            "");

    s_metric_rows[SERVER_METRIC_UPTIME] =
        metric_row_create(
            GRect(
                12,
                METRIC_START_Y +
                    (SERVER_METRIC_UPTIME *
                     METRIC_STRIDE),
                bounds.size.w - 24,
                METRIC_HEIGHT),
            "Uptime",
            "");

    for (int index = 0;
         index < SERVER_METRIC_COUNT;
         ++index)
    {
        if (s_metric_rows[index] != NULL)
        {
            layer_add_child(
                s_page_layers[SERVER_PAGE_METRICS],
                metric_row_get_layer(
                    s_metric_rows[index]));
        }
    }

    /*
     * Pages 2 and 3 — monitored service state.
     */
    for (int service_page = 0;
         service_page < SERVER_SERVICE_PAGE_COUNT;
         ++service_page)
    {
        const int page =
            SERVER_PAGE_SERVICES_FIRST +
            service_page;

        s_services_title_layers[service_page] =
            text_layer_create(
                GRect(
                    0,
                    SERVICES_TITLE_Y,
                    bounds.size.w,
                    SERVICES_TITLE_HEIGHT));

        if (s_services_title_layers[service_page] !=
            NULL)
        {
            text_layer_set_text(
                s_services_title_layers[service_page],
                service_page == 0
                    ? "Services 1/2"
                    : "Services 2/2");

            text_layer_set_font(
                s_services_title_layers[service_page],
                fonts_get_system_font(
                    FONT_KEY_GOTHIC_18_BOLD));

            text_layer_set_text_alignment(
                s_services_title_layers[service_page],
                GTextAlignmentCenter);

            layer_add_child(
                s_page_layers[page],
                text_layer_get_layer(
                    s_services_title_layers
                        [service_page]));
        }
    }

    const ServerStatus *const status =
        server_status_service_get();

    for (int index = 0;
         index < SERVER_SERVICE_COUNT;
         ++index)
    {
        const int service_page =
            index / SERVER_SERVICES_PER_PAGE;

        const int service_row =
            index % SERVER_SERVICES_PER_PAGE;

        const int page =
            SERVER_PAGE_SERVICES_FIRST +
            service_page;

        s_service_rows[index] =
            status_row_create(
                GRect(
                    12,
                    SERVICE_START_Y +
                        (service_row *
                         SERVICE_STRIDE),
                    bounds.size.w - 24,
                    SERVICE_HEIGHT),
                status->services[index].name,
                prv_state_from_online(
                    status->services[index].online));

        if (s_service_rows[index] != NULL)
        {
            layer_add_child(
                s_page_layers[page],
                status_row_get_layer(
                    s_service_rows[index]));
        }
    }

    /*
     * Fixed page indicator — same native drawn-dot
     * language established by Attention.
     */
    /*
     * Diagnostic only:
     * allocate one additional Layer but do not attach or draw it.
     * This isolates layer_create() from layer_add_child()/rendering.
     */
    s_diagnostic_layer =
        layer_create(
            GRect(
                12,
                39,
                bounds.size.w - 24,
                1));

    if (s_diagnostic_layer != NULL)
    {
        /*
         * Diagnostic only:
         * attach the otherwise-empty layer to the window.
         * It has no update proc and draws nothing.
         */
        layer_set_update_proc(
            s_diagnostic_layer,
            prv_diagnostic_update_proc);

        layer_add_child(
            window_layer,
            s_diagnostic_layer);
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

    prv_set_viewport_page(
        SERVER_PAGE_METRICS);

    server_window_refresh();
}

static void prv_window_unload(
    Window *window)
{
    (void)window;

    /*
     * Back navigation is ignored during the short page
     * animation, so no active animation references these
     * layers during normal unload.
     */
    s_is_animating = false;

    for (int index = 0;
         index < SERVER_SERVICE_COUNT;
         ++index)
    {
        status_row_destroy(
            s_service_rows[index]);

        s_service_rows[index] = NULL;
    }

    for (int service_page = 0;
         service_page < SERVER_SERVICE_PAGE_COUNT;
         ++service_page)
    {
        if (s_services_title_layers[service_page] !=
            NULL)
        {
            text_layer_destroy(
                s_services_title_layers
                    [service_page]);

            s_services_title_layers[service_page] =
                NULL;
        }
    }

    for (int index = 0;
         index < SERVER_METRIC_COUNT;
         ++index)
    {
        metric_row_destroy(
            s_metric_rows[index]);

        s_metric_rows[index] = NULL;
    }

    status_row_destroy(
        s_server_status_row);

    s_server_status_row = NULL;

    if (s_summary_detail_layer != NULL)
    {
        text_layer_destroy(
            s_summary_detail_layer);

        s_summary_detail_layer = NULL;
    }

    if (s_diagnostic_layer != NULL)
    {
        layer_destroy(
            s_diagnostic_layer);

        s_diagnostic_layer = NULL;
    }

    if (s_page_indicator_layer != NULL)
    {
        layer_destroy(
            s_page_indicator_layer);

        s_page_indicator_layer = NULL;
    }

    for (int page = 0;
         page < SERVER_PAGE_COUNT;
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

Window *server_window_create(void)
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

void server_window_destroy(
    Window *window)
{
    if (window != NULL)
    {
        window_destroy(window);
    }
}
