#pragma once

#include <pebble.h>

typedef struct ConnectionStatusView ConnectionStatusView;

ConnectionStatusView *connection_status_view_create(
    GRect frame
);

Layer *connection_status_view_get_layer(
    ConnectionStatusView *view
);

void connection_status_view_refresh(
    ConnectionStatusView *view
);

void connection_status_view_destroy(
    ConnectionStatusView *view
);