#include <Arduino.h>
#include "M5GFX.h"
#include "M5Unified.h"
#include "lvgl.h"
#include "m5gfx_lvgl.h"
#include "ui.h"
#include "screens.h"
#include "led_strip.h"
#include "audio_player.h"

// Callback for volume slider
static void slider_volume_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);
    audio_player_set_volume((uint8_t)value);
    Serial.printf("Volume: %d\n", value);
}

// Callback for mode button
static void button_mode_cb(lv_event_t *e) {
    led_strip_next_mode();
}

void setup() {
    auto cfg = M5.config();
    cfg.internal_spk = true;  // Enable internal speaker
    M5.begin(cfg);

    Serial.begin(115200);
    Serial.println("CoreS3 Starting...");

    M5.Display.setBrightness(100);

    // Initialize LVGL and UI FIRST
    lv_init();
    m5gfx_lvgl_init();
    ui_init();

    // Add event callbacks for UI elements
    lv_obj_add_event_cb(objects.slider_volume, slider_volume_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.button_mode, button_mode_cb, LV_EVENT_CLICKED, NULL);

    // Initialize LED strip
    led_strip_init();
    led_strip_start();

    // Initialize audio player
    audio_player_init();

    // Load and play MP3 (SD will be released after loading)
    Serial.println("Loading MP3 from SD...");
    if (!audio_player_play_mp3("/xmas.mp3")) {
        Serial.println("Failed to play MP3!");
    }
}

void loop() {
    // Pump audio decoder frequently to keep speaker queue filled
    audio_player_mp3_loop();
    // Handle LVGL timers and rendering
    lv_timer_handler();
    // Short delay prevents watchdog while minimizing audio gaps
    delay(1);
}
