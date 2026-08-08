#include "server_status_service.h"

ServerStatus server_status_service_get(void)
{
    return (ServerStatus) {
        .has_attention = true,
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

        .active_downloads = 2,

        .attention_item_count = 2,

        .attention_items = {
            {
                .text = "Large download detected",
                .severity = ATTENTION_SEVERITY_WARNING,
            },
            {
                .text = "qBittorrent offline",
                .severity = ATTENTION_SEVERITY_CRITICAL,
            },
        },

        .downloads = {
            {
                .name = "Ubuntu 24.04",
                .progress_percent = 52,
                .speed_text = "2.4 MB/s",
                .eta_text = "4 min",
                .size_mb = 4800,
                .suspicious = false,
            },
            {
                .name = "Fedora ISO",
                .progress_percent = 14,
                .speed_text = "1.1 MB/s",
                .eta_text = "12 min",
                .size_mb = 38000,
                .suspicious = true,
            },
        },

        .services = {
            { "Jellyfin",    true },
            { "qBittorrent", false },
            { "Sonarr",      true },
            { "Radarr",      true },
            { "Prowlarr",    true },
            { "Immich",      true },
        },
    };
}