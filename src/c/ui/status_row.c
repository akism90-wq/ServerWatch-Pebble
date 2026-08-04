#include "status_row.h"

#include <stdlib.h>

#include "ui_layout.h"

struct StatusRow
{
    Layer *root_layer;
    Layer *indicator_layer;
    TextLayer *text_layer;
};

StatusRow *status_row_create(
    GRect frame,
    const char *text,
    StatusIndicatorState state
)
{
    StatusRow *const row = calloc(1, sizeof(StatusRow));

    if (row == NULL) {
        return NULL;
    }

    row->root_layer = layer_create(frame);

    if (row->root_layer == NULL) {
        free(row);
        return NULL;
    }

    row->indicator_layer = status_indicator_create(
        GRect(
            0,
            6,
            UI_STATUS_DOT_SIZE,
            UI_STATUS_DOT_SIZE
        ),
        state
    );

    row->text_layer = text_layer_create(
        GRect(
            18,
            0,
            frame.size.w - 18,
            frame.size.h
        )
    );

    if (row->text_layer == NULL) {
        status_row_destroy(row);
        return NULL;
    }

    text_layer_set_text(row->text_layer, text);

    text_layer_set_font(
        row->text_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_18)
    );

    text_layer_set_text_alignment(
        row->text_layer,
        GTextAlignmentLeft
    );

    if (row->indicator_layer != NULL) {
        layer_add_child(
            row->root_layer,
            row->indicator_layer
        );
    }

    layer_add_child(
        row->root_layer,
        text_layer_get_layer(row->text_layer)
    );

    return row;
}

Layer *status_row_get_layer(StatusRow *row)
{
    return row == NULL ? NULL : row->root_layer;
}

void status_row_set_state(
    StatusRow *row,
    StatusIndicatorState state
)
{
    if ((row == NULL) || (row->indicator_layer == NULL)) {
        return;
    }

    status_indicator_set_state(
        row->indicator_layer,
        state
    );
}

void status_row_set_emphasized(
    StatusRow *row,
    bool emphasized
)
{
    if (row == NULL) {
        return;
    }

    text_layer_set_font(
        row->text_layer,
        fonts_get_system_font(
            emphasized
                ? FONT_KEY_GOTHIC_18_BOLD
                : FONT_KEY_GOTHIC_18
        )
    );
}

void status_row_destroy(StatusRow *row)
{
    if (row == NULL) {
        return;
    }

    status_indicator_destroy(row->indicator_layer);

    if (row->text_layer != NULL) {
        text_layer_destroy(row->text_layer);
    }

    if (row->root_layer != NULL) {
        layer_destroy(row->root_layer);
    }

    free(row);
}
