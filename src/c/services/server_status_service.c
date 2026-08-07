#include "server_status_service.h"

ServerStatus server_status_service_get(void)
{
    return (ServerStatus) {
        .has_attention = false,
        .server_online = true,

        .cpu_percent = 2.1f,
        .ram_percent = 34.2f,
        .temperature_celsius = 38.6f,
        .load_average = 0.08f,

        .uptime_text = "8 days",
        .updated_text = "5 sec ago",

        .storage_used_percent = 16,

        .storage_used_tb = 1.8f,
        .storage_free_tb = 9.3f,
        .storage_total_tb = 11.1f,

        .movies_gb = 389.0f,
        .tv_gb = 734.0f,
        .immich_gb = 124.0f,
        .downloads_gb = 4.0f,
        .other_gb = 560.0f,

        .active_downloads = 0,

        .services = {
            { "Jellyfin",    true },
            { "qBittorrent", true },
            { "Sonarr",      true },
            { "Radarr",      true },
            { "Prowlarr",    true },
            { "Immich",      true },
        },
    };
}
