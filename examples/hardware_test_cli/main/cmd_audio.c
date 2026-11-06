#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include "esp_board_device.h"
#include "esp_check.h"
#include "esp_codec_dev.h"
#include "esp_audio_dec_default.h"
#include "esp_audio_simple_dec.h"
#include "esp_audio_simple_dec_default.h"
#include "esp_heap_caps.h"
#include "esp_console.h"
#include "dev_audio_codec.h"
#include "cmd_audio.h"
#include "jobs.h"
#include "report.h"

static const char *TAG = "audio";
#define TONE_RATE        48000
#define FRAMES_PER_CHUNK 480    /* 10 ms at 48 kHz */
#define REC_RATE         16000
#define REC_CHUNK_FRAMES 320    /* 20 ms at 16 kHz */

/* amixer state: applied on codec_open and immediately to an open codec */
static int s_out_vol = 70;                 /* 0..100 % */
static float s_in_gain_db = 30.0f;         /* microphone gain, dB */
static esp_codec_dev_handle_t s_dac_open;  /* currently open DAC, or NULL */
static esp_codec_dev_handle_t s_adc_open;  /* currently open ADC, or NULL */

static esp_err_t codec_open(const char *dev_name, bool is_dac, uint32_t rate, uint8_t channels,
                            esp_codec_dev_handle_t *out)
{
    ESP_RETURN_ON_ERROR(esp_board_device_init(dev_name), TAG, "%s", dev_name);
    dev_audio_codec_handles_t *h = NULL;
    ESP_RETURN_ON_ERROR(esp_board_device_get_handle(dev_name, (void **)&h), TAG, "handle");
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = rate,
        .channel = channels,
        .bits_per_sample = 16,
    };
    esp_codec_dev_close(h->codec_dev);
    if (esp_codec_dev_open(h->codec_dev, &fs) != ESP_CODEC_DEV_OK) {
        printf("%s: codec open failed\n", dev_name);
        esp_board_device_deinit(dev_name);
        return ESP_FAIL;
    }
    if (is_dac) {
        esp_codec_dev_set_out_vol(h->codec_dev, s_out_vol);
        s_dac_open = h->codec_dev;
    } else {
        esp_codec_dev_set_in_gain(h->codec_dev, s_in_gain_db);
        s_adc_open = h->codec_dev;
    }
    *out = h->codec_dev;
    return ESP_OK;
}

static void codec_close(const char *dev_name, esp_codec_dev_handle_t dev)
{
    if (dev) {
        if (dev == s_dac_open) {
            s_dac_open = NULL;
        }
        if (dev == s_adc_open) {
            s_adc_open = NULL;
        }
        esp_codec_dev_close(dev);
        esp_board_device_deinit(dev_name);
    }
}

/* amixer | amixer set Master <0-100> | amixer set Capture <dB> */
static int cmd_amixer(int argc, char **argv)
{
    if (argc == 1) {
        printf("Master  (playback volume): %d%%\n", s_out_vol);
        printf("Capture (mic gain):        %.1f dB\n", s_in_gain_db);
        return 0;
    }
    if (argc == 4 && strcmp(argv[1], "set") == 0) {
        if (strcasecmp(argv[2], "Master") == 0) {
            int vol = atoi(argv[3]);
            if (vol < 0 || vol > 100) {
                printf("Master must be 0..100\n");
                return 1;
            }
            s_out_vol = vol;
            if (s_dac_open) {
                esp_codec_dev_set_out_vol(s_dac_open, s_out_vol);
            }
            printf("Master: %d%%\n", s_out_vol);
            return 0;
        }
        if (strcasecmp(argv[2], "Capture") == 0) {
            float db = strtof(argv[3], NULL);
            if (db < 0.0f || db > 60.0f) {
                printf("Capture must be 0..60 dB\n");
                return 1;
            }
            s_in_gain_db = db;
            if (s_adc_open) {
                esp_codec_dev_set_in_gain(s_adc_open, s_in_gain_db);
            }
            printf("Capture: %.1f dB\n", s_in_gain_db);
            return 0;
        }
    }
    printf("usage: amixer | amixer set Master <0-100> | amixer set Capture <0-60 dB>\n");
    return 1;
}

/* ---- speaker-test [-f HZ]: sine on both channels until killed ---- */
typedef struct {
    esp_codec_dev_handle_t dac;
    int16_t buf[FRAMES_PER_CHUNK * 2];
    float phase;
    float step;
} tone_ctx_t;

