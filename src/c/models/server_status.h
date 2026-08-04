#pragma once

#include <stdbool.h>

#define SERVER_SERVICE_COUNT 6

typedef struct
{
    const char *name;
    bool online;
} ServiceStatus;

typedef struct
{
    bool has_attention;
    bool server_online;

    float cpu_percent;
    float ram_percent;
    float temperature_celsius;
    float load_average;

    const char *uptime_text;
    const char *updated_text;

    int storage_used_percent;
    int active_downloads;

    ServiceStatus services[SERVER_SERVICE_COUNT];
} ServerStatus;
