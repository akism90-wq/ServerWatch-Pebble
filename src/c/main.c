#include <pebble.h>
#include "services/server_status_service.h"
#include "windows/home_window.h"
#include "windows/server_window.h"
#include "windows/storage_window.h"
#include "windows/downloads_window.h"

static Window *s_home_window;

static void prv_set_attention_item_from_message(
    DictionaryIterator *iterator,
    int index,
    uint32_t text_key,
    uint32_t severity_key)
{
    Tuple *text_tuple =
        dict_find(iterator, text_key);

    Tuple *severity_tuple =
        dict_find(iterator, severity_key);

    if ((text_tuple == NULL) ||
        (severity_tuple == NULL))
    {
        return;
    }

    server_status_service_set_attention_item(
        index,
        text_tuple->value->cstring,
        (AttentionSeverity)severity_tuple->value->int32);
}

static void prv_inbox_received_handler(
    DictionaryIterator *iterator,
    void *context)
{
    (void)context;

    Tuple *connection_state_tuple =
        dict_find(
            iterator,
            MESSAGE_KEY_connectionState
        );

    if (connection_state_tuple != NULL)
    {
        const int32_t state_value =
            connection_state_tuple->value->int32;

        if (state_value == 1)
        {
            server_status_service_set_connection_state(
                CONNECTION_STATE_CONNECTED
            );

            APP_LOG(
                APP_LOG_LEVEL_INFO,
                "Connection state: connected"
            );
        }
        else if (state_value == 2)
        {
            server_status_service_set_connection_state(
                CONNECTION_STATE_FAILED
            );

            APP_LOG(
                APP_LOG_LEVEL_INFO,
                "Connection state: failed"
            );
        }
    }

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

    Tuple *attention_count_tuple =
        dict_find(iterator, MESSAGE_KEY_attentionCount);

    if (attention_count_tuple != NULL)
    {
        server_status_service_set_attention_item_count(
            attention_count_tuple->value->int32);
    }
    
    prv_set_attention_item_from_message(
        iterator,
        0,
        MESSAGE_KEY_attention0Text,
        MESSAGE_KEY_attention0Severity);

    prv_set_attention_item_from_message(
        iterator,
        1,
        MESSAGE_KEY_attention1Text,
        MESSAGE_KEY_attention1Severity);

    prv_set_attention_item_from_message(
        iterator,
        2,
        MESSAGE_KEY_attention2Text,
        MESSAGE_KEY_attention2Severity);

    prv_set_attention_item_from_message(
        iterator,
        3,
        MESSAGE_KEY_attention3Text,
        MESSAGE_KEY_attention3Severity);

    prv_set_attention_item_from_message(
        iterator,
        4,
        MESSAGE_KEY_attention4Text,
        MESSAGE_KEY_attention4Severity);

    prv_set_attention_item_from_message(
        iterator,
        5,
        MESSAGE_KEY_attention5Text,
        MESSAGE_KEY_attention5Severity);

    prv_set_attention_item_from_message(
        iterator,
        6,
        MESSAGE_KEY_attention6Text,
        MESSAGE_KEY_attention6Severity);

    prv_set_attention_item_from_message(
        iterator,
        7,
        MESSAGE_KEY_attention7Text,
        MESSAGE_KEY_attention7Severity);    
    
    Tuple *download_count_tuple =
        dict_find(iterator, MESSAGE_KEY_downloadCount);

    Tuple *download0_name_tuple =
        dict_find(iterator, MESSAGE_KEY_download0Name);

    Tuple *download0_subtitle_tuple =
        dict_find(iterator, MESSAGE_KEY_download0Subtitle);

    Tuple *download0_quality_tuple =
        dict_find(iterator, MESSAGE_KEY_download0Quality);

    Tuple *download0_state_tuple =
        dict_find(iterator, MESSAGE_KEY_download0State);

    Tuple *download0_progress_tuple =
        dict_find(iterator, MESSAGE_KEY_download0ProgressPercent);

    Tuple *download0_speed_tuple =
        dict_find(iterator, MESSAGE_KEY_download0SpeedText);

    Tuple *download0_eta_tuple =
        dict_find(iterator, MESSAGE_KEY_download0EtaText);

    Tuple *download0_size_tuple =
        dict_find(iterator, MESSAGE_KEY_download0SizeMb);

    Tuple *download0_suspicious_tuple =
        dict_find(iterator, MESSAGE_KEY_download0Suspicious);

    Tuple *download1_name_tuple =
        dict_find(iterator, MESSAGE_KEY_download1Name);

    Tuple *download1_subtitle_tuple =
        dict_find(iterator, MESSAGE_KEY_download1Subtitle);

    Tuple *download1_quality_tuple =
        dict_find(iterator, MESSAGE_KEY_download1Quality);

    Tuple *download1_state_tuple =
        dict_find(iterator, MESSAGE_KEY_download1State);

    Tuple *download1_progress_tuple =
        dict_find(iterator, MESSAGE_KEY_download1ProgressPercent);

    Tuple *download1_speed_tuple =
        dict_find(iterator, MESSAGE_KEY_download1SpeedText);

    Tuple *download1_eta_tuple =
        dict_find(iterator, MESSAGE_KEY_download1EtaText);

    Tuple *download1_size_tuple =
        dict_find(iterator, MESSAGE_KEY_download1SizeMb);

    Tuple *download1_suspicious_tuple =
        dict_find(iterator, MESSAGE_KEY_download1Suspicious);

    Tuple *download2_name_tuple =
        dict_find(iterator, MESSAGE_KEY_download2Name);

    Tuple *download2_subtitle_tuple =
        dict_find(iterator, MESSAGE_KEY_download2Subtitle);

    Tuple *download2_quality_tuple =
        dict_find(iterator, MESSAGE_KEY_download2Quality);

    Tuple *download2_state_tuple =
        dict_find(iterator, MESSAGE_KEY_download2State);

    Tuple *download2_progress_tuple =
        dict_find(iterator, MESSAGE_KEY_download2ProgressPercent);

    Tuple *download2_speed_tuple =
        dict_find(iterator, MESSAGE_KEY_download2SpeedText);

    Tuple *download2_eta_tuple =
        dict_find(iterator, MESSAGE_KEY_download2EtaText);

    Tuple *download2_size_tuple =
        dict_find(iterator, MESSAGE_KEY_download2SizeMb);

    Tuple *download2_suspicious_tuple =
        dict_find(iterator, MESSAGE_KEY_download2Suspicious);

    if (download_count_tuple != NULL)
    {
        server_status_service_set_active_download_count(
            download_count_tuple->value->int32);
    }

    if ((download0_name_tuple != NULL) &&
        (download0_subtitle_tuple != NULL) &&
        (download0_quality_tuple != NULL) &&
        (download0_state_tuple != NULL) &&
        (download0_progress_tuple != NULL) &&
        (download0_speed_tuple != NULL) &&
        (download0_eta_tuple != NULL) &&
        (download0_size_tuple != NULL) &&
        (download0_suspicious_tuple != NULL))
    {
        server_status_service_set_download(
            0,
            download0_name_tuple->value->cstring,
            download0_subtitle_tuple->value->cstring,
            download0_quality_tuple->value->cstring,
            download0_state_tuple->value->cstring,
            download0_progress_tuple->value->int32,
            download0_speed_tuple->value->cstring,
            download0_eta_tuple->value->cstring,
            (uint32_t)download0_size_tuple->value->int32,
            download0_suspicious_tuple->value->int32 != 0);

        APP_LOG(
            APP_LOG_LEVEL_INFO,
            "Received live download slot 0");
    }
    
    if ((download1_name_tuple != NULL) &&
        (download1_subtitle_tuple != NULL) &&
        (download1_quality_tuple != NULL) &&
        (download1_state_tuple != NULL) &&
        (download1_progress_tuple != NULL) &&
        (download1_speed_tuple != NULL) &&
        (download1_eta_tuple != NULL) &&
        (download1_size_tuple != NULL) &&
        (download1_suspicious_tuple != NULL))
    {
        server_status_service_set_download(
            1,
            download1_name_tuple->value->cstring,
            download1_subtitle_tuple->value->cstring,
            download1_quality_tuple->value->cstring,
            download1_state_tuple->value->cstring,
            download1_progress_tuple->value->int32,
            download1_speed_tuple->value->cstring,
            download1_eta_tuple->value->cstring,
            (uint32_t)download1_size_tuple->value->int32,
            download1_suspicious_tuple->value->int32 != 0);

        APP_LOG(
            APP_LOG_LEVEL_INFO,
            "Received live download slot 1");
    }

    if ((download2_name_tuple != NULL) &&
        (download2_subtitle_tuple != NULL) &&
        (download2_quality_tuple != NULL) &&
        (download2_state_tuple != NULL) &&
        (download2_progress_tuple != NULL) &&
        (download2_speed_tuple != NULL) &&
        (download2_eta_tuple != NULL) &&
        (download2_size_tuple != NULL) &&
        (download2_suspicious_tuple != NULL))
    {
        server_status_service_set_download(
            2,
            download2_name_tuple->value->cstring,
            download2_subtitle_tuple->value->cstring,
            download2_quality_tuple->value->cstring,
            download2_state_tuple->value->cstring,
            download2_progress_tuple->value->int32,
            download2_speed_tuple->value->cstring,
            download2_eta_tuple->value->cstring,
            (uint32_t)download2_size_tuple->value->int32,
            download2_suspicious_tuple->value->int32 != 0);

        APP_LOG(
            APP_LOG_LEVEL_INFO,
            "Received live download slot 2");
    }

    /*
    * A real status snapshot always contains serverName.
    * Connection-state-only messages must not advance
    * the last-good-snapshot timestamp.
    */
    if (server_name_tuple != NULL)
    {
        server_status_service_mark_updated();
    }

    attention_window_refresh();
    server_window_refresh();
    storage_window_refresh();
    downloads_window_refresh();
}

static void prv_request_refresh(void)
{
    DictionaryIterator *iterator = NULL;

    const AppMessageResult result =
        app_message_outbox_begin(&iterator);

    if ((result == APP_MSG_OK) &&
        (iterator != NULL))
    {
        dict_write_uint8(
            iterator,
            MESSAGE_KEY_refreshRequest,
            1);

        app_message_outbox_send();
    }
}

static void prv_app_focus_changed(bool in_focus)
{
    if (in_focus)
    {
        prv_request_refresh();
    }
}

static void prv_init(void)
{
    app_message_register_inbox_received(
        prv_inbox_received_handler);

    const AppMessageResult open_result =
        app_message_open(1024, 128);

    app_focus_service_subscribe(
        prv_app_focus_changed);

    APP_LOG(
        APP_LOG_LEVEL_INFO,
        "AppMessage open result: %d",
        open_result);

    s_home_window = home_window_create();
    window_stack_push(s_home_window, true);

    prv_request_refresh();
}

static void prv_deinit(void)
{
    app_focus_service_unsubscribe();
    home_window_destroy(s_home_window);
    s_home_window = NULL;
}

int main(void)
{
    prv_init();
    app_event_loop();
    prv_deinit();
}