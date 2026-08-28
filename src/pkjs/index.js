/* global Pebble */

var config = require("./config.local");

var requestInFlight = false;
var requestSequence = 0;
var refreshQueued = false;
var deleteInFlight = false;

function getDeleteUrl() {
    return config.agentUrl.replace(
        /\/status\/?$/,
        "/downloads/delete"
    );
}

function isServiceUp(services, name) {
    for (var i = 0; i < services.length; i++) {
        if (services[i].name === name) {
            return services[i].state === "Up" ? 1 : 0;
        }
    }

    return 0;
}

function isServiceMonitored(services, name) {
    for (var i = 0; i < services.length; i++) {
        if (services[i].name === name) {
            return 1;
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

function getAttentionSeverity(alert) {
    if (alert.severity === "Problem") {
        return 2;
    }

    if (alert.severity === "Warning") {
        return 1;
    }

    return 0;
}

function getAttention(alerts, index) {
    return index < alerts.length && index < 8
        ? alerts[index]
        : null;
}

function buildStatusMessage(status) {
    var prioritizedDownloads =
        getPrioritizedDownloads(status.downloads);

    var attentionCount = Math.min(
        status.alerts.length,
        8
    );

    var attention0 = getAttention(status.alerts, 0);
    var attention1 = getAttention(status.alerts, 1);
    var attention2 = getAttention(status.alerts, 2);
    var attention3 = getAttention(status.alerts, 3);
    var attention4 = getAttention(status.alerts, 4);
    var attention5 = getAttention(status.alerts, 5);
    var attention6 = getAttention(status.alerts, 6);
    var attention7 = getAttention(status.alerts, 7);

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

        "chaptarrOnline":
            isServiceUp(status.services, "Chaptarr"),

        "chaptarrMonitored":
            isServiceMonitored(status.services, "Chaptarr"),

        "audiobookshelfOnline":
            isServiceUp(status.services, "Audiobookshelf"),

        "audiobookshelfMonitored":
            isServiceMonitored(status.services, "Audiobookshelf"),

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

        "attentionCount":
            attentionCount,

        "downloadCount":
            status.downloads.length,

        "attention0Text":
            attention0
                ? attention0.title
                : "",

        "attention0Severity":
            attention0
                ? getAttentionSeverity(attention0)
                : 0,

        "attention1Text":
            attention1
                ? attention1.title
                : "",

        "attention1Severity":
            attention1
                ? getAttentionSeverity(attention1)
                : 0,
                
        "attention2Text":
            attention2
                ? attention2.title
                : "",

        "attention2Severity":
            attention2
                ? getAttentionSeverity(attention2)
                : 0,

        "attention3Text":
            attention3
                ? attention3.title
                : "",

        "attention3Severity":
            attention3
                ? getAttentionSeverity(attention3)
                : 0,

        "attention4Text":
            attention4
                ? attention4.title
                : "",

        "attention4Severity":
            attention4
                ? getAttentionSeverity(attention4)
                : 0,

        "attention5Text":
            attention5
                ? attention5.title
                : "",

        "attention5Severity":
            attention5
                ? getAttentionSeverity(attention5)
                : 0,

        "attention6Text":
            attention6
                ? attention6.title
                : "",
                
        "attention6Severity":
            attention6
                ? getAttentionSeverity(attention6)
                : 0,

        "attention7Text":
            attention7
                ? attention7.title
                : "",

        "attention7Severity":
            attention7
                ? getAttentionSeverity(attention7)
                : 0,               

        "download0Name":
            download0 ? download0.title : "",

        "download0Hash":
            download0 ? download0.hash : "",

        "download1Name":
            download1 ? download1.title : "",

        "download1Hash":
            download1 ? download1.hash : "",

        "download2Name":
            download2 ? download2.title : "",

        "download2Hash":
            download2 ? download2.hash : "",

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

function fetchQueuedServerStatus() {
    if (!refreshQueued || requestInFlight) {
        return;
    }

    refreshQueued = false;
    fetchServerStatus("queued refresh");
}

function sendDeleteResult(success, errorMessage) {
    Pebble.sendAppMessage(
        {
            "deleteResult": success ? 1 : 2,
            "deleteError": errorMessage || ""
        },
        function () {
            console.log(
                "Delete result sent: " +
                (success ? "success" : "failure")
            );
        },
        function (error) {
            console.log(
                "Delete result failed: " +
                JSON.stringify(error)
            );
        }
    );
}

function deleteDownload(hash) {
    if (!hash) {
        sendDeleteResult(false, "Missing torrent hash");
        return;
    }

    if (deleteInFlight) {
        sendDeleteResult(false, "Delete already running");
        return;
    }

    deleteInFlight = true;

    var request = new XMLHttpRequest();
    var requestFinished = false;

    request.open(
        "POST",
        getDeleteUrl(),
        true
    );

    request.setRequestHeader(
        "Content-Type",
        "application/json"
    );

    request.setRequestHeader(
        "X-ServerWatch-Api-Key",
        config.apiKey
    );

    var watchdog = setTimeout(function () {
        if (requestFinished) {
            return;
        }

        requestFinished = true;
        deleteInFlight = false;

        console.log(
            "ServerWatch delete watchdog expired"
        );

        sendDeleteResult(false, "Delete timed out");

        try {
            request.abort();
        } catch (error) {
            console.log(
                "ServerWatch delete abort failed: " +
                error
            );
        }
    }, 4000);

    request.onload = function () {
        if (requestFinished) {
            return;
        }

        requestFinished = true;
        deleteInFlight = false;
        clearTimeout(watchdog);

        console.log(
            "ServerWatch delete response: " +
            request.status
        );

        if (request.status < 200 ||
            request.status >= 300) {
            sendDeleteResult(
                false,
                "Delete failed"
            );
            return;
        }

        sendDeleteResult(true, "");
        fetchServerStatus("delete success");
    };

    request.onerror = function () {
        if (requestFinished) {
            return;
        }

        requestFinished = true;
        deleteInFlight = false;
        clearTimeout(watchdog);

        console.log(
            "ServerWatch delete request failed"
        );

        sendDeleteResult(false, "Delete failed");
    };

    request.send(
        JSON.stringify(
            {
                hash: hash
            }
        )
    );
}

function fetchServerStatus(reason) {
    if (requestInFlight) {
        console.log(
            "ServerWatch fetch skipped while request is in flight: " +
            reason
        );

        refreshQueued = true;
        return;
    }

    requestInFlight = true;

    var requestId = ++requestSequence;
    var request = new XMLHttpRequest();
    var request_finished = false;

    console.log(
        "ServerWatch XHR starting #" +
        requestId +
        ": " +
        reason
    );

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
        requestInFlight = false;

        console.log(
            "ServerWatch HTTP watchdog expired #" +
            requestId
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

        fetchQueuedServerStatus();
    }, 4000);

    request.onload = function () {
        if (request_finished) {
            return;
        }

        request_finished = true;
        requestInFlight = false;
        clearTimeout(watchdog);

        console.log(
            "ServerWatch HTTP response #" +
            requestId +
            ": " +
            request.status
        );

        if (request.status !== 200) {
            console.log(
                "ServerWatch request failed: " +
                request.responseText
            );

            sendConnectionState(2);
            fetchQueuedServerStatus();
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
            fetchQueuedServerStatus();
        } catch (error) {
            console.log(
                "ServerWatch JSON parse failed: " +
                error
            );

            sendConnectionState(2);
            fetchQueuedServerStatus();
        }
    };

    request.onerror = function () {
        if (request_finished) {
            return;
        }

        request_finished = true;
        requestInFlight = false;
        clearTimeout(watchdog);

        console.log(
            "ServerWatch HTTP request failed #" +
            requestId
        );

        sendConnectionState(2);
        fetchQueuedServerStatus();
    };

    request.send();
}

Pebble.addEventListener("ready", function () {
    console.log("ServerWatch PebbleKit JS ready");

    fetchServerStatus("ready");

    setInterval(function () {
        console.log("ServerWatch poll timer fired");

        fetchServerStatus("interval");
    }, 5000);
});

Pebble.addEventListener(
    "appmessage",
    function (event) {
        if (event.payload.refreshRequest === 1) {
            console.log(
                "ServerWatch refresh requested by watch"
            );

            fetchServerStatus("watch refresh");
        }

        if (event.payload.deleteDownloadHash) {
            console.log(
                "ServerWatch delete requested by watch"
            );

            deleteDownload(
                event.payload.deleteDownloadHash
            );
        }
    }
);