static bool tone_iter(void *arg)
{
    tone_ctx_t *t = arg;
    for (int i = 0; i < FRAMES_PER_CHUNK; i++) {
        int16_t s = (int16_t)(sinf(t->phase) * 12000.0f);
        t->buf[2 * i] = s;
        t->buf[2 * i + 1] = s;
        t->phase += t->step;
        if (t->phase > 2.0f * (float)M_PI) {
            t->phase -= 2.0f * (float)M_PI;
        }
    }
    int err = esp_codec_dev_write(t->dac, t->buf, sizeof(t->buf));
    job_report(err == ESP_CODEC_DEV_OK, sizeof(t->buf));
    return err == ESP_CODEC_DEV_OK;
}

static void tone_cleanup(void *arg)
{
    tone_ctx_t *t = arg;
    codec_close("audio_dac", t->dac);
    free(t);
}

static int cmd_speaker_test(int argc, char **argv)
{
    int freq = 1000;
    for (int i = 1; i + 1 < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            freq = atoi(argv[i + 1]);
        }
    }
    if (freq < 20 || freq > 20000) {
        printf("usage: speaker-test [-f 20..20000]\n");
        return 1;
    }
    if (res_owner(RES_AUDIO)) {
        printf("audio busy (held by %s)\n", res_owner(RES_AUDIO));
        return 1;
    }
    tone_ctx_t *t = calloc(1, sizeof(*t));
    if (!t) {
        return 1;
    }
    if (codec_open("audio_dac", true, TONE_RATE, 2, &t->dac) != ESP_OK) {
        free(t);
        return 1;
    }
    t->step = 2.0f * (float)M_PI * freq / TONE_RATE;
    job_desc_t d = {
        .name = "speaker-test",
        .iter = tone_iter,
        .cleanup = tone_cleanup,
        .ctx = t,
        .stack_size = 6144,
        .res = RES_AUDIO,
    };
    if (job_start(&d) < 0) {
        tone_cleanup(t);
        return 1;
    }
    printf("%d Hz sine playing; stop with kill <id>. Probe GPIO13 MCLK / GPIO12 SCLK / GPIO10 WS\n", freq);
    report_set("audio.play", REPORT_MANUAL, NULL, "listen for tone");
    return 0;
}

/* ---- WAV helpers (16-bit PCM, canonical 44-byte header) ---- */
typedef struct __attribute__((packed)) {
    char riff[4];
    uint32_t size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_len;
    uint16_t format;
    uint16_t channels;
    uint32_t rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits;
    char data[4];
    uint32_t data_len;
} wav_header_t;

static void wav_write_header(FILE *f, uint32_t rate, uint16_t ch, uint32_t data_len)
{
    wav_header_t h = {
        .size = 36 + data_len,
        .fmt_len = 16,
        .format = 1,
        .channels = ch,
        .rate = rate,
        .byte_rate = rate * ch * 2,
        .block_align = ch * 2,
        .bits = 16,
        .data_len = data_len,
    };
    memcpy(h.riff, "RIFF", 4);
    memcpy(h.wave, "WAVE", 4);
    memcpy(h.fmt, "fmt ", 4);
    memcpy(h.data, "data", 4);
    fseek(f, 0, SEEK_SET);
    fwrite(&h, sizeof(h), 1, f);
}

/* arecord -d <secs> <file>  (16 kHz mono) */
static int cmd_arecord(int argc, char **argv)
{
    int secs = 5;
    const char *path = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            secs = atoi(argv[++i]);
        } else {
            path = argv[i];
        }
    }
    if (!path || secs < 1 || secs > 60) {
        printf("usage: arecord -d <1..60> /sdcard/rec.wav\n");
        return 1;
    }
    if (!res_lock(RES_AUDIO, "arecord")) {
        return 1;
    }
    esp_codec_dev_handle_t adc = NULL;
    int ret = 1;
    FILE *f = fopen(path, "wb");
    if (!f) {
        printf("cannot create %s\n", path);
        goto out;
    }
    if (codec_open("audio_adc", false, REC_RATE, 1, &adc) != ESP_OK) {
        goto out;
    }
    wav_write_header(f, REC_RATE, 1, 0);
    int16_t chunk[REC_CHUNK_FRAMES];
    uint32_t total = 0;
    printf("recording %d s...\n", secs);
    for (int n = 0; n < secs * (REC_RATE / REC_CHUNK_FRAMES); n++) {
        if (esp_codec_dev_read(adc, chunk, sizeof(chunk)) != ESP_CODEC_DEV_OK) {
            printf("read error\n");
            goto out;
        }
        if (fwrite(chunk, 1, sizeof(chunk), f) != sizeof(chunk)) {
            printf("write error\n");
            goto out;
        }
        total += sizeof(chunk);
    }
    wav_write_header(f, REC_RATE, 1, total);
    printf("recorded %u bytes to %s\n", (unsigned)total, path);
    report_set("audio.record", REPORT_MANUAL, NULL, "play back with aplay");
    ret = 0;
