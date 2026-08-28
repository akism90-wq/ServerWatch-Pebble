#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pebble.h>
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
        { "Jellyfin",       true,  true },
        { "qBittorrent",    false, true },
        { "Sonarr",         true,  true },
        { "Radarr",         true,  true },
        { "Prowlarr",       true,  true },
        { "Immich",         true,  true },
        { "Chaptarr",       true,  false },
        { "Audiobookshelf", true,  false },
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
    bool immich_online,
    bool chaptarr_online,
    bool chaptarr_monitored,
    bool audiobookshelf_online,
    bool audiobookshelf_monitored)
{
    s_status.services[0].online = jellyfin_online;
    s_status.services[1].online = qbittorrent_online;
    s_status.services[2].online = sonarr_online;
    s_status.services[3].online = radarr_online;
    s_status.services[4].online = prowlarr_online;
    s_status.services[5].online = immich_online;
    s_status.services[6].online = chaptarr_online;
    s_status.services[6].monitored = chaptarr_monitored;
    s_status.services[7].online = audiobookshelf_online;
    s_status.services[7].monitored = audiobookshelf_monitored;
}

void server_status_service_mark_updated(void)
{
    time_t now = 0;

    now = time(NULL);

    s_status.last_update_time = now;
    s_status.has_received_snapshot = true;
}

uint32_t server_status_service_get_update_age_seconds(void)
{
    const ServerStatus *const status = server_status_service_get();

    if (status->last_update_time == 0)
    {
        return 0U;
    }

    time_t now = 0;

    (void)time_ms(&now, NULL);

    if (now <= status->last_update_time)
    {
        return 0U;
    }

    return (uint32_t)(now - status->last_update_time);
}

void server_status_service_format_update_age_value(
    char *buffer,
    size_t buffer_size)
{
    const ServerStatus *const status =
        server_status_service_get();

    if ((buffer == NULL) ||
        (buffer_size == 0U))
    {
        return;
    }

    if (status->last_update_time == 0)
    {
        snprintf(
            buffer,
            buffer_size,
            "Never");
        return;
    }

    const uint32_t age_seconds =
        server_status_service_get_update_age_seconds();

    if (age_seconds < 10U)
    {
        snprintf(
            buffer,
            buffer_size,
            "Just now");
    }
    else if (age_seconds < 25U)
    {
        snprintf(
            buffer,
            buffer_size,
            "10 sec ago");
    }
    else if (age_seconds < 60U)
    {
        snprintf(
            buffer,
            buffer_size,
            "25 sec ago");
    }
    else if (age_seconds < 3600U)
    {
        const uint32_t minutes =
            age_seconds / 60U;

        snprintf(
            buffer,
            buffer_size,
            "%lu min ago",
            (unsigned long)minutes);
    }
    else if (age_seconds < 86400U)
    {
        const uint32_t hours =
            age_seconds / 3600U;

        snprintf(
            buffer,
            buffer_size,
            "%lu hr ago",
            (unsigned long)hours);
    }
    else
    {
        const uint32_t days =
            age_seconds / 86400U;

        snprintf(
            buffer,
            buffer_size,
            "%lu day%s ago",
            (unsigned long)days,
            (days == 1U) ? "" : "s");
    }
}

void server_status_service_format_update_age(
    char *buffer,
    size_t buffer_size)
{
    if ((buffer == NULL) ||
        (buffer_size == 0U))
    {
        return;
    }

    char age_text[24];

    server_status_service_format_update_age_value(
        age_text,
        sizeof(age_text));

    snprintf(
        buffer,
        buffer_size,
        "Updated\n%s",
        age_text);
}

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
    int32_t other_tenths_gb)
{
    s_status.storage_used_percent =
        storage_used_percent;

    s_status.storage_warning =
        storage_warning;

    s_status.storage_used_tb =
        (float)storage_used_tenths_gb / 10000.0f;

    s_status.storage_free_tb =
        (float)storage_free_tenths_gb / 10000.0f;

    s_status.storage_total_tb =
        (float)storage_total_tenths_gb / 10000.0f;

    s_status.movies_gb =
        (float)movies_tenths_gb / 10.0f;

    s_status.tv_gb =
        (float)tv_tenths_gb / 10.0f;

    s_status.immich_gb =
        (float)immich_tenths_gb / 10.0f;

    s_status.downloads_gb =
        (float)downloads_tenths_gb / 10.0f;

    s_status.other_gb =
        (float)other_tenths_gb / 10.0f;
}

