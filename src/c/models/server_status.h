#pragma once

#include <stdbool.h>

typedef struct
{
    bool has_attention;
    bool server_online;
    int storage_used_percent;
    int active_downloads;
} ServerStatus;
