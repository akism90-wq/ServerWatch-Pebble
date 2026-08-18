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

function formatDownloadSpeed(bytesPerSecond) {
    if (bytesPerSecond >= 1000000) {
        return (
            (bytesPerSecond / 1000000).toFixed(1) +
            " MB/s"
        );
    }

    if (bytesPerSecond >= 1000) {
        return (
            (bytesPerSecond / 1000).toFixed(1) +
            " KB/s"
        );
    }

    return bytesPerSecond + " B/s";
}

function formatEta(seconds) {
    if (seconds <= 0) {
        return "--";
    }

    var days = Math.floor(seconds / 86400);
    var hours = Math.floor(
        (seconds % 86400) / 3600
    );
    var minutes = Math.floor(
        (seconds % 3600) / 60
    );

    if (days > 0) {
        return days + "d " + hours + "h";
    }

    if (hours > 0) {
        return hours + "h " + minutes + "m";
    }

    return minutes + "m";
}

function getDownload(downloads, index) {
    return index < downloads.length
        ? downloads[index]
        : null;
}

function getDownloadPriority(download) {
    if (download === null) {
        return 99;
    }

    if (download.state === "Downloading") {
        return 0;
    }

    if (download.state === "Forced downloading") {
        return 0;
    }

    if (download.state === "Stalled") {
        return 1;
    }

    if (download.state === "Checking") {
        return 1;
    }

    if (download.state === "Allocating") {
        return 1;
    }

    if (download.state === "Queued") {
        return 2;
    }

    return 3;
}

function getPrioritizedDownloads(downloads) {
    var result = downloads.slice();

    result.sort(function (a, b) {
        return getDownloadPriority(a) -
            getDownloadPriority(b);
    });

    return result;
}

function buildStatusMessage(status) {
    var prioritizedDownloads =
        getPrioritizedDownloads(status.downloads);

    var download0 =
        getDownload(prioritizedDownloads, 0);

    var download1 =
        getDownload(prioritizedDownloads, 1);

    var download2 =
        getDownload(prioritizedDownloads, 2);

    return {
        "serverName": status.server.name,
        "serverOnline": status.server.online ? 1 : 0,
        "connectionState": 1,

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
            ),

        "downloadCount":
            status.downloads.length,

        "download0Name":
            download0 ? download0.title : "",

        "download1Name":
            download1 ? download1.title : "",

        "download2Name":
            download2 ? download2.title : "",

        "download0Subtitle":
            download0
                ? download0.subtitle
                : "",

        "download1Subtitle":
            download1
                ? download1.subtitle
                : "",

        "download2Subtitle":
            download2
                ? download2.subtitle
                : "",

        "download0Quality":
            download0
                ? download0.quality
                : "",

        "download1Quality":
            download1
                ? download1.quality
                : "",

        "download2Quality":
            download2
                ? download2.quality
                : "",

        "download0State":
            download0
                ? download0.state
                : "",

        "download1State":
            download1
                ? download1.state
                : "",

        "download2State":
            download2
                ? download2.state
                : "",

        "download0ProgressPercent":
            download0
                ? download0.progressPercent
                : 0,

        "download1ProgressPercent":
            download1
                ? download1.progressPercent
                : 0,

        "download2ProgressPercent":
            download2
                ? download2.progressPercent
                : 0,

        "download0SpeedText":
            download0
                ? formatDownloadSpeed(
                    download0.downloadSpeedBytesPerSecond
                )
                : "",

        "download1SpeedText":
            download1
                ? formatDownloadSpeed(
                    download1.downloadSpeedBytesPerSecond
                )
                : "",

        "download2SpeedText":
            download2
                ? formatDownloadSpeed(
                    download2.downloadSpeedBytesPerSecond
                )
                : "",

        "download0EtaText":
            download0
                ? formatEta(
                    download0.etaSeconds
                )
                : "",

        "download1EtaText":
            download1
                ? formatEta(
                    download1.etaSeconds
                )
                : "",

        "download2EtaText":
            download2
                ? formatEta(
                    download2.etaSeconds
                )
                : "",

        "download0SizeMb":
            download0
                ? Math.round(
                    download0.sizeGb * 1000
                )
                : 0,

        "download1SizeMb":
            download1
                ? Math.round(
                    download1.sizeGb * 1000
                )
                : 0,

        "download2SizeMb":
            download2
                ? Math.round(
                    download2.sizeGb * 1000
                )
                : 0,

        "download0Suspicious":
            download0 && download0.suspicious
                ? 1
                : 0,

        "download1Suspicious":
            download1 && download1.suspicious
                ? 1
                : 0,

        "download2Suspicious":
            download2 && download2.suspicious
                ? 1
                : 0
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

function sendConnectionState(state) {
    Pebble.sendAppMessage(
        {
            "connectionState": state
        },
        function () {
            console.log(
                "Connection state sent: " +
                state
            );
        },
        function (error) {
            console.log(
                "Connection state failed: " +
                JSON.stringify(error)
            );
        }
    );
}

function fetchServerStatus() {
    var request = new XMLHttpRequest();
    var request_finished = false;

    request.open(
        "GET",
        config.agentUrl,
        true
    );

    request.setRequestHeader(
        "X-ServerWatch-Api-Key",
        config.apiKey
    );

    /*
     * PebbleKit JS does not reliably honour XMLHttpRequest.timeout,
     * so use an explicit watchdog to detect an unavailable Agent.
     */
    var watchdog = setTimeout(function () {
        if (request_finished) {
            return;
        }

        request_finished = true;

        console.log(
            "ServerWatch HTTP watchdog expired"
        );

        sendConnectionState(2);

        try {
            request.abort();
        } catch (error) {
            console.log(
                "ServerWatch request abort failed: " +
                error
            );
        }
    }, 4000);

    request.onload = function () {
        if (request_finished) {
            return;
        }

        request_finished = true;
        clearTimeout(watchdog);

        console.log(
            "ServerWatch HTTP response: " +
            request.status
        );

        if (request.status !== 200) {
            console.log(
                "ServerWatch request failed: " +
                request.responseText
            );

            sendConnectionState(2);
            return;
        }

        try {
            var status =
                JSON.parse(request.responseText);

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

            sendConnectionState(2);
        }
    };

    request.onerror = function () {
        if (request_finished) {
            return;
        }

        request_finished = true;
        clearTimeout(watchdog);

        console.log(
            "ServerWatch HTTP request failed"
        );

        sendConnectionState(2);
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

Pebble.addEventListener(
    "appmessage",
    function (event) {
        if (event.payload.refreshRequest === 1) {
            console.log(
                "ServerWatch refresh requested by watch"
            );

            fetchServerStatus();
        }
    }
);