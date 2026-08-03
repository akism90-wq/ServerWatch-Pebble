#pragma once

#include <pebble.h>

#include "attention_window.h"
#include "server_window.h"
#include "storage_window.h"
#include "downloads_window.h"

Window *home_window_create(void);
void home_window_destroy(Window *window);
