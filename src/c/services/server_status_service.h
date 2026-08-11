#pragma once

#include "../models/server_status.h"

const ServerStatus *server_status_service_get(void);

void server_status_service_set_server_name(const char *name);   

void server_status_service_set_server_online(bool online);

void server_status_service_set_system_metrics(
    int32_t cpu_tenths,
    int32_t ram_tenths,
    int32_t temperature_tenths,
    int32_t load_hundredths,
    uint32_t uptime_seconds
);

void server_status_service_set_service_states(
    bool jellyfin_online,
    bool qbittorrent_online,
    bool sonarr_online,
    bool radarr_online,
    bool prowlarr_online,
    bool immich_online
);

void server_status_service_mark_updated(void);

uint32_t server_status_service_get_update_age_seconds(void);

void server_status_service_format_update_age(
    char *buffer,
    size_t buffer_size
);

void server_status_service_set_storage(
    int storage_used_percent,
    bool storage_warning,
    int32_t storage_used_tenths_gb,
    int32_t storage_free_tenths_gb,
    int32_t storage_total_tenths_gb,
    int32_t movies_tenths_gb,
    int32_t tv_tenths_gb,
    int32_t immich_tenths_gb,
    int32_t downloads_tenths_gb,
    int32_t other_tenths_gb
);