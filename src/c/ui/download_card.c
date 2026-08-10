#include "download_card.h"

#include <stdio.h>
#include <stdlib.h>

struct DownloadCard
{
    Layer *root_layer;
    Layer *progress_bar_layer;
    Layer *warning_layer;

    TextLayer *name_layer;
    TextLayer *progress_layer;
    TextLayer *speed_layer;
    TextLayer *eta_layer;
    TextLayer *size_layer;

    int progress_percent;

    char progress_text[16];
    char eta_display_text[32];
    char size_display_text[32];
};

static void prv_progress_bar_update(
    Layer *layer,
    GContext *context
)
{
    DownloadCard *const *card_ref =
        layer_get_data(layer);

    if ((card_ref == NULL) || (*card_ref == NULL)) {
        return;
    }

    DownloadCard *const card = *card_ref;

    const GRect bounds =
        layer_get_bounds(layer);

    graphics_context_set_stroke_color(
        context,
        GColorBlack
    );

    graphics_draw_rect(
        context,
        bounds
    );

    const int16_t inner_width =
        bounds.size.w - 2;

    const int16_t fill_width =
        (inner_width * card->progress_percent) / 100;

    if (fill_width > 0) {
        graphics_context_set_fill_color(
            context,
            GColorBlack
        );

        graphics_fill_rect(
            context,
            GRect(
                1,
                1,
                fill_width,
                bounds.size.h - 2
            ),
            0,
            GCornerNone
        );
    }
}

static void prv_warning_update(
    Layer *layer,
    GContext *context
)
{
    (void)layer;

    graphics_context_set_fill_color(
        context,
        GColorBlack
    );

    const GPathInfo path_info = {
        .num_points = 3,
        .points = (GPoint[]) {
            GPoint(4, 0),
            GPoint(0, 8),
            GPoint(8, 8)
        }
    };

    GPath *const path =
        gpath_create(&path_info);

    if (path == NULL) {
        return;
    }

    gpath_draw_filled(
        context,
        path
    );

    gpath_destroy(path);
}

