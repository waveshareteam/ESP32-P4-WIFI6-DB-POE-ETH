/* SD card and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// This example uses SDMMC peripheral to communicate with SD card.

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_board_device.h"
#include "dev_fs_fat.h"

#define EXAMPLE_MAX_CHAR_SIZE 64
#define MOUNT_POINT "/sdcard"
static const char *TAG = "example";

static esp_err_t s_example_write_file(const char *path, char *data)
{
    ESP_LOGI(TAG, "Opening file %s", path);
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, data);
    fclose(f);
    ESP_LOGI(TAG, "File written");

    return ESP_OK;
}

static esp_err_t s_example_read_file(const char *path)
{
    ESP_LOGI(TAG, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[EXAMPLE_MAX_CHAR_SIZE];
    fgets(line, sizeof(line), f);
    fclose(f);

    // strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    return ESP_OK;
}

void app_main(void)
{
    esp_err_t ret;

    dev_fs_fat_config_t *board_config = NULL;
    ret = esp_board_device_get_config("fs_sdcard", (void **)&board_config);
    if (ret != ESP_OK || board_config == NULL) {
        ESP_LOGE(TAG, "Failed to get SD card configuration: %s", esp_err_to_name(ret));
        return;
    }
    dev_fs_fat_config_t card_config = *board_config;
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
    card_config.vfs_config.format_if_mount_failed = true;
#endif
    ret = esp_board_device_override_config("fs_sdcard", &card_config, sizeof(card_config));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure SD card: %s", esp_err_to_name(ret));
        return;
    }
    ret = esp_board_device_init("fs_sdcard");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        esp_board_device_restore_config("fs_sdcard");
        return;
    }
    dev_fs_fat_handle_t *card_handle = NULL;
    ret = esp_board_device_get_handle("fs_sdcard", (void **)&card_handle);
    if (ret != ESP_OK || card_handle == NULL || card_handle->card == NULL) {
        ESP_LOGE(TAG, "Failed to get SD card handle: %s", esp_err_to_name(ret));
        goto cleanup;
    }
    sdmmc_card_t *card = card_handle->card;
    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    // Use POSIX and C standard library functions to work with files:

    // First create a file.
    const char *file_hello = MOUNT_POINT"/hello.txt";
    char data[EXAMPLE_MAX_CHAR_SIZE];
    snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "Hello", card->cid.name);
    ret = s_example_write_file(file_hello, data);
    if (ret != ESP_OK) {
        goto cleanup;
    }

    const char *file_foo = MOUNT_POINT"/foo.txt";
    // Check if destination file exists before renaming
    struct stat st;
    if (stat(file_foo, &st) == 0) {
        // Delete it if it exists
        unlink(file_foo);
    }

    // Rename original file
    ESP_LOGI(TAG, "Renaming file %s to %s", file_hello, file_foo);
    if (rename(file_hello, file_foo) != 0) {
        ESP_LOGE(TAG, "Rename failed");
        goto cleanup;
    }

    ret = s_example_read_file(file_foo);
    if (ret != ESP_OK) {
        goto cleanup;
    }

    // Format FATFS
#ifdef CONFIG_EXAMPLE_FORMAT_SD_CARD
    ret = esp_vfs_fat_sdcard_format(MOUNT_POINT, card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format FATFS (%s)", esp_err_to_name(ret));
        goto cleanup;
    }

    if (stat(file_foo, &st) == 0) {
        ESP_LOGI(TAG, "file still exists");
        goto cleanup;
    } else {
        ESP_LOGI(TAG, "file doesn't exist, formatting done");
    }
#endif // CONFIG_EXAMPLE_FORMAT_SD_CARD

    const char *file_nihao = MOUNT_POINT"/nihao.txt";
    memset(data, 0, EXAMPLE_MAX_CHAR_SIZE);
    snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "Nihao", card->cid.name);
    ret = s_example_write_file(file_nihao, data);
    if (ret != ESP_OK) {
        goto cleanup;
    }

    //Open file for reading
    ret = s_example_read_file(file_nihao);
    if (ret != ESP_OK) {
        goto cleanup;
    }

cleanup:
    ret = esp_board_device_deinit("fs_sdcard");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
        return;
    }
    ESP_ERROR_CHECK(esp_board_device_restore_config("fs_sdcard"));
    ESP_LOGI(TAG, "Card unmounted");
}