out:
    if (f) {
        fclose(f);
    }
    codec_close("audio_adc", adc);
    res_unlock(RES_AUDIO);
    return ret;
}

/* aplay <file>: .wav .mp3 .aac .m4a .flac via esp_audio_codec's simple decoder */
static esp_audio_simple_dec_type_t dec_type_for(const char *path)
{
    const char *ext = strrchr(path, '.');
    if (!ext) {
        return 0;
    }
    ext++;
    if (strcasecmp(ext, "wav") == 0)  { return ESP_AUDIO_SIMPLE_DEC_TYPE_WAV; }
    if (strcasecmp(ext, "mp3") == 0)  { return ESP_AUDIO_SIMPLE_DEC_TYPE_MP3; }
    if (strcasecmp(ext, "aac") == 0)  { return ESP_AUDIO_SIMPLE_DEC_TYPE_AAC; }
    if (strcasecmp(ext, "m4a") == 0)  { return ESP_AUDIO_SIMPLE_DEC_TYPE_M4A; }
    if (strcasecmp(ext, "flac") == 0) { return ESP_AUDIO_SIMPLE_DEC_TYPE_FLAC; }
    return 0;
}

static void decoders_register_once(void)
{
    static bool done;
    if (!done) {
        esp_audio_dec_register_default();
        esp_audio_simple_dec_register_default();
        done = true;
    }
}

#define APLAY_IN_CHUNK  (4 * 1024)
#define APLAY_OUT_INIT  (16 * 1024)

static int cmd_aplay(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: aplay /sdcard/<file>.wav|.mp3|.aac|.m4a|.flac\n");
        return 1;
    }
    esp_audio_simple_dec_type_t type = dec_type_for(argv[1]);
    if (type == 0) {
        printf("unsupported extension (wav/mp3/aac/m4a/flac)\n");
        return 1;
    }
    if (!res_lock(RES_AUDIO, "aplay")) {
        return 1;
    }
    int ret = 1;
    esp_codec_dev_handle_t dac = NULL;
    esp_audio_simple_dec_handle_t dec = NULL;
    uint8_t *in_buf = NULL;
    uint8_t *out_buf = NULL;
    uint32_t out_size = APLAY_OUT_INIT;
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        printf("cannot open %s\n", argv[1]);
        goto out;
    }
    decoders_register_once();
    esp_audio_simple_dec_cfg_t dec_cfg = { .dec_type = type };
    if (esp_audio_simple_dec_open(&dec_cfg, &dec) != ESP_AUDIO_ERR_OK) {
        printf("decoder open failed (decoder for this format not enabled in menuconfig?)\n");
        goto out;
    }
    in_buf = heap_caps_malloc(APLAY_IN_CHUNK, MALLOC_CAP_SPIRAM);
    out_buf = heap_caps_malloc(out_size, MALLOC_CAP_SPIRAM);
    if (!in_buf || !out_buf) {
        printf("no memory\n");
        goto out;
    }
    uint64_t total_pcm = 0;
    bool eof = false;
    while (!eof) {
        size_t n = fread(in_buf, 1, APLAY_IN_CHUNK, f);
        if (n == 0) {
            break;
        }
        eof = n < APLAY_IN_CHUNK;
        esp_audio_simple_dec_raw_t raw = { .buffer = in_buf, .len = n, .eos = eof };
        /* One input chunk may hold several frames: loop until it is consumed. */
        while (raw.len) {
            esp_audio_simple_dec_out_t frame = { .buffer = out_buf, .len = out_size };
            esp_audio_err_t err = esp_audio_simple_dec_process(dec, &raw, &frame);
            if (err == ESP_AUDIO_ERR_BUFF_NOT_ENOUGH) {
                uint8_t *bigger = heap_caps_realloc(out_buf, frame.needed_size, MALLOC_CAP_SPIRAM);
                if (!bigger) {
                    printf("no memory for %u-byte frame\n", (unsigned)frame.needed_size);
                    goto out;
                }
                out_buf = bigger;
                out_size = frame.needed_size;
                continue;
            }
            if (err != ESP_AUDIO_ERR_OK) {
                printf("decode error %d after %llu PCM bytes\n", (int)err, (unsigned long long)total_pcm);
                goto out;
            }
            if (frame.decoded_size) {
                if (!dac) {
                    esp_audio_simple_dec_info_t info = {0};
                    esp_audio_simple_dec_get_info(dec, &info);
                    if (info.bits_per_sample != 16 || info.channel < 1 || info.channel > 2) {
                        printf("unsupported PCM format: %u bit %u ch\n", info.bits_per_sample, info.channel);
                        goto out;
                    }
                    if (codec_open("audio_dac", true, info.sample_rate, info.channel, &dac) != ESP_OK) {
                        goto out;
                    }
                    printf("playing %s: %u Hz, %u ch, 16 bit\n", argv[1], (unsigned)info.sample_rate, info.channel);
                }
                if (esp_codec_dev_write(dac, frame.buffer, frame.decoded_size) != ESP_CODEC_DEV_OK) {
                    printf("codec write error\n");
                    goto out;
                }
                total_pcm += frame.decoded_size;
            }
            raw.buffer += raw.consumed;
            raw.len -= raw.consumed;
        }
    }
    if (!dac) {
        printf("no audio decoded (wrong format?)\n");
        goto out;
    }
    printf("done, %llu PCM bytes\n", (unsigned long long)total_pcm);
    ret = 0;
