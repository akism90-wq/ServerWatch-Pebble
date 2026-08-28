#include "updated_age_view.h"

#include "../services/server_status_service.h"

struct UpdatedAgeView
{
    TextLayer *text_layer;
    AppTimer *timer;
    char text[48];
};

static void prv_refresh(
    UpdatedAgeView *view)
{
    if ((view == NULL) ||
        (view->text_layer == NULL))
    {
        return;
    }

    server_status_service_format_update_age(
        view->text,
        sizeof(view->text));

    text_layer_set_text(
        view->text_layer,
        view->text);

    layer_mark_dirty(
        text_layer_get_layer(
            view->text_layer));
}

static void prv_timer_callback(void *context)
{
    UpdatedAgeView *const view =
        (UpdatedAgeView *)context;

    if (view == NULL)
    {
        return;
    }

    prv_refresh(view);

    view->timer =
        app_timer_register(
            1000,
            prv_timer_callback,
            view);
}

UpdatedAgeView *updated_age_view_create(
    GRect frame)
{
    UpdatedAgeView *const view =
        malloc(sizeof(UpdatedAgeView));

    if (view == NULL)
    {
        return NULL;
    }

    view->text_layer = text_layer_create(frame);
    view->timer = NULL;
    view->text[0] = '\0';

    if (view->text_layer == NULL)
    {
        free(view);
        return NULL;
    }

    text_layer_set_font(
        view->text_layer,
        fonts_get_system_font(FONT_KEY_GOTHIC_14));

    text_layer_set_text_alignment(
        view->text_layer,
        GTextAlignmentLeft);

    prv_refresh(view);

    view->timer =
        app_timer_register(
            1000,
            prv_timer_callback,
            view);

    return view;
}

Layer *updated_age_view_get_layer(
    UpdatedAgeView *view)
{
    if (view == NULL)
    {
        return NULL;
    }

    return text_layer_get_layer(
        view->text_layer);
}

void updated_age_view_refresh(
    UpdatedAgeView *view)
{
    prv_refresh(view);
}

void updated_age_view_destroy(
    UpdatedAgeView *view)
{
    if (view == NULL)
    {
        return;
    }

    if (view->timer != NULL)
    {
        app_timer_cancel(view->timer);
        view->timer = NULL;
    }

    text_layer_destroy(view->text_layer);
    view->text_layer = NULL;

    free(view);
}
