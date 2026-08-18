#include "connection_status_view.h"

#include <stdlib.h>

#include "../services/server_status_service.h"

struct ConnectionStatusView
{
    TextLayer *text_layer;
};

static void prv_refresh(
    ConnectionStatusView *view)
{
    if ((view == NULL) ||
        (view->text_layer == NULL))
    {
        return;
    }

    const ServerStatus *const status =
        server_status_service_get();

    if (status == NULL)
    {
        return;
    }

    if (status->connection_state ==
        CONNECTION_STATE_CONNECTED)
    {
        layer_set_hidden(
            text_layer_get_layer(
                view->text_layer),
            true);

        return;
    }

    if (status->connection_state ==
        CONNECTION_STATE_FAILED)
    {
        layer_set_hidden(
            text_layer_get_layer(
                view->text_layer),
            false);

        if (status->has_received_snapshot)
        {
            char age_text[24];

            server_status_service_format_update_age_value(
                age_text,
                sizeof(age_text));

            static char connection_text[64];

            snprintf(
                connection_text,
                sizeof(connection_text),
                "Connection lost\nCached - %s",
                age_text);

            text_layer_set_text(
                view->text_layer,
                connection_text);
        }
        else
        {
            text_layer_set_text(
                view->text_layer,
                "Connection failed");
        }

        return;
    }

    layer_set_hidden(
        text_layer_get_layer(
            view->text_layer),
        true);
}

ConnectionStatusView *connection_status_view_create(
    GRect frame)
{
    ConnectionStatusView *const view =
        calloc(
            1,
            sizeof(ConnectionStatusView));

    if (view == NULL)
    {
        return NULL;
    }

    view->text_layer =
        text_layer_create(frame);

    if (view->text_layer == NULL)
    {
        free(view);
        return NULL;
    }

    text_layer_set_font(
        view->text_layer,
        fonts_get_system_font(
            FONT_KEY_GOTHIC_14_BOLD));

    text_layer_set_text_alignment(
        view->text_layer,
        GTextAlignmentLeft);

    prv_refresh(view);

    return view;
}

Layer *connection_status_view_get_layer(
    ConnectionStatusView *view)
{
    if (view == NULL)
    {
        return NULL;
    }

    return text_layer_get_layer(
        view->text_layer);
}

void connection_status_view_refresh(
    ConnectionStatusView *view)
{
    prv_refresh(view);
}

void connection_status_view_destroy(
    ConnectionStatusView *view)
{
    if (view == NULL)
    {
        return;
    }

    if (view->text_layer != NULL)
    {
        text_layer_destroy(
            view->text_layer);

        view->text_layer = NULL;
    }

    free(view);
}