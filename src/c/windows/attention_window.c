#include "attention_window.h"

#include "../services/server_status_service.h"
#include "../ui/connection_status_view.h"
#include "../ui/status_row.h"
#include "../ui/updated_age_view.h"

#define ATTENTION_ITEMS_PER_PAGE 4
#define ATTENTION_MAX_PAGES 2

#define TITLE_Y 8
#define TITLE_HEIGHT 30

#define PAGE_VIEWPORT_Y 46
#define ROW_HEIGHT 24
#define ROW_STRIDE 28
#define PAGE_VIEWPORT_HEIGHT \
    (ATTENTION_ITEMS_PER_PAGE * ROW_STRIDE)

#define FOOTER_HEIGHT 40
#define FOOTER_BOTTOM_MARGIN 22

#define PAGE_INDICATOR_HEIGHT 20
#define PAGE_DOT_RADIUS 3
#define PAGE_DOT_SPACING 8

#define PAGE_ANIMATION_DURATION_MS 250

static Layer *s_page_viewport_layer;
static Layer *s_page_layers[ATTENTION_MAX_PAGES];
static Layer *s_page_indicator_layer;

static TextLayer *s_title_layer;
static TextLayer *s_empty_layer;

static UpdatedAgeView *s_updated_age_view;
static ConnectionStatusView *s_connection_status_view;

static StatusRow *s_attention_rows
    [ATTENTION_MAX_PAGES]
    [ATTENTION_ITEMS_PER_PAGE];

static int s_current_page;
static int s_target_page;

static bool s_is_animating;

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

static void prv_page_indicator_update_proc(
    Layer *layer,
    GContext *context)
{
    const ServerStatus *const status =
        server_status_service_get();

    if ((status == NULL) ||
        (!status->has_received_snapshot) ||
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

    if ((status == NULL) ||
        (!status->has_received_snapshot) ||
        (prv_get_page_count(status) <= 1))
    {
        layer_set_hidden(
            s_page_indicator_layer,
            true);

        return;
    }

    layer_set_hidden(
        s_page_indicator_layer,
        false);

    layer_mark_dirty(
        s_page_indicator_layer);
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

static void prv_refresh_pages(void)
{
    const ServerStatus *const status =
        server_status_service_get();

    if (status == NULL)
    {
        return;
    }

    /*
     * Never expose seeded development data before the
     * first genuine ServerWatch snapshot.
     */
    if (!status->has_received_snapshot)
    {
        layer_set_hidden(
            text_layer_get_layer(
                s_empty_layer),
            true);

        layer_set_hidden(
            s_page_layers[0],
            true);

        layer_set_hidden(
            s_page_layers[1],
            true);

        layer_set_hidden(
            s_page_indicator_layer,
            true);

        return;
    }

    const bool has_attention =
        status->attention_item_count > 0;

    layer_set_hidden(
        text_layer_get_layer(
            s_empty_layer),
        has_attention);

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

static void prv_refresh_footer(void)
{
    const ServerStatus *const status =
        server_status_service_get();

    if (status == NULL)
    {
        return;
    }

    const bool connection_lost =
        status->connection_state ==
        CONNECTION_STATE_FAILED;

    if (s_updated_age_view != NULL)
    {
        Layer *const updated_layer =
            updated_age_view_get_layer(
                s_updated_age_view);

        layer_set_hidden(
            updated_layer,
            connection_lost ||
                !status->has_received_snapshot);

        if (!connection_lost &&
            status->has_received_snapshot)
        {
            updated_age_view_refresh(
                s_updated_age_view);
        }
    }

    if (s_connection_status_view != NULL)
    {
        connection_status_view_refresh(
            s_connection_status_view);
    }
}

static void prv_refresh(void)
{
    prv_refresh_pages();
    prv_refresh_footer();
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

    /*
     * Snap to the exact final bounds after animation.
     * The SDK owns/frees the completed animation.
     */
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

    /*
     * Avoid tearing down the viewport while its bounds are
     * actively being animated. The transition is only 250ms.
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

    s_current_page = 0;
    s_target_page = 0;
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

    /*
     * The viewport is fixed on screen. Its bounds origin
     * moves horizontally across the two side-by-side pages.
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

    s_empty_layer =
        text_layer_create(
            GRect(
                12,
                68,
                bounds.size.w - 24,
                40));

    text_layer_set_text(
        s_empty_layer,
        "No attention required");

    text_layer_set_font(
        s_empty_layer,
        fonts_get_system_font(
            FONT_KEY_GOTHIC_18));

    text_layer_set_text_alignment(
        s_empty_layer,
        GTextAlignmentCenter);

    layer_add_child(
        window_layer,
        text_layer_get_layer(
            s_empty_layer));

    const int16_t footer_y =
        bounds.size.h -
        FOOTER_HEIGHT -
        FOOTER_BOTTOM_MARGIN;

    s_updated_age_view =
        updated_age_view_create(
            GRect(
                12,
                footer_y,
                bounds.size.w - 24,
                FOOTER_HEIGHT));

    if (s_updated_age_view != NULL)
    {
        layer_add_child(
            window_layer,
            updated_age_view_get_layer(
                s_updated_age_view));
    }

    s_connection_status_view =
        connection_status_view_create(
            GRect(
                12,
                footer_y,
                bounds.size.w - 24,
                FOOTER_HEIGHT));

    if (s_connection_status_view != NULL)
    {
        layer_add_child(
            window_layer,
            connection_status_view_get_layer(
                s_connection_status_view));
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

    /*
     * Back navigation is ignored during the short page
     * transition, so there is no live animation referencing
     * these layers when unload begins.
     */
    s_is_animating = false;

    prv_destroy_all_rows();

    connection_status_view_destroy(
        s_connection_status_view);

    s_connection_status_view = NULL;

    updated_age_view_destroy(
        s_updated_age_view);

    s_updated_age_view = NULL;

    if (s_page_indicator_layer != NULL)
    {
        layer_destroy(
            s_page_indicator_layer);

        s_page_indicator_layer = NULL;
    }

    if (s_empty_layer != NULL)
    {
        text_layer_destroy(
            s_empty_layer);

        s_empty_layer = NULL;
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