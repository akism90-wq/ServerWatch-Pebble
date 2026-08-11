#include "metric_row.h"

#include <stdlib.h>

#include "ui_layout.h"

struct MetricRow
{
    Layer *root_layer;
    TextLayer *label_layer;
    TextLayer *value_layer;
};

MetricRow *metric_row_create(
    GRect frame,
    const char *label,
    const char *value
)
{
    MetricRow *const row = calloc(1, sizeof(MetricRow));

    if (row == NULL) {
        return NULL;
    }

    row->root_layer = layer_create(frame);

    if (row->root_layer == NULL) {
        free(row);
        return NULL;
    }

    row->label_layer = text_layer_create(
        GRect(
            0,
            0,
            UI_METRIC_LABEL_WIDTH,
            frame.size.h
        )
    );

    row->value_layer = text_layer_create(
        GRect(
            UI_METRIC_VALUE_X,
            0,
            frame.size.w
                - UI_METRIC_VALUE_X
                - UI_METRIC_VALUE_RIGHT_INSET,
            frame.size.h
        )
    );

    if ((row->label_layer == NULL) ||
        (row->value_layer == NULL)) {
        metric_row_destroy(row);
        return NULL;
    }

    text_layer_set_text(row->label_layer, label);
    text_layer_set_text(row->value_layer, value);

    text_layer_set_font(
        row->label_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_18)
    );

    text_layer_set_font(
        row->value_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_18)
    );

    text_layer_set_text_alignment(
        row->label_layer,
        GTextAlignmentLeft
    );

    text_layer_set_text_alignment(
        row->value_layer,
        GTextAlignmentRight
    );

    layer_add_child(
        row->root_layer,
        text_layer_get_layer(row->label_layer)
    );

    layer_add_child(
        row->root_layer,
        text_layer_get_layer(row->value_layer)
    );

    return row;
}

Layer *metric_row_get_layer(MetricRow *row)
{
    return row == NULL ? NULL : row->root_layer;
}

void metric_row_set_value(
    MetricRow *row,
    const char *value
)
{
    if (row == NULL) {
        return;
    }

    text_layer_set_text(row->value_layer, value);
}

void metric_row_destroy(MetricRow *row)
{
    if (row == NULL) {
        return;
    }

    if (row->value_layer != NULL) {
        text_layer_destroy(row->value_layer);
    }

    if (row->label_layer != NULL) {
        text_layer_destroy(row->label_layer);
    }

    if (row->root_layer != NULL) {
        layer_destroy(row->root_layer);
    }

    free(row);
}