DownloadCard *download_card_create(
    GRect frame,
    const char *name,
    int progress_percent,
    const char *speed_text,
    const char *eta_text,
    uint32_t size_mb,
    bool suspicious
)
{
    DownloadCard *const card =
        calloc(1, sizeof(DownloadCard));

    if (card == NULL) {
        return NULL;
    }

    card->root_layer =
        layer_create(frame);

    if (card->root_layer == NULL) {
        free(card);
        return NULL;
    }

    const int16_t progress_text_width = 38;
    const int16_t progress_gap = 6;

    const int16_t progress_bar_width =
        frame.size.w -
        progress_text_width -
        progress_gap;

    card->progress_bar_layer =
        layer_create_with_data(
            GRect(
                0,
                35,
                progress_bar_width,
                10
            ),
            sizeof(DownloadCard *)
        );

    if (card->progress_bar_layer != NULL) {
        DownloadCard **const card_ref =
            layer_get_data(
                card->progress_bar_layer
            );

        *card_ref = card;

        layer_set_update_proc(
            card->progress_bar_layer,
            prv_progress_bar_update
        );
    }

    card->warning_layer =
        layer_create(
            GRect(0, 107, 9, 9)
        );

    if (card->warning_layer != NULL) {
        layer_set_update_proc(
            card->warning_layer,
            prv_warning_update
        );

        layer_set_hidden(
            card->warning_layer,
            !suspicious
        );
    }

    card->name_layer =
        text_layer_create(
            GRect(0, 0, frame.size.w, 24)
        );

    card->progress_layer =
        text_layer_create(
            GRect(
                progress_bar_width + progress_gap,
                28,
                progress_text_width,
                24
            )
        );

    card->speed_layer =
        text_layer_create(
            GRect(0, 52, frame.size.w, 24)
        );

    card->eta_layer =
        text_layer_create(
            GRect(0, 76, frame.size.w, 24)
        );

    const int16_t size_text_x =
        suspicious ? 12 : 0;        

    card->size_layer =
        text_layer_create(
            GRect(
                size_text_x,
                100,
                frame.size.w - size_text_x,
                24
            )
        );

    if ((card->name_layer == NULL) ||
        (card->progress_layer == NULL) ||
        (card->speed_layer == NULL) ||
        (card->eta_layer == NULL) ||
        (card->size_layer == NULL) ||
        (card->warning_layer == NULL)) {

        download_card_destroy(card);
        return NULL;
    }

    card->progress_percent =
        progress_percent;

    snprintf(
        card->progress_text,
        sizeof(card->progress_text),
        "%d%%",
        card->progress_percent
    );

    snprintf(
        card->eta_display_text,
        sizeof(card->eta_display_text),
        "ETA: %s",
        eta_text
    );

    if (size_mb >= 1000U) {
        snprintf(
            card->size_display_text,
            sizeof(card->size_display_text),
            "Size %lu.%lu GB",
            (unsigned long)(size_mb / 1000U),
            (unsigned long)(
                (size_mb % 1000U) / 100U
            )
        );
    } else {
        snprintf(
            card->size_display_text,
            sizeof(card->size_display_text),
            "Size %lu MB",
            (unsigned long)size_mb
        );
    }

    text_layer_set_text(
        card->name_layer,
        name
    );

    text_layer_set_text(
        card->progress_layer,
        card->progress_text
    );

    text_layer_set_text(
        card->speed_layer,
        speed_text
    );

    text_layer_set_text(
        card->eta_layer,
        card->eta_display_text
    );

    text_layer_set_text(
        card->size_layer,
        card->size_display_text
    );

    text_layer_set_font(
        card->name_layer,
        fonts_get_system_font(
            FONT_KEY_GOTHIC_18_BOLD
        )
    );

    text_layer_set_font(
        card->progress_layer,
        fonts_get_system_font(
            FONT_KEY_GOTHIC_18
        )
    );

    text_layer_set_font(
        card->speed_layer,
        fonts_get_system_font(
            FONT_KEY_GOTHIC_18
        )
    );

    text_layer_set_font(
        card->eta_layer,
        fonts_get_system_font(
            FONT_KEY_GOTHIC_18
        )
    );

    text_layer_set_font(
        card->size_layer,
        fonts_get_system_font(
            FONT_KEY_GOTHIC_18
        )
    );

    text_layer_set_text_alignment(
        card->name_layer,
        GTextAlignmentLeft
    );

    text_layer_set_text_alignment(
        card->progress_layer,
        GTextAlignmentLeft
    );

    text_layer_set_text_alignment(
        card->speed_layer,
        GTextAlignmentLeft
    );

    text_layer_set_text_alignment(
        card->eta_layer,
        GTextAlignmentLeft
    );

    text_layer_set_text_alignment(
        card->size_layer,
        GTextAlignmentLeft
    );

    layer_add_child(
        card->root_layer,
        text_layer_get_layer(
            card->name_layer
        )
    );

    if (card->progress_bar_layer != NULL) {
        layer_add_child(
            card->root_layer,
            card->progress_bar_layer
        );
    }

    layer_add_child(
        card->root_layer,
        text_layer_get_layer(
            card->progress_layer
        )
    );

    layer_add_child(
        card->root_layer,
        text_layer_get_layer(
            card->speed_layer
        )
    );

    layer_add_child(
        card->root_layer,
        text_layer_get_layer(
            card->eta_layer
        )
    );

    layer_add_child(
        card->root_layer,
        card->warning_layer
    );

    layer_add_child(
        card->root_layer,
        text_layer_get_layer(
            card->size_layer
        )
    );

    return card;
}

Layer *download_card_get_layer(
    DownloadCard *card
)
{
    return card == NULL
        ? NULL
        : card->root_layer;
}

void download_card_set_progress(
    DownloadCard *card,
    int progress_percent
)
{
    if (card == NULL) {
        return;
    }

    card->progress_percent =
        progress_percent;

    snprintf(
        card->progress_text,
        sizeof(card->progress_text),
        "%d%%",
        card->progress_percent
    );

    text_layer_set_text(
        card->progress_layer,
        card->progress_text
    );

    if (card->progress_bar_layer != NULL) {
        layer_mark_dirty(
            card->progress_bar_layer
        );
    }
}

void download_card_destroy(
    DownloadCard *card
)
{
    if (card == NULL) {
        return;
    }

    if (card->size_layer != NULL) {
        text_layer_destroy(
            card->size_layer
        );
    }

    if (card->eta_layer != NULL) {
        text_layer_destroy(
            card->eta_layer
        );
    }

    if (card->speed_layer != NULL) {
        text_layer_destroy(
            card->speed_layer
        );
    }

    if (card->progress_layer != NULL) {
        text_layer_destroy(
            card->progress_layer
        );
    }

    if (card->progress_bar_layer != NULL) {
        layer_destroy(
            card->progress_bar_layer
        );
    }

    if (card->warning_layer != NULL) {
        layer_destroy(
            card->warning_layer
        );
    }

    if (card->name_layer != NULL) {
        text_layer_destroy(
            card->name_layer
        );
    }

    if (card->root_layer != NULL) {
        layer_destroy(
            card->root_layer
        );
    }

    free(card);
}