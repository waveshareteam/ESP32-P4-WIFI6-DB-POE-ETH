#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    REPORT_PASS = 0,
    REPORT_FAIL,
    REPORT_SKIP,
    REPORT_MANUAL,
} report_result_t;

/* Record (or overwrite) the result of a test item, e.g.
 * report_set("sd.read.40M", REPORT_PASS, "21.3 MB/s", NULL). value/note may be NULL. */
void report_set(const char *item, report_result_t result, const char *value, const char *note);
void report_print(void);
void report_clear(void);

#ifdef __cplusplus
}
#endif
