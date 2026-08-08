#pragma once

#include <stdbool.h>
#include <stdint.h>

#define SERVER_SERVICE_COUNT 6
#define SERVER_DOWNLOAD_COUNT 2
#define SERVER_ATTENTION_ITEM_COUNT 3

typedef struct
{
    const char *name;
    int progress_percent;
    const char *speed_text;
    const char *eta_text;

    uint32_t size_mb;
    bool suspicious;
} DownloadStatus;

typedef enum
{
    ATTENTION_SEVERITY_INFO = 0,
    ATTENTION_SEVERITY_WARNING,
    ATTENTION_SEVERITY_CRITICAL
} AttentionSeverity;

typedef struct
{
    const char *text;
    AttentionSeverity severity;
} AttentionItem;

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

    float storage_used_tb;
    float storage_free_tb;
    float storage_total_tb;

    float movies_gb;
    float tv_gb;
    float immich_gb;
    float downloads_gb;
    float other_gb;

    int active_downloads;

    int attention_item_count;
    AttentionItem attention_items[SERVER_ATTENTION_ITEM_COUNT];
    
    DownloadStatus downloads[SERVER_DOWNLOAD_COUNT];
    
    ServiceStatus services[SERVER_SERVICE_COUNT];
} ServerStatus;
