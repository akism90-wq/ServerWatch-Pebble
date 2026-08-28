# ServerWatch Pebble — Agent Instructions

## Project

ServerWatch Pebble is the Pebble client for ServerWatch, a watch-first home-server monitoring system.

Architecture:

Pebble app -> PebbleKit JS -> authenticated ServerWatch Agent -> server/services

The ServerWatch Agent is the source of truth. The Pebble client must not directly query qBittorrent, Jellyfin, Sonarr, Radarr, or other monitored services.

The Pebble application is a native Pebble application written primarily in C, with PebbleKit JS handling communication with the Agent.

Target platforms are Emery and Gabbro.

## Development Principles

- Preserve existing working behaviour unless a change explicitly requires otherwise.
- Diagnose the root cause before implementing a fix.
- Do not knowingly introduce technical debt as a shortcut.
- Prefer small, coherent changes over broad rewrites.
- Respect Pebble memory constraints and existing fixed-size-buffer design.
- Do not silently change established UX or connection semantics.
- Do not modify unrelated code while fixing a specific problem.
- Build after implementation changes and investigate all build warnings/errors introduced by the change.
- Explain the root cause and resulting change after completing work.

## ServerWatch Agent Contract

The Pebble client consumes the authenticated ServerWatch Agent API.

Authentication uses:

`X-ServerWatch-Api-Key`

Agent API changes must remain backward compatible with the existing ServerWatch Wear OS client.

Prefer additive API changes.

Do not change or remove existing Agent fields/contracts merely to make Pebble implementation easier.

Secrets and production credentials must never be committed to Git.

## Connection and Cached-Data Semantics

Connection state and server health are separate concepts.

Pebble connection states are:

- UNKNOWN
- CONNECTED
- FAILED

A successful Agent response must deliver CONNECTED atomically with the new snapshot and update the last-success timestamp.

A failed request must:

- send only the FAILED connection state
- preserve the last valid server snapshot
- preserve the last-success timestamp
- never replace cached values with dummy/failed values

Cold-start failure:

- show `Connection failed`
- do not display invented or dummy server data

Failure after at least one successful snapshot:

- preserve the last-good data
- indicate that cached data is being displayed
- show the age of the last successful update

Recovery must automatically return the UI to normal live state.

PebbleKit JS uses an explicit approximately 4-second request watchdog because relying solely on XHR timeout behaviour has previously been unreliable.

Background polling must never unexpectedly move the user's current page, selection, or viewport.

## Server Status Semantics

GREEN / Online:

- Agent reachable
- server reported online
- monitored services healthy

YELLOW / Warning:

- Agent reachable
- server online
- one or more monitored services unhealthy

RED / Connection lost:

- Pebble cannot communicate with the Agent

When disconnected, individual service states remain their last-known values rather than being forcibly changed to failed/red.

Retain Agent-reported `server_online` independently from Pebble connection state.

## UI Architecture

Top-level application areas are:

- Attention
- Server
- Storage
- Downloads

ServerWatch should feel native to Pebble rather than imitate the Wear OS application.

Use colour sparingly:

- green = healthy
- yellow = warning
- red = critical/disconnected
- white/default = normal information

### Attention

Attention is informational and horizontally paged.

Maximum displayed attention items: 8.

The existing healthy-state cat presentation is intentional and should not be removed or substantially redesigned without explicit instruction.

### Server

Server is an informational horizontally paged view.

Current structure:

- page 1: system metrics
- page 2: monitored services

### Storage

Storage is an informational horizontally paged view.

Current structure:

- page 1: total/free/used capacity
- page 2: category breakdown

### Downloads

Downloads intentionally uses a different interaction model.

It is a vertical, selectable, card-based view because downloads are interactive.

Do not convert Downloads to the horizontal pager used by Attention, Server, and Storage.

Download selection and viewport should remain stable during live updates.

The client supports up to three displayed active download cards, with overflow represented separately.

## Polling

Normal Agent polling interval is approximately 5 seconds.

Refresh also occurs on application startup and focus regain.

Live updates must not disturb navigation state.

If one screen appears stale while other screens remain connected, investigate the complete data path before assuming a network failure:

Agent payload -> PebbleKit JS -> AppMessage -> C model -> window update

## Destructive Actions

Existing ServerWatch Agent download deletion behaviour used by Wear OS must remain compatible.

For Pebble destructive actions:

- never perform deletion without explicit user interaction/confirmation
- destructive actions must be disabled when displaying cached/disconnected data
- use immutable torrent identity/hash rather than display names
- refresh immediately after a successful destructive operation

Do not execute real destructive operations as part of automated testing.

## Git and Release Safety

The agent may:

- inspect/search repository files
- edit local project files when explicitly tasked
- build
- run appropriate local tests
- inspect Git status/diffs/history

The agent must NOT perform any of the following without explicit user approval:

- push to a remote
- create or move Git tags
- create a release
- deploy to production
- modify production server configuration
- modify Tailscale/Funnel configuration
- expose or commit credentials/secrets
- perform destructive download operations

Do not automatically commit changes unless explicitly instructed.

## Current Release Baseline

The repository reached Pebble v1.0.0 at commit:

`2c8f7c4`

The v1.0.0 tag represents the clean baseline before post-release development.

Treat changes after this point as normal post-v1.0 development and preserve released behaviour unless deliberately changing it.