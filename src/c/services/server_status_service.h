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
    uint32_t uptime_seconds);

void server_status_service_set_service_states(
    bool jellyfin_online,
    bool qbittorrent_online,
    bool sonarr_online,
    bool radarr_online,
    bool prowlarr_online,
    bool immich_online);