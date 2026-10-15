/**
 * @file lv_demo_music_main.h
 *
 */

#ifndef APP_DEMO_MUSIC_MAIN_H
#define APP_DEMO_MUSIC_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lv_demo_music.h"

#if APP_DEMO_MUSIC_ENABLE

#if LV_USE_GRID == 0
#error "LV_USE_GRID needs to be enabled"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
lv_obj_t * app_music_main_create(lv_obj_t * parent);
void app_music_main_update_layout(lv_obj_t *parent);
void app_music_play(uint32_t id);
void app_music_resume(void);
void app_music_pause(void);
void app_music_album_next(bool next);

/**********************
 *      MACROS
 **********************/
#endif /*APP_DEMO_MUSIC_ENABLE*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*APP_DEMO_MUSIC_MAIN_H*/
