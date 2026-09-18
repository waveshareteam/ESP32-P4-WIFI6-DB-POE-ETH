#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_console.h"
#include "esp_timer.h"
#include "jobs.h"

#define JOB_MAX       8
#define JOB_STAT_US   (5 * 1000 * 1000LL)
#define JOB_TLS_INDEX 0

typedef struct {
    bool used;
    int id;
    volatile bool stop;
    job_desc_t desc;
    int64_t start_us;
    int64_t window_us;
    uint64_t iterations;
    uint64_t errors;
    uint64_t bytes;
    uint64_t bytes_window;
} job_t;

static job_t s_jobs[JOB_MAX];
static int s_next_id = 1;
static SemaphoreHandle_t s_jobs_mutex;
static SemaphoreHandle_t s_args_mutex;
static SemaphoreHandle_t s_res_mutex;
static const char *s_res_owner[RES_MAX];
static const char *const s_res_name[RES_MAX] = { "i2c", "sd", "usb", "audio", "lcd", "camera" };

static void jobs_init_once(void)
{
    if (s_jobs_mutex == NULL) {
        s_jobs_mutex = xSemaphoreCreateMutex();
        s_args_mutex = xSemaphoreCreateMutex();
        s_res_mutex = xSemaphoreCreateMutex();
    }
}

static job_t *current_job(void)
{
    return (job_t *)pvTaskGetThreadLocalStoragePointer(NULL, JOB_TLS_INDEX);
}

bool job_is_background(void)
{
    return current_job() != NULL;
}

bool job_should_stop(void)
{
    job_t *j = current_job();
    return j ? j->stop : false;
}

void job_report(bool ok, uint64_t bytes)
{
    job_t *j = current_job();
    if (!j) {
        return;
    }
    if (!ok) {
        j->errors++;
    }
    j->bytes += bytes;
    j->bytes_window += bytes;
}

static double mbps(uint64_t bytes, int64_t us)
{
    return us > 0 ? (double)bytes / (us / 1e6) / (1024.0 * 1024.0) : 0.0;
}

static void job_task(void *arg)
{
    job_t *j = arg;
    vTaskSetThreadLocalStoragePointer(NULL, JOB_TLS_INDEX, j);
    j->start_us = j->window_us = esp_timer_get_time();
    printf("[%d] started: %s\n", j->id, j->desc.name);
    while (!j->stop) {
        int64_t now = esp_timer_get_time();
        if (j->desc.secs && now - j->start_us >= (int64_t)j->desc.secs * 1000000LL) {
            break;
        }
        if (!j->desc.iter(j->desc.ctx)) {
            break;
        }
        j->iterations++;
        now = esp_timer_get_time();
        if (now - j->window_us >= JOB_STAT_US) {
            printf("[%d] %s: iter=%llu err=%llu %.2f MB/s\n", j->id, j->desc.name,
                   (unsigned long long)j->iterations, (unsigned long long)j->errors,
                   mbps(j->bytes_window, now - j->window_us));
            j->bytes_window = 0;
            j->window_us = now;
        }
        vTaskDelay(1);
    }
    int64_t total_us = esp_timer_get_time() - j->start_us;
    printf("[%d] done: %s  iter=%llu err=%llu  %.1f s  avg %.2f MB/s\n", j->id, j->desc.name,
           (unsigned long long)j->iterations, (unsigned long long)j->errors,
           total_us / 1e6, mbps(j->bytes, total_us));
    if (j->desc.cleanup) {
        j->desc.cleanup(j->desc.ctx);
    }
    if (j->desc.res < RES_MAX) {
        res_unlock(j->desc.res);
    }
    xSemaphoreTake(s_jobs_mutex, portMAX_DELAY);
    j->used = false;
    xSemaphoreGive(s_jobs_mutex);
    vTaskDelete(NULL);
}

int job_start(const job_desc_t *desc)
{
    jobs_init_once();
    if (desc->res < RES_MAX && !res_lock(desc->res, desc->name)) {
        return -1;
    }
    xSemaphoreTake(s_jobs_mutex, portMAX_DELAY);
    job_t *j = NULL;
    for (int i = 0; i < JOB_MAX; i++) {
        if (!s_jobs[i].used) {
            j = &s_jobs[i];
            break;
        }
    }
    if (j) {
        memset(j, 0, sizeof(*j));
        j->used = true;
        j->id = s_next_id++;
        j->desc = *desc;
    }
    xSemaphoreGive(s_jobs_mutex);
    if (!j) {
        printf("no free job slot (max %d)\n", JOB_MAX);
        if (desc->res < RES_MAX) {
            res_unlock(desc->res);
        }
        return -1;
    }
    uint32_t stack = desc->stack_size ? desc->stack_size : 8192;
    if (xTaskCreate(job_task, desc->name, stack, j, 5, NULL) != pdPASS) {
        printf("failed to create job task\n");
        j->used = false;
        if (desc->res < RES_MAX) {
            res_unlock(desc->res);
        }
        return -1;
    }
    return j->id;
}

