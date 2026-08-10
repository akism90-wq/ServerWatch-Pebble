#include <string.h>
#include <stdio.h>
#include "server_status_service.h"

/*
 * ServerWatch owns one persistent status snapshot.
 *
 * Today this is initialised with development data. In v0.5.0 the
 * AppMessage receive path will update this same object with live data.
 *
 * Keeping the object here also guarantees that strings stored inside
 * ServerStatus remain valid for as long as the application is running.
 */
static ServerStatus s_status = {
    .server_name = "ServerWatch",
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

const ServerStatus *server_status_service_get(void)
{
    return &s_status;
}

void server_status_service_set_server_name(const char *name)
{
    if (name == NULL)
    {
        return;
    }

    strncpy(
        s_status.server_name,
        name,
        sizeof(s_status.server_name) - 1
    );

    s_status.server_name[sizeof(s_status.server_name) - 1] = '\0';
}

void server_status_service_set_server_online(bool online)
{
    s_status.server_online = online;
}

static void prv_format_uptime(
    uint32_t uptime_seconds,
    char *buffer,
    size_t buffer_size)
{
    const uint32_t days =
        uptime_seconds / 86400U;

    const uint32_t hours =
        (uptime_seconds % 86400U) / 3600U;

    const uint32_t minutes =
        (uptime_seconds % 3600U) / 60U;

    if (days > 0U)
    {
        snprintf(
            buffer,
            buffer_size,
            "%lud %luh",
            (unsigned long)days,
            (unsigned long)hours);
    }
    else if (hours > 0U)
    {
        snprintf(
            buffer,
            buffer_size,
            "%luh %lum",
            (unsigned long)hours,
            (unsigned long)minutes);
    }
    else
    {
        snprintf(
            buffer,
            buffer_size,
            "%lum",
            (unsigned long)minutes);
    }
}

void server_status_service_set_system_metrics(
    int32_t cpu_tenths,
    int32_t ram_tenths,
    int32_t temperature_tenths,
    int32_t load_hundredths,
    uint32_t uptime_seconds)
{
    s_status.cpu_percent =
        (float)cpu_tenths / 10.0f;

    s_status.ram_percent =
        (float)ram_tenths / 10.0f;

    s_status.temperature_celsius =
        (float)temperature_tenths / 10.0f;

    s_status.load_average =
        (float)load_hundredths / 100.0f;

    prv_format_uptime(
        uptime_seconds,
        s_status.uptime_text,
        sizeof(s_status.uptime_text));
}

void server_status_service_set_service_states(
    bool jellyfin_online,
    bool qbittorrent_online,
    bool sonarr_online,
    bool radarr_online,
    bool prowlarr_online,
    bool immich_online)
{
    s_status.services[0].online = jellyfin_online;
    s_status.services[1].online = qbittorrent_online;
    s_status.services[2].online = sonarr_online;
    s_status.services[3].online = radarr_online;
    s_status.services[4].online = prowlarr_online;
    s_status.services[5].online = immich_online;
}