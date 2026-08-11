/* global Pebble */

var config = require("./config.local");

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

            function isServiceUp(services, name) {
                for (var i = 0; i < services.length; i++) {
                    if (services[i].name === name) {
                        return services[i].state === "Up" ? 1 : 0;
                    }
                }

                return 0;
            }

            Pebble.sendAppMessage(
                {
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
                        isServiceUp(status.services, "Immich")
                },
                function () {
                    console.log("Live server name sent");
                },
                function (error) {
                    console.log(
                        "Live server name failed: " +
                        JSON.stringify(error)
                    );
                }
            );
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
