#include "audio_player.h"
#include "M5Unified.h"
#include "m5gfx_lvgl.h"
#include <SD.h>
#include <SPI.h>
#include <AudioFileSourcePROGMEM.h>
#include <AudioFileSourceBuffer.h>
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutput.h>

static uint8_t current_volume = AUDIO_VOLUME;
static bool sd_initialized = false;

// MP3 data loaded into PSRAM
static uint8_t *mp3_data = nullptr;
static size_t mp3_size = 0;

// MP3 playback state
static AudioGeneratorMP3 *mp3 = nullptr;
static AudioFileSourcePROGMEM *file = nullptr;
static AudioFileSourceSD *sdfile = nullptr;
static AudioFileSourceBuffer *buff = nullptr;
static bool mp3_loop_enabled = false;  // play once, do not auto-restart

// Streaming into M5.Speaker using triple buffering (per M5Unified example)
#define STREAM_CHANNEL 0

// Custom output for M5.Speaker with smooth buffering
class AudioOutputM5Speaker : public AudioOutput {
public:
    AudioOutputM5Speaker() {
        hertz = 44100;
        bps = 16;
        channels = 2;
    }

    bool begin() override {
        return true;
    }

    bool ConsumeSample(int16_t sample[2]) override {
        // Downmix to mono to reduce bandwidth and CPU
        int32_t mono = ((int32_t)sample[0] + (int32_t)sample[1]) / 2;
        if (_tri_buffer_index < tri_buf_size) {
            _tri_buffer[_tri_index][_tri_buffer_index++] = (int16_t)mono;
            return true;
        }
        flush();
        return false;
    }

    bool stop() override {
        flush();
        M5.Speaker.stop(STREAM_CHANNEL);
        return true;
    }

    void flush() override {
        if (_tri_buffer_index) {
            M5.Speaker.playRaw(
                _tri_buffer[_tri_index],
                _tri_buffer_index,
                hertz,
                false,
                1,
                STREAM_CHANNEL
            );
            _tri_index = _tri_index < 2 ? _tri_index + 1 : 0;
            _tri_buffer_index = 0;
        }
    }

    bool SetRate(int hz) override {
        hertz = hz;
        return true;
    }
private:
    static constexpr size_t tri_buf_size = 1536; // mono samples (~35ms @44.1kHz)
    int16_t _tri_buffer[3][tri_buf_size];
    size_t _tri_buffer_index = 0;
    size_t _tri_index = 0;
};

static AudioOutputM5Speaker *out = nullptr;

void audio_player_init(void) {
    // Reconfigure speaker for smoother streaming (larger DMA, higher priority)
    auto scfg = M5.Speaker.config();
    scfg.sample_rate   = 44100;
    scfg.dma_buf_len   = 512;  // larger DMA chunk reduces refill pressure
    scfg.dma_buf_count = 8;    // keep internal queue deep enough
    scfg.task_priority = 4;    // prioritize audio mixing vs. UI
    scfg.i2s_port      = I2S_NUM_0; // keep audio on I2S0; display uses separate peripheral
#if defined(CONFIG_FREERTOS_UNICORE) && (CONFIG_FREERTOS_UNICORE == 0)
    scfg.task_pinned_core = 1; // keep audio on APP CPU when dual-core
#endif
    M5.Speaker.end();
    M5.Speaker.config(scfg);
    M5.Speaker.begin();

    M5.Speaker.setVolume(current_volume);
    M5.Speaker.setChannelVolume(STREAM_CHANNEL, current_volume);
}

bool audio_player_init_sd(void) {
    if (sd_initialized) return true;
    sd_initialized = SD.begin(SD_CS_PIN, SPI, 25000000);
    Serial.printf("SD init: %s\n", sd_initialized ? "OK" : "FAILED");
    return sd_initialized;
}

// Keep SD mounted to avoid SPI bus re-inits that can disturb the display
void audio_player_deinit_sd(void) {
    // no-op: keep sd_initialized and bus active
}

