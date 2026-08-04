#pragma once

#include <pebble.h>

#include "status_indicator.h"

typedef struct StatusRow StatusRow;

StatusRow *status_row_create(
    GRect frame,
    const char *text,
    StatusIndicatorState state
);

Layer *status_row_get_layer(StatusRow *row);

void status_row_set_state(
    StatusRow *row,
    StatusIndicatorState state
);

void status_row_set_emphasized(
    StatusRow *row,
    bool emphasized
);

void status_row_destroy(StatusRow *row);