out:
    if (dec) {
        esp_audio_simple_dec_close(dec);
    }
    free(in_buf);
    free(out_buf);
    if (f) {
        fclose(f);
    }
    codec_close("audio_dac", dac);
    res_unlock(RES_AUDIO);
    return ret;
}

/* ---- alsaloop: mic -> speaker until killed (16 kHz mono) ---- */
typedef struct {
    esp_codec_dev_handle_t adc;
    esp_codec_dev_handle_t dac;
    int16_t buf[REC_CHUNK_FRAMES];
} loop_ctx_t;

static bool loop_iter(void *arg)
{
    loop_ctx_t *l = arg;
    if (esp_codec_dev_read(l->adc, l->buf, sizeof(l->buf)) != ESP_CODEC_DEV_OK) {
        job_report(false, 0);
        return false;
    }
    int err = esp_codec_dev_write(l->dac, l->buf, sizeof(l->buf));
    job_report(err == ESP_CODEC_DEV_OK, sizeof(l->buf));
    return err == ESP_CODEC_DEV_OK;
}

static void loop_cleanup(void *arg)
{
    loop_ctx_t *l = arg;
    codec_close("audio_adc", l->adc);
    codec_close("audio_dac", l->dac);
    free(l);
}

static int cmd_alsaloop(int argc, char **argv)
{
    if (res_owner(RES_AUDIO)) {
        printf("audio busy (held by %s)\n", res_owner(RES_AUDIO));
        return 1;
    }
    loop_ctx_t *l = calloc(1, sizeof(*l));
    if (!l) {
        return 1;
    }
    if (codec_open("audio_dac", true, REC_RATE, 1, &l->dac) != ESP_OK ||
        codec_open("audio_adc", false, REC_RATE, 1, &l->adc) != ESP_OK) {
        loop_cleanup(l);
        return 1;
    }
    job_desc_t d = {
        .name = "alsaloop",
        .iter = loop_iter,
        .cleanup = loop_cleanup,
        .ctx = l,
        .stack_size = 6144,
        .res = RES_AUDIO,
    };
    if (job_start(&d) < 0) {
        loop_cleanup(l);
        return 1;
    }
    printf("mic -> speaker loopback; stop with kill <id>\n");
    report_set("audio.loopback", REPORT_MANUAL, NULL, "speak into mic");
    return 0;
}

void register_audio_commands(void)
{
    const esp_console_cmd_t cmds[] = {
        { .command = "speaker-test", .help = "Play a sine tone until killed. Usage: speaker-test [-f 1000]", .func = cmd_speaker_test },
        { .command = "arecord",      .help = "Record 16 kHz mono WAV. Usage: arecord -d 5 /sdcard/rec.wav", .func = cmd_arecord },
        { .command = "aplay",        .help = "Play wav/mp3/aac/m4a/flac. Usage: aplay /sdcard/song.mp3", .func = cmd_aplay },
        { .command = "alsaloop",     .help = "Microphone to speaker loopback until killed", .func = cmd_alsaloop },
        { .command = "amixer",       .help = "Volume / mic gain. amixer | amixer set Master 80 | amixer set Capture 30", .func = cmd_amixer },
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }
}