int job_find(const char *name_prefix)
{
    for (int i = 0; i < JOB_MAX; i++) {
        if (s_jobs[i].used && strncmp(s_jobs[i].desc.name, name_prefix, strlen(name_prefix)) == 0) {
            return s_jobs[i].id;
        }
    }
    return -1;
}

static job_t *job_by_id(int id)
{
    for (int i = 0; i < JOB_MAX; i++) {
        if (s_jobs[i].used && s_jobs[i].id == id) {
            return &s_jobs[i];
        }
    }
    return NULL;
}

esp_err_t job_kill(int id)
{
    job_t *j = job_by_id(id);
    if (!j) {
        printf("no such job: %d\n", id);
        return ESP_ERR_NOT_FOUND;
    }
    j->stop = true;
    for (int i = 0; i < 1000 && j->used && j->id == id; i++) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (j->used && j->id == id) {
        printf("job %d did not stop within 10 s\n", id);
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

bool res_lock(res_id_t res, const char *owner)
{
    jobs_init_once();
    xSemaphoreTake(s_res_mutex, portMAX_DELAY);
    const char *held = s_res_owner[res];
    if (held == NULL) {
        s_res_owner[res] = owner;
    }
    xSemaphoreGive(s_res_mutex);
    if (held != NULL) {
        printf("%s busy (held by %s)\n", s_res_name[res], held);
        return false;
    }
    return true;
}

void res_unlock(res_id_t res)
{
    xSemaphoreTake(s_res_mutex, portMAX_DELAY);
    s_res_owner[res] = NULL;
    xSemaphoreGive(s_res_mutex);
}

const char *res_owner(res_id_t res)
{
    return s_res_owner[res];
}

void args_lock(void)
{
    jobs_init_once();
    xSemaphoreTake(s_args_mutex, portMAX_DELAY);
}

void args_unlock(void)
{
    xSemaphoreGive(s_args_mutex);
}

/* ---- watch / jobs / kill ---- */

typedef struct {
    char cmdline[256];
} watch_ctx_t;

static bool watch_iter(void *arg)
{
    watch_ctx_t *w = arg;
    int ret = 0;
    esp_err_t err = esp_console_run(w->cmdline, &ret);
    job_report(err == ESP_OK && ret == 0, 0);
    return true;
}

static void watch_cleanup(void *arg)
{
    free(arg);
}

static int cmd_watch(int argc, char **argv)
{
    int i = 1;
    uint32_t secs = 0;
    if (argc > 2 && strcmp(argv[1], "--secs") == 0) {
        secs = (uint32_t)atoi(argv[2]);
        i = 3;
    }
    if (i >= argc) {
        printf("usage: watch [--secs N] <command> [args...]\n");
        return 1;
    }
    watch_ctx_t *w = calloc(1, sizeof(*w));
    if (!w) {
        return 1;
    }
    for (; i < argc; i++) {
        strlcat(w->cmdline, argv[i], sizeof(w->cmdline));
        if (i + 1 < argc) {
            strlcat(w->cmdline, " ", sizeof(w->cmdline));
        }
    }
    /* Run once in the foreground first: rejects bad arguments and busy resources
     * before a background task is created. */
    int ret = 0;
    esp_err_t err = esp_console_run(w->cmdline, &ret);
    if (err != ESP_OK || ret != 0) {
        printf("watch: first run failed, not starting\n");
        free(w);
        return 1;
    }
    job_desc_t d = {
        .name = w->cmdline,
        .iter = watch_iter,
        .cleanup = watch_cleanup,
        .ctx = w,
        .secs = secs,
        .stack_size = 12288,
        .res = RES_MAX,
    };
    if (job_start(&d) < 0) {
        free(w);
        return 1;
    }
    return 0;
}

static int cmd_jobs(int argc, char **argv)
{
    printf("%-4s %-8s %-10s %-8s %s\n", "ID", "TIME", "ITER", "ERR", "COMMAND");
    int64_t now = esp_timer_get_time();
    for (int i = 0; i < JOB_MAX; i++) {
        if (s_jobs[i].used) {
            printf("%-4d %-8.1f %-10llu %-8llu %s\n", s_jobs[i].id, (now - s_jobs[i].start_us) / 1e6,
                   (unsigned long long)s_jobs[i].iterations, (unsigned long long)s_jobs[i].errors,
                   s_jobs[i].desc.name);
        }
    }
    return 0;
}

static int cmd_kill(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: kill <id|all>\n");
        return 1;
    }
    if (strcmp(argv[1], "all") == 0) {
        for (int i = 0; i < JOB_MAX; i++) {
            if (s_jobs[i].used) {
                job_kill(s_jobs[i].id);
            }
        }
        return 0;
    }
    return job_kill(atoi(argv[1])) == ESP_OK ? 0 : 1;
}

void register_job_commands(void)
{
    jobs_init_once();
    const esp_console_cmd_t cmds[] = {
        { .command = "watch", .help = "Repeat a command in the background until killed. Usage: watch [--secs N] <cmd...>", .func = cmd_watch },
        { .command = "jobs",  .help = "List background jobs", .func = cmd_jobs },
        { .command = "kill",  .help = "Stop a background job. Usage: kill <id|all>", .func = cmd_kill },
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }
}
