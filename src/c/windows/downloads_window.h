#pragma once

#include <pebble.h>

Window *downloads_window_create(void);
void downloads_window_destroy(Window *window);
void downloads_window_refresh(void);
void downloads_window_handle_delete_result(
    bool success,
    const char *error_message);
