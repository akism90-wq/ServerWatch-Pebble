#pragma once

#include <pebble.h>

typedef struct DownloadCard DownloadCard;

DownloadCard *download_card_create(
    GRect frame,
    const char *name,
    const char *subtitle,
    const char *state,
    int progress_percent,
    const char *speed_text,
    const char *eta_text,
    uint32_t size_mb,
    bool suspicious
);

Layer *download_card_get_layer(DownloadCard *card);

void download_card_set_progress(
    DownloadCard *card,
    int progress_percent
);

void download_card_set_selected(
    DownloadCard *card,
    bool selected
);

void download_card_update(
    DownloadCard *card,
    const char *name,
    const char *subtitle,
    const char *state,
    int progress_percent,
    const char *speed_text,
    const char *eta_text,
    uint32_t size_mb,
    bool suspicious
);

void download_card_destroy(DownloadCard *card);