void audio_player_set_volume(uint8_t volume) {
    current_volume = volume;
    M5.Speaker.setVolume(volume);
    M5.Speaker.setChannelVolume(0, volume);
}

void audio_player_play_tone(uint16_t freq, uint32_t duration_ms) {
    M5.Speaker.tone(freq, duration_ms);
}

void audio_player_play_wav(const uint8_t *wav_data, size_t size) {
    M5.Speaker.playWav(wav_data, size, 1, 1, false);
}

bool audio_player_play_wav_file(const char *filepath) {
    audio_player_stop();
    if (!audio_player_init_sd()) return false;

    if (!SD.exists(filepath)) {
        return false;
    }

    File f = SD.open(filepath, FILE_READ);
    if (!f) {
        return false;
    }

    size_t wav_size = f.size();
    uint8_t *wav_buffer = (uint8_t *)ps_malloc(wav_size);
    if (!wav_buffer) wav_buffer = (uint8_t *)malloc(wav_size);
    if (!wav_buffer) {
        f.close();
        audio_player_deinit_sd();
        return false;
    }

    f.read(wav_buffer, wav_size);
    f.close();
    // keep SD mounted

    M5.Speaker.playWav(wav_buffer, wav_size, 1, STREAM_CHANNEL, false);
    return true;
}

bool audio_player_play_mp3(const char *filepath) {
    audio_player_stop();

    if (!audio_player_init_sd()) return false;
    if (!SD.exists(filepath)) {
        Serial.printf("MP3 not found: %s\n", filepath);
        return false;
    }

    sdfile = new AudioFileSourceSD(filepath);
    if (!sdfile) return false;
    // Larger read-ahead buffer smooths SD I/O hiccups
    buff = new AudioFileSourceBuffer(sdfile, 16384);
    if (!buff) { delete sdfile; sdfile = nullptr; return false; }
    out = new AudioOutputM5Speaker();
    if (!out) { delete buff; buff = nullptr; delete sdfile; sdfile = nullptr; return false; }
    mp3 = new AudioGeneratorMP3();
    if (!mp3) { delete out; out = nullptr; delete buff; buff = nullptr; delete sdfile; sdfile = nullptr; return false; }

    if (!mp3->begin(buff, out)) {
        delete mp3; mp3 = nullptr;
        delete out; out = nullptr;
        delete buff; buff = nullptr;
        delete sdfile; sdfile = nullptr;
        return false;
    }

    Serial.println("Playing MP3 from SD...");
    return true;
}

void audio_player_mp3_loop(void) {
    if (!mp3) return;
    if (!mp3->isRunning()) return;

    // Aggressively decode while the speaker has room in its queue
    int loops = 0;
    while (M5.Speaker.isPlaying(STREAM_CHANNEL) < 2 && loops < 12) {
        if (!mp3->loop()) {
            Serial.println("MP3 finished");
            audio_player_stop();
            return;
        }
        loops++;
    }
}

bool audio_player_is_playing(void) {
    return (mp3 != nullptr && mp3->isRunning()) || M5.Speaker.isPlaying();
}

void audio_player_stop(void) {
    if (mp3) {
        if (mp3->isRunning()) mp3->stop();
        if (out) out->stop();
        delete mp3; mp3 = nullptr;
    }
    // Clean up allocated audio objects
    if (out)  { delete out;  out  = nullptr; }
    if (buff) { delete buff; buff = nullptr; }
    if (file) { delete file; file = nullptr; }
    if (sdfile) { delete sdfile; sdfile = nullptr; }

    // Free cached MP3 data to release PSRAM
    if (mp3_data) {
        free(mp3_data);
        mp3_data = nullptr;
        mp3_size = 0;
    }
    M5.Speaker.stop(STREAM_CHANNEL);
    M5.Speaker.setChannelVolume(STREAM_CHANNEL, current_volume);
}
