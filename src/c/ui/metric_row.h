#pragma once

#include <pebble.h>

typedef struct MetricRow MetricRow;

MetricRow *metric_row_create(
    GRect frame,
    const char *label,
    const char *value
);

Layer *metric_row_get_layer(
    MetricRow *row
);

void metric_row_set_value(
    MetricRow *row,
    const char *value
);

void metric_row_destroy(
    MetricRow *row
);
