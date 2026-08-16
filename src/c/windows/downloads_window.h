#pragma once

#include <pebble.h>

Window *downloads_window_create(void);
void downloads_window_destroy(Window *window);
void downloads_window_refresh(void);