#ifndef APP_MUSIC_H
#define APP_MUSIC_H
#include "lvgl.h"
/* Keep the app independent of LVGL's built-in demos. */
#define APP_DEMO_MUSIC_ENABLE 1
#define APP_DEMO_MUSIC_LARGE 1
#define APP_DEMO_MUSIC_SQUARE 0
#define APP_DEMO_MUSIC_ROUND 0
#define APP_DEMO_MUSIC_HANDLE_SIZE (APP_DEMO_MUSIC_LARGE ? 40 : 20)
#define APP_MUSIC_MAX_TRACKS 5
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    APP_MUSIC_STOPPED, APP_MUSIC_PLAYING, APP_MUSIC_PAUSED,
    APP_MUSIC_FINISHED, APP_MUSIC_ERROR,
} app_music_state_t;
/* Callbacks run on the LVGL thread. Backend threads never touch widgets. */
typedef struct {
    void *context;
    uint32_t track_count;
    const char *(*title)(void *context, uint32_t id);
    bool (*play)(void *context, uint32_t id);
    bool (*pause)(void *context);
    bool (*resume)(void *context, uint32_t id);
    app_music_state_t (*status)(void *context, uint32_t *position, uint32_t *duration);
} app_music_backend_t;
void app_music_create(lv_obj_t *parent, const app_music_backend_t *backend);
void app_music_close(void);
const app_music_backend_t *app_music_get_backend(void);
uint32_t app_music_get_track_count(void);
const char *app_music_get_title(uint32_t id);
const char *app_music_get_artist(uint32_t id);
const char *app_music_get_genre(uint32_t id);
uint32_t app_music_get_track_length(uint32_t id);
#ifdef __cplusplus
}
#endif
#endif
