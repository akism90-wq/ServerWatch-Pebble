#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define SERVER_SERVICE_COUNT 8
#define SERVER_DOWNLOAD_COUNT 3
#define SERVER_ATTENTION_ITEM_COUNT 8
#define SERVER_NAME_LENGTH 32

/*
 * Fixed-size storage is intentional.
 *
 * ServerStatus will shortly be populated from AppMessage callbacks.
 * Owning the character data here prevents the model from retaining
 * pointers into temporary Pebble/AppMessage buffers.
 */
#define DOWNLOAD_NAME_LENGTH 64
#define DOWNLOAD_SPEED_TEXT_LENGTH 16
#define DOWNLOAD_ETA_TEXT_LENGTH 16
#define DOWNLOAD_SUBTITLE_LENGTH 32
#define DOWNLOAD_QUALITY_LENGTH 8
#define DOWNLOAD_STATE_LENGTH 16
#define ATTENTION_TEXT_LENGTH 64
#define SERVICE_NAME_LENGTH 16
#define UPTIME_TEXT_LENGTH 24
#define UPDATED_TEXT_LENGTH 24

typedef struct
{
    char name[DOWNLOAD_NAME_LENGTH];
    char subtitle[DOWNLOAD_SUBTITLE_LENGTH];
    char quality[DOWNLOAD_QUALITY_LENGTH];
    char state[DOWNLOAD_STATE_LENGTH];

    int progress_percent;

    char speed_text[DOWNLOAD_SPEED_TEXT_LENGTH];
    char eta_text[DOWNLOAD_ETA_TEXT_LENGTH];

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
    char text[ATTENTION_TEXT_LENGTH];
    AttentionSeverity severity;
} AttentionItem;

typedef struct
{
    char name[SERVICE_NAME_LENGTH];
    bool online;
    bool monitored;
} ServiceStatus;

typedef enum
{
    CONNECTION_STATE_UNKNOWN = 0,
    CONNECTION_STATE_CONNECTED,
    CONNECTION_STATE_FAILED
} ConnectionState;

typedef struct
{
    char server_name[SERVER_NAME_LENGTH];

    bool has_attention;
    bool server_online;

    ConnectionState connection_state;
    bool has_received_snapshot;

    float cpu_percent;
    float ram_percent;
    float temperature_celsius;
    float load_average;

    char uptime_text[UPTIME_TEXT_LENGTH];
    char updated_text[UPDATED_TEXT_LENGTH];
    time_t last_update_time;

    int storage_used_percent;
    bool storage_warning;

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
