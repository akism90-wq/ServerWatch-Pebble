#include "server_status_service.h"

ServerStatus server_status_service_get(void)
{
    return (ServerStatus) {
        .has_attention = false,
        .server_online = true,
        .storage_used_percent = 16,
        .active_downloads = 0,
    };
}
