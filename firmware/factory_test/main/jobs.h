#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Hardware resources that only one command / job may use at a time. */
typedef enum {
    RES_I2C = 0,
    RES_SD,
    RES_USB,
    RES_AUDIO,
    RES_LCD,
    RES_CAMERA,
    RES_MAX,        /* "no resource" */
} res_id_t;

/* One background iteration. Return false to finish the job. */
typedef bool (*job_iter_fn_t)(void *ctx);
typedef void (*job_cleanup_fn_t)(void *ctx);

typedef struct {
    const char *name;            /* shown by `jobs`; must outlive the job (e.g. lives in ctx) */
    job_iter_fn_t iter;
    job_cleanup_fn_t cleanup;    /* may be NULL */
    void *ctx;
    uint32_t secs;               /* 0 = run until killed */
    uint32_t stack_size;         /* 0 = 8192 */
    res_id_t res;                /* locked for the job's lifetime; RES_MAX = none */
} job_desc_t;

int job_start(const job_desc_t *desc);           /* returns id > 0, or -1 */
int job_find(const char *name_prefix);           /* id or -1 */
esp_err_t job_kill(int id);                      /* blocks until the task exits (10 s max) */
bool job_should_stop(void);                      /* for code running inside a job task */
bool job_is_background(void);                    /* true inside a job task */
void job_report(bool ok, uint64_t bytes);        /* feeds the 5 s statistics line */

bool res_lock(res_id_t res, const char *owner);  /* prints "<res> busy (held by X)" on failure */
void res_unlock(res_id_t res);
const char *res_owner(res_id_t res);             /* NULL when free */

/* argtable3 tables are static and shared between the REPL task and job tasks:
 * hold this lock around arg_parse() and copy the values out before releasing. */
void args_lock(void);
void args_unlock(void);

void register_job_commands(void);                /* watch / jobs / kill */

#ifdef __cplusplus
}
#endif
