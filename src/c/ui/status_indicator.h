#pragma once

#include <pebble.h>

typedef enum
{
    STATUS_INDICATOR_HEALTHY,
    STATUS_INDICATOR_WARNING,
    STATUS_INDICATOR_CRITICAL
} StatusIndicatorState;

Layer *status_indicator_create(
    GRect frame,
    StatusIndicatorState state
);

void status_indicator_set_state(
    Layer *layer,
    StatusIndicatorState state
);

void status_indicator_destroy(Layer *layer);
