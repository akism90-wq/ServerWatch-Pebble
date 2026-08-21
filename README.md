# ServerWatch for Pebble

ServerWatch is a native Pebble watch application for monitoring a self-hosted server directly from your wrist.

The Pebble client connects to the ServerWatch Agent through PebbleKit JS and presents server health, system metrics, service status, storage usage, active downloads, alerts, and connection state in an interface designed specifically for modern Pebble hardware.

ServerWatch Pebble v1.0.0 targets **Pebble Time 2 (emery)** and **Pebble Round 2 (gabbro)**.

## Features

### Attention
A dedicated overview for server/service problems, storage warnings, suspicious download warnings, and other alerts. Alerts use severity-aware paged presentation. When everything is healthy, the screen shows the animated sleeping ServerWatch cat.

### Server
A two-page live dashboard covering CPU, RAM, temperature, load average, uptime, and monitored service health for Jellyfin, qBittorrent, Sonarr, Radarr, Prowlarr, and Immich.

### Storage
A two-page dashboard showing used percentage, used/free/total capacity, storage warning state, and category usage for Movies, TV, Immich, Downloads, and Other.

### Downloads
Selectable cards show parsed title, year/subtitle, quality, size, progress, speed, ETA, remaining size, download state, and suspicious-download indication.

Existing download actions include **Stop** and **Delete download + data**.

## Connection handling

When live Agent data is available, the watch displays the current server state.

If connectivity is lost after data has already been received, ServerWatch preserves the last known data and clearly marks it as cached rather than pretending it is current. If no cached data exists, the application presents a connection-failure state.

The client polls periodically and automatically returns to live data when connectivity is restored.

## Architecture

ServerWatch consists of:

1. **ServerWatch Agent** — runs on the monitored Linux server and exposes an authenticated HTTP API.
2. **ServerWatch Pebble** — the native Pebble C application plus PebbleKit JS bridge.

PebbleKit JS performs HTTP requests from the paired phone and sends status data to the watch using AppMessage.

For remote monitoring, the Agent can be exposed through an authenticated HTTPS endpoint such as a Tailscale Funnel. The API key remains required by the Agent.

Do not expose an unauthenticated ServerWatch Agent to the public internet.

## Requirements

- Pebble SDK / RePebble development environment
- Pebble Time 2 (`emery`) or Pebble Round 2 (`gabbro`)
- Running ServerWatch Agent
- Paired phone capable of running PebbleKit JS
- Network access from the phone to the configured Agent endpoint

## Configuration

Local PebbleKit JS configuration is stored in:

```text
src/pkjs/config.local.js
```

It contains the Agent endpoint and API key and is intentionally excluded from version control.

Copy the supplied example:

```sh
cp src/pkjs/config.example.js src/pkjs/config.local.js
```

Then edit it:

```js
module.exports = {
    agentUrl: "https://your-server.example/serverwatch/status",
    apiKey: "<YOUR_API_KEY_HERE>"
};
```

The endpoint must resolve to the ServerWatch Agent `/status` API and the key must match the Agent configuration.

Never commit `config.local.js` or a real ServerWatch API key.

## Building

```sh
pebble clean
pebble build
```

The `.pbw` is written to `build/`.

## Emulator

```sh
pebble install --emulator emery
```

## Physical watch

```sh
pebble install --phone <PHONE_IP>
```

## Project layout

```text
src/c/           Native Pebble application
src/c/windows/   Home, Attention, Server, Storage and Downloads screens
src/c/ui/        Reusable UI components
src/c/services/  Watch-side ServerWatch state/services
src/pkjs/        PebbleKit JS HTTP/AppMessage bridge
resources/       Application image resources
package.json     Application metadata, targets, resources and message keys
wscript          Pebble build configuration
```

## v1.0.0 scope

v1.0.0 establishes the complete initial monitoring experience:

- Live ServerWatch Agent integration
- Attention dashboard
- Server metrics and service health
- Storage monitoring
- Download monitoring and existing Stop/Delete actions
- Cached-data connection handling and automatic recovery
- Native Pebble paging/navigation
- Pebble Time 2 and Pebble Round 2 targets
- Physical Pebble Time 2 validation

The focus of v1.0.0 is reliable monitoring and a polished native Pebble experience.

## Roadmap

Development continues beyond v1.0.0. Planned work includes:

- **Touch interaction** on supported Pebble hardware
- Richer Pebble-native interactions and controls
- Additional remote-control capabilities
- **Kill/termination capability** where appropriate and safely supported by the ServerWatch Agent
- Further Pebble Round 2 layout refinement
- Continued interaction and connection-state polish

These capabilities are **not part of v1.0.0** unless explicitly described above.

## Security

ServerWatch Agent endpoints use the `X-ServerWatch-Api-Key` HTTP header.

Treat the API key as a secret: keep it in `config.local.js`, never commit it, do not publish it in screenshots/logs/issues/release artifacts, and rotate it if accidentally exposed. Internet-accessible deployments should use HTTPS while retaining Agent authentication.

## Pebble SDK documentation

RePebble SDK documentation, tutorials, and API reference:

<https://developer.repebble.com>

## Status

**ServerWatch Pebble v1.0.0 release candidate**

Validated against a production ServerWatch Agent and on physical Pebble Time 2 hardware.