void server_status_service_set_active_download_count(
    int active_downloads)
{
    s_status.active_downloads =
        active_downloads < 0
            ? 0
            : active_downloads;
}

void server_status_service_set_attention_item_count(
    int attention_item_count)
{
    if (attention_item_count < 0)
    {
        s_status.attention_item_count = 0;
    }
    else if (attention_item_count >
             SERVER_ATTENTION_ITEM_COUNT)
    {
        s_status.attention_item_count =
            SERVER_ATTENTION_ITEM_COUNT;
    }
    else
    {
        s_status.attention_item_count =
            attention_item_count;
    }

    s_status.has_attention =
        s_status.attention_item_count > 0;
}

void server_status_service_set_attention_item(
    int index,
    const char *text,
    AttentionSeverity severity)
{
    if ((index < 0) ||
        (index >= SERVER_ATTENTION_ITEM_COUNT))
    {
        return;
    }

    AttentionItem *const item =
        &s_status.attention_items[index];

    snprintf(
        item->text,
        sizeof(item->text),
        "%s",
        text != NULL ? text : "");

    if (severity > ATTENTION_SEVERITY_CRITICAL)
    {
        item->severity = ATTENTION_SEVERITY_INFO;
    }
    else
    {
        item->severity = severity;
    }
}

void server_status_service_set_download(
    int index,
    const char *name,
    const char *subtitle,
    const char *quality,
    const char *state,
    int progress_percent,
    const char *speed_text,
    const char *eta_text,
    uint32_t size_mb,
    bool suspicious)
{
    if ((index < 0) ||
        (index >= SERVER_DOWNLOAD_COUNT))
    {
        return;
    }

    DownloadStatus *const download =
        &s_status.downloads[index];

    snprintf(
        download->name,
        sizeof(download->name),
        "%s",
        name != NULL ? name : "");

    snprintf(
        download->subtitle,
        sizeof(download->subtitle),
        "%s",
        subtitle != NULL ? subtitle : "");

    snprintf(
        download->quality,
        sizeof(download->quality),
        "%s",
        quality != NULL ? quality : "");

    snprintf(
        download->state,
        sizeof(download->state),
        "%s",
        state != NULL ? state : "");

    if (progress_percent < 0)
    {
        download->progress_percent = 0;
    }
    else if (progress_percent > 100)
    {
        download->progress_percent = 100;
    }
    else
    {
        download->progress_percent = progress_percent;
    }

    snprintf(
        download->speed_text,
        sizeof(download->speed_text),
        "%s",
        speed_text != NULL ? speed_text : "");

    snprintf(
        download->eta_text,
        sizeof(download->eta_text),
        "%s",
        eta_text != NULL ? eta_text : "");

    download->size_mb = size_mb;
    download->suspicious = suspicious;
}

void server_status_service_set_connection_state(
    ConnectionState state)
{
    s_status.connection_state = state;
}

bool server_status_service_has_received_snapshot(void)
{
    return s_status.has_received_snapshot;
}

bool server_status_service_refresh_connection_freshness(
    uint32_t stale_after_seconds)
{
    if (!s_status.has_received_snapshot)
    {
        return false;
    }

    if (s_status.connection_state !=
        CONNECTION_STATE_CONNECTED)
    {
        return false;
    }

    if (server_status_service_get_update_age_seconds() <
        stale_after_seconds)
    {
        return false;
    }

    s_status.connection_state =
        CONNECTION_STATE_FAILED;

    return true;
}
