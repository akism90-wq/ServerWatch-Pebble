#include "status_indicator.h"

typedef struct
{
    StatusIndicatorState state;
} StatusIndicatorData;

static GColor prv_color_for_state(StatusIndicatorState state)
{
    switch (state) {
        case STATUS_INDICATOR_HEALTHY:
            return GColorGreen;

        case STATUS_INDICATOR_WARNING:
            return GColorYellow;

        case STATUS_INDICATOR_CRITICAL:
        default:
            return GColorRed;
    }
}

static void prv_update_proc(Layer *layer, GContext *context)
{
    const GRect bounds = layer_get_bounds(layer);
    const GPoint centre = GPoint(
        bounds.size.w / 2,
        bounds.size.h / 2
    );
    const int16_t radius =
        (bounds.size.w < bounds.size.h
            ? bounds.size.w
            : bounds.size.h) / 2;

    const StatusIndicatorData *const data =
        layer_get_data(layer);

    graphics_context_set_fill_color(
        context,
        prv_color_for_state(data->state)
    );

    graphics_fill_circle(
        context,
        centre,
        radius
    );
}

Layer *status_indicator_create(
    GRect frame,
    StatusIndicatorState state
)
{
    Layer *const layer = layer_create_with_data(
        frame,
        sizeof(StatusIndicatorData)
    );

    if (layer != NULL) {
        StatusIndicatorData *const data =
            layer_get_data(layer);

        data->state = state;

        layer_set_update_proc(
            layer,
            prv_update_proc
        );
    }

    return layer;
}

void status_indicator_set_state(
    Layer *layer,
    StatusIndicatorState state
)
{
    if (layer == NULL) {
        return;
    }

    StatusIndicatorData *const data =
        layer_get_data(layer);

    data->state = state;
    layer_mark_dirty(layer);
}

void status_indicator_destroy(Layer *layer)
{
    if (layer != NULL) {
        layer_destroy(layer);
    }
}
