#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "esp_console.h"
#include "cmd_file.h"

#define CAT_MAX_BYTES (64 * 1024)

/* cat <path> [<path>...] */
static int cmd_cat(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: cat <path>\n");
        return 1;
    }
    int ret = 0;
    for (int i = 1; i < argc; i++) {
        FILE *f = fopen(argv[i], "rb");
        if (!f) {
            printf("cat: %s: %s\n", argv[i], strerror(errno));
            ret = 1;
            continue;
        }
        char buf[256];
        size_t total = 0;
        size_t n;
        char last = '\n';
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0 && total < CAT_MAX_BYTES) {
            fwrite(buf, 1, n, stdout);
            total += n;
            last = buf[n - 1];
        }
        fclose(f);
        if (last != '\n') {
            printf("\n");
        }
        if (total >= CAT_MAX_BYTES) {
            printf("[cat: output truncated at %u bytes]\n", (unsigned)CAT_MAX_BYTES);
        }
    }
    return ret;
}

/* echo <text...> [> <path> | >> <path>] */
static int cmd_echo(int argc, char **argv)
{
    const char *path = NULL;
    const char *mode = NULL;
    int last = argc;
    if (argc >= 3 && (strcmp(argv[argc - 2], ">") == 0 || strcmp(argv[argc - 2], ">>") == 0)) {
        path = argv[argc - 1];
        mode = strcmp(argv[argc - 2], ">") == 0 ? "wb" : "ab";
        last = argc - 2;
    }
    char text[256] = {0};
    for (int i = 1; i < last; i++) {
        strlcat(text, argv[i], sizeof(text));
        if (i + 1 < last) {
            strlcat(text, " ", sizeof(text));
        }
    }
    strlcat(text, "\n", sizeof(text));
    if (!path) {
        fputs(text, stdout);
        return 0;
    }
    FILE *f = fopen(path, mode);
    if (!f) {
        printf("echo: %s: %s\n", path, strerror(errno));
        return 1;
    }
    size_t len = strlen(text);
    bool ok = fwrite(text, 1, len, f) == len;
    int saved = errno;
    if (fclose(f) != 0) {
        ok = false;
        saved = errno;
    }
    if (!ok) {
        printf("echo: %s: %s\n", path, strerror(saved));
        return 1;
    }
    return 0;
}

void register_file_commands(void)
{
    const esp_console_cmd_t cmds[] = {
        { .command = "cat",  .help = "Print a file. Usage: cat /gpio20/value | cat /sdcard/x.txt", .func = cmd_cat },
        { .command = "echo", .help = "Print text or write it to a file. Usage: echo 1 > /gpio20/value", .func = cmd_echo },
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }
}
