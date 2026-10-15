#include "lv_demo_music.h"
#include "lv_demo_music_main.h"
#include "lv_demo_music_list.h"
static lv_obj_t *root;
static app_music_backend_t backend;
static void root_delete_cb(lv_event_t *event)
{
    LV_UNUSED(event);
    root = NULL;
    backend = (app_music_backend_t){0};
}
static void root_size_cb(lv_event_t *event)
{
    app_music_main_update_layout(lv_event_get_target(event));
}
void app_music_create(lv_obj_t *parent, const app_music_backend_t *config)
{
    app_music_close();
    backend = *config;
    root = lv_obj_create(parent);
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, lv_pct(100), lv_pct(100));
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x343247), 0);
    lv_obj_add_event_cb(root, root_delete_cb, LV_EVENT_DELETE, NULL);
    lv_obj_update_layout(root);
    app_music_list_create(root);
    app_music_main_create(root);
    lv_obj_add_event_cb(root, root_size_cb, LV_EVENT_SIZE_CHANGED, NULL);
}
void app_music_close(void) { if(root) lv_obj_delete(root); }
const app_music_backend_t *app_music_get_backend(void) { return &backend; }
uint32_t app_music_get_track_count(void) { return backend.track_count; }
const char *app_music_get_title(uint32_t id)
{
    return id < backend.track_count ? backend.title(backend.context, id) : NULL;
}
const char *app_music_get_artist(uint32_t id) { LV_UNUSED(id); return "Local audio"; }
const char *app_music_get_genre(uint32_t id) { LV_UNUSED(id); return "MP3"; }
uint32_t app_music_get_track_length(uint32_t id) { LV_UNUSED(id); return 0; }
