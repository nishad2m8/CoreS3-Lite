#include "led_strip.h"
#include <FastLED.h>

static CRGB leds[LED_STRIP_COUNT];
static TaskHandle_t led_task_handle = NULL;
static volatile bool led_running = false;
static volatile led_mode_t current_mode = LED_MODE_RAINBOW;

// Rainbow animation
static void rainbow_animation(void) {
    static uint8_t hue = 0;
    for (int i = 0; i < LED_STRIP_COUNT; i++) {
        leds[i] = CHSV(hue + (i * 256 / LED_STRIP_COUNT), 255, LED_BRIGHTNESS);
    }
    FastLED.show();
    hue += 2;
}

// Candy chase - white, red, off pattern running
static void candy_chase_animation(void) {
    static int offset = 0;
    const int white_width = 5;
    const int red_width = 10;
    const int gap_width = 3;
    const int pattern_len = white_width + red_width + gap_width;

    for (int i = 0; i < LED_STRIP_COUNT; i++) {
        int pos = (i + offset) % pattern_len;
        if (pos < white_width) {
            leds[i] = CRGB(LED_BRIGHTNESS, LED_BRIGHTNESS, LED_BRIGHTNESS);  // White
        } else if (pos < white_width + red_width) {
            leds[i] = CRGB(LED_BRIGHTNESS, 0, 0);  // Red
        } else {
            leds[i] = CRGB::Black;  // Gap
        }
    }
    FastLED.show();
    offset++;
    if (offset >= pattern_len) offset = 0;
}

// Candy cane: 20 green, 20 red repeating with white LEDs (1 white, 20 gap)
static void candy_cane_animation(void) {
    static int offset = 0;
    static const int white_gap = 21;  // 1 white + 20 gap

    // Set base pattern: 20 green, 20 red, repeating
    for (int i = 0; i < LED_STRIP_COUNT; i++) {
        int segment = (i / 20) % 2;
        if (segment == 0) {
            leds[i] = CRGB(0, LED_BRIGHTNESS, 0);  // Green
        } else {
            leds[i] = CRGB(LED_BRIGHTNESS, 0, 0);  // Red
        }
    }

    // Draw white LEDs: 1 on, 20 off pattern
    for (int i = 0; i < LED_STRIP_COUNT; i++) {
        if ((i + offset) % white_gap == 0) {
            leds[i] = CRGB::White;
        }
    }

    FastLED.show();

    offset--;
    if (offset < 0) offset = white_gap - 1;
}

// Main LED animation task
static void led_task(void *pvParameters) {
    while (led_running) {
        switch (current_mode) {
            case LED_MODE_RAINBOW:
                rainbow_animation();
                vTaskDelay(pdMS_TO_TICKS(20));
                break;
            case LED_MODE_CANDY_CHASE:
                candy_chase_animation();
                vTaskDelay(pdMS_TO_TICKS(50));
                break;
            case LED_MODE_CANDY_CANE:
                candy_cane_animation();
                vTaskDelay(pdMS_TO_TICKS(80));
                break;
            default:
                vTaskDelay(pdMS_TO_TICKS(20));
                break;
        }
    }
    vTaskDelete(NULL);
}

void led_strip_init(void) {
    FastLED.addLeds<WS2812, LED_STRIP_PIN, GRB>(leds, LED_STRIP_COUNT);
    FastLED.setBrightness(LED_BRIGHTNESS);
    FastLED.clear();
    FastLED.show();
}

void led_strip_set_brightness(uint8_t brightness) {
    FastLED.setBrightness(brightness);
}

void led_strip_clear(void) {
    FastLED.clear();
    FastLED.show();
}

void led_strip_show(void) {
    FastLED.show();
}

void led_strip_set_pixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index < LED_STRIP_COUNT) {
        leds[index] = CRGB(r, g, b);
    }
}

void led_strip_start(void) {
    if (led_running) return;

    led_running = true;
    xTaskCreatePinnedToCore(led_task, "led_anim", 2048, NULL, 1, &led_task_handle, 0);
}

void led_strip_stop(void) {
    led_running = false;
    if (led_task_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(50));
        led_task_handle = NULL;
    }
    led_strip_clear();
}

void led_strip_next_mode(void) {
    current_mode = (led_mode_t)((current_mode + 1) % LED_MODE_COUNT);
    Serial.printf("LED mode: %d\n", current_mode);
}

led_mode_t led_strip_get_mode(void) {
    return current_mode;
}
