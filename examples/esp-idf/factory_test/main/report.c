#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "report.h"

#define REPORT_MAX 48

typedef struct {
    char item[32];
    char value[24];
    char note[48];
    report_result_t result;
    bool used;
} report_entry_t;

static report_entry_t s_entries[REPORT_MAX];
static SemaphoreHandle_t s_mutex;
static const char *const s_result_str[] = { "PASS", "FAIL", "SKIP", "MANUAL" };

static void lock(void)
{
    if (!s_mutex) {
        s_mutex = xSemaphoreCreateMutex();
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
}

static void unlock(void)
{
    xSemaphoreGive(s_mutex);
}

void report_set(const char *item, report_result_t result, const char *value, const char *note)
{
    lock();
    report_entry_t *e = NULL;
    for (int i = 0; i < REPORT_MAX; i++) {
        if (s_entries[i].used && strcmp(s_entries[i].item, item) == 0) {
            e = &s_entries[i];
            break;
        }
    }
    if (!e) {
        for (int i = 0; i < REPORT_MAX; i++) {
            if (!s_entries[i].used) {
                e = &s_entries[i];
                e->used = true;
                strlcpy(e->item, item, sizeof(e->item));
                break;
            }
        }
    }
    if (e) {
        e->result = result;
        strlcpy(e->value, value ? value : "", sizeof(e->value));
        strlcpy(e->note, note ? note : "", sizeof(e->note));
    }
    unlock();
}

void report_print(void)
{
    lock();
    int counts[4] = {0};
    printf("%-30s %-7s %-22s %s\n", "ITEM", "RESULT", "VALUE", "NOTE");
    for (int i = 0; i < REPORT_MAX; i++) {
        if (s_entries[i].used) {
            counts[s_entries[i].result]++;
            printf("%-30s %-7s %-22s %s\n", s_entries[i].item, s_result_str[s_entries[i].result],
                   s_entries[i].value, s_entries[i].note);
        }
    }
    printf("pass=%d fail=%d skip=%d manual=%d\n", counts[0], counts[1], counts[2], counts[3]);
    unlock();
}

void report_clear(void)
{
    lock();
    memset(s_entries, 0, sizeof(s_entries));
    unlock();
}
