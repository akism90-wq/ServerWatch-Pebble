#pragma once

#include <pebble.h>

typedef struct UpdatedAgeView UpdatedAgeView;

UpdatedAgeView *updated_age_view_create(GRect frame);

Layer *updated_age_view_get_layer(
    UpdatedAgeView *view);

void updated_age_view_refresh(
    UpdatedAgeView *view);

void updated_age_view_destroy(
    UpdatedAgeView *view);
