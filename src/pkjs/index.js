/* global Pebble */

var config = require("./config.local");

function isServiceUp(services, name) {
    for (var i = 0; i < services.length; i++) {
        if (services[i].name === name) {
            return services[i].state === "Up" ? 1 : 0;
        }
    }

    return 0;
}

function getStorageCategoryUsedGb(categories, name) {
    for (var i = 0; i < categories.length; i++) {
        if (categories[i].name === name) {
            return categories[i].usedGb;
        }
    }

    return 0;
}

function buildStatusMessage(status) {
    return {
        "serverName": status.server.name,
        "serverOnline": status.server.online ? 1 : 0,

        "cpuTenths":
            Math.round(status.systemMetrics.cpuUsagePercent * 10),

        "ramTenths":
            Math.round(
                (status.systemMetrics.memoryUsedGb /
                    status.systemMetrics.memoryTotalGb) *
                1000
            ),

        "temperatureTenths":
            Math.round(
                status.systemMetrics.temperatureCelsius * 10
            ),

        "loadHundredths":
            Math.round(
                status.systemMetrics.loadAverage * 100
            ),

        "uptimeSeconds":
            status.systemMetrics.uptimeSeconds,

        "jellyfinOnline":
            isServiceUp(status.services, "Jellyfin"),

        "qbittorrentOnline":
            isServiceUp(status.services, "qBittorrent"),

        "sonarrOnline":
            isServiceUp(status.services, "Sonarr"),

        "radarrOnline":
            isServiceUp(status.services, "Radarr"),

        "prowlarrOnline":
            isServiceUp(status.services, "Prowlarr"),

        "immichOnline":
            isServiceUp(status.services, "Immich"),

        "storageUsedPercent":
            status.storage.usedPercent,

        "storageWarning":
            status.storage.warning ? 1 : 0,

        "storageUsedTenthsGb":
            Math.round(status.storage.usedGb * 10),

        "storageFreeTenthsGb":
            Math.round(status.storage.freeGb * 10),

        "storageTotalTenthsGb":
            Math.round(status.storage.totalGb * 10),

        "moviesTenthsGb":
            Math.round(
                getStorageCategoryUsedGb(
                    status.storage.categories,
                    "Movies"
                ) * 10
            ),

        "tvTenthsGb":
            Math.round(
                getStorageCategoryUsedGb(
                    status.storage.categories,
                    "TV"
                ) * 10
            ),

        "immichTenthsGb":
            Math.round(
                getStorageCategoryUsedGb(
                    status.storage.categories,
                    "Immich"
                ) * 10
            ),

        "downloadsTenthsGb":
            Math.round(
                getStorageCategoryUsedGb(
                    status.storage.categories,
                    "Downloads"
                ) * 10
            ),

        "otherTenthsGb":
            Math.round(
                getStorageCategoryUsedGb(
                    status.storage.categories,
                    "Other"
                ) * 10
            )
    };
}

function sendStatusToPebble(status) {
    Pebble.sendAppMessage(
        buildStatusMessage(status),
        function () {
            console.log("Live ServerWatch status sent");
        },
        function (error) {
            console.log(
                "Live ServerWatch status failed: " +
                JSON.stringify(error)
            );
        }
    );
}

function fetchServerStatus() {
    var request = new XMLHttpRequest();

    request.open("GET", config.agentUrl, true);

    request.setRequestHeader(
        "X-ServerWatch-Api-Key",
        config.apiKey
    );

    request.onload = function () {
        console.log(
            "ServerWatch HTTP response: " +
            request.status
        );

        if (request.status !== 200) {
            console.log(
                "ServerWatch request failed: " +
                request.responseText
            );
            return;
        }

        try {
            var status = JSON.parse(request.responseText);

            console.log(
                "ServerWatch JSON parsed. Server: " +
                status.server.name
            );

            sendStatusToPebble(status);
        } catch (error) {
            console.log(
                "ServerWatch JSON parse failed: " +
                error
            );
        }
    };

    request.onerror = function () {
        console.log("ServerWatch HTTP request failed");
    };

    request.send();
}

Pebble.addEventListener("ready", function () {
    console.log("ServerWatch PebbleKit JS ready");

    fetchServerStatus();

    setInterval(function () {
        fetchServerStatus();
    }, 5000);
});