#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h>

// Audio configuration
#define AUDIO_VOLUME        100  // 0-255

// SD Card pin for CoreS3
#define SD_CS_PIN           GPIO_NUM_4

void audio_player_init(void);
bool audio_player_init_sd(void);
void audio_player_set_volume(uint8_t volume);
void audio_player_play_tone(uint16_t freq, uint32_t duration_ms);
void audio_player_play_wav(const uint8_t *wav_data, size_t wav_size);
bool audio_player_play_wav_file(const char *filepath);
bool audio_player_play_mp3(const char *filepath);  // Note: converts to WAV lookup
void audio_player_mp3_loop(void);
bool audio_player_is_playing(void);
void audio_player_stop(void);

#endif // AUDIO_PLAYER_H
