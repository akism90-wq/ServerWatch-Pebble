#include <pebble.h>
#include "services/server_status_service.h"
#include "windows/home_window.h"
#include "windows/server_window.h"
#include "windows/storage_window.h"

static Window *s_home_window;

static void prv_inbox_received_handler(
    DictionaryIterator *iterator,
    void *context)
{
    (void)context;

    Tuple *server_name_tuple =
        dict_find(iterator, MESSAGE_KEY_serverName);

    if (server_name_tuple != NULL)
    {
        server_status_service_set_server_name(
            server_name_tuple->value->cstring
        );

        APP_LOG(
            APP_LOG_LEVEL_INFO,
            "Received live server name: %s",
            server_name_tuple->value->cstring
        );
    }

    Tuple *server_online_tuple =
    dict_find(iterator, MESSAGE_KEY_serverOnline);

    if (server_online_tuple != NULL)
    {
        server_status_service_set_server_online(
            server_online_tuple->value->int32 != 0
        );

        APP_LOG(
            APP_LOG_LEVEL_INFO,
            "Received live server online state: %ld",
            server_online_tuple->value->int32
        );
    }

    Tuple *cpu_tuple =
    dict_find(iterator, MESSAGE_KEY_cpuTenths);

    Tuple *ram_tuple =
        dict_find(iterator, MESSAGE_KEY_ramTenths);

    Tuple *temperature_tuple =
        dict_find(iterator, MESSAGE_KEY_temperatureTenths);

    Tuple *load_tuple =
        dict_find(iterator, MESSAGE_KEY_loadHundredths);

    Tuple *uptime_tuple =
        dict_find(iterator, MESSAGE_KEY_uptimeSeconds);

    if ((cpu_tuple != NULL) &&
        (ram_tuple != NULL) &&
        (temperature_tuple != NULL) &&
        (load_tuple != NULL) &&
        (uptime_tuple != NULL))
    {
        server_status_service_set_system_metrics(
            cpu_tuple->value->int32,
            ram_tuple->value->int32,
            temperature_tuple->value->int32,
            load_tuple->value->int32,
            uptime_tuple->value->uint32);

            APP_LOG(
                APP_LOG_LEVEL_INFO,
                "Received live system metrics");
    }

    Tuple *jellyfin_tuple =
        dict_find(iterator, MESSAGE_KEY_jellyfinOnline);

    Tuple *qbittorrent_tuple =
        dict_find(iterator, MESSAGE_KEY_qbittorrentOnline);

    Tuple *sonarr_tuple =
        dict_find(iterator, MESSAGE_KEY_sonarrOnline);

    Tuple *radarr_tuple =
        dict_find(iterator, MESSAGE_KEY_radarrOnline);

    Tuple *prowlarr_tuple =
        dict_find(iterator, MESSAGE_KEY_prowlarrOnline);

    Tuple *immich_tuple =
        dict_find(iterator, MESSAGE_KEY_immichOnline);

    if ((jellyfin_tuple != NULL) &&
        (qbittorrent_tuple != NULL) &&
        (sonarr_tuple != NULL) &&
        (radarr_tuple != NULL) &&
        (prowlarr_tuple != NULL) &&
        (immich_tuple != NULL))
    {
        server_status_service_set_service_states(
            jellyfin_tuple->value->int32 != 0,
            qbittorrent_tuple->value->int32 != 0,
            sonarr_tuple->value->int32 != 0,
            radarr_tuple->value->int32 != 0,
            prowlarr_tuple->value->int32 != 0,
            immich_tuple->value->int32 != 0);

        APP_LOG(
            APP_LOG_LEVEL_INFO,
            "Received live service states");
    }

    Tuple *storage_used_percent_tuple =
    dict_find(iterator, MESSAGE_KEY_storageUsedPercent);

    Tuple *storage_warning_tuple =
        dict_find(iterator, MESSAGE_KEY_storageWarning);

    Tuple *storage_used_tuple =
        dict_find(iterator, MESSAGE_KEY_storageUsedTenthsGb);

    Tuple *storage_free_tuple =
        dict_find(iterator, MESSAGE_KEY_storageFreeTenthsGb);

    Tuple *storage_total_tuple =
        dict_find(iterator, MESSAGE_KEY_storageTotalTenthsGb);

    Tuple *movies_tuple =
        dict_find(iterator, MESSAGE_KEY_moviesTenthsGb);

    Tuple *tv_tuple =
        dict_find(iterator, MESSAGE_KEY_tvTenthsGb);

    Tuple *immich_storage_tuple =
        dict_find(iterator, MESSAGE_KEY_immichTenthsGb);

    Tuple *downloads_storage_tuple =
        dict_find(iterator, MESSAGE_KEY_downloadsTenthsGb);

    Tuple *other_tuple =
        dict_find(iterator, MESSAGE_KEY_otherTenthsGb);

    if ((storage_used_percent_tuple != NULL) &&
    (storage_warning_tuple != NULL) &&
    (storage_used_tuple != NULL) &&
    (storage_free_tuple != NULL) &&
    (storage_total_tuple != NULL) &&
    (movies_tuple != NULL) &&
    (tv_tuple != NULL) &&
    (immich_storage_tuple != NULL) &&
    (downloads_storage_tuple != NULL) &&
    (other_tuple != NULL))
    {
        server_status_service_set_storage(
            storage_used_percent_tuple->value->int32,
            storage_warning_tuple->value->int32 != 0,
            storage_used_tuple->value->int32,
            storage_free_tuple->value->int32,
            storage_total_tuple->value->int32,
            movies_tuple->value->int32,
            tv_tuple->value->int32,
            immich_storage_tuple->value->int32,
            downloads_storage_tuple->value->int32,
            other_tuple->value->int32);

            APP_LOG(
            APP_LOG_LEVEL_INFO,
            "Received live storage status");
    }    
    
    server_status_service_mark_updated();
    server_window_refresh();
    storage_window_refresh();
}

static void prv_init(void)
{
    app_message_register_inbox_received(prv_inbox_received_handler);

    const AppMessageResult open_result =
        app_message_open(512, 128);

    APP_LOG(
        APP_LOG_LEVEL_INFO,
        "AppMessage open result: %d",
        open_result);

    s_home_window = home_window_create();
    window_stack_push(s_home_window, true);
}

static void prv_deinit(void)
{
    home_window_destroy(s_home_window);
    s_home_window = NULL;
}

int main(void)
{
    prv_init();
    app_event_loop();
    prv_deinit();
}