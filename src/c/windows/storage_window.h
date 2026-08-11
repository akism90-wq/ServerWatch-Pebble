#pragma once

#include <pebble.h>

Window *storage_window_create(void);
void storage_window_destroy(Window *window);

void storage_window_refresh(void);