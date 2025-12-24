#ifndef LED_STRIP_H
#define LED_STRIP_H

#include <Arduino.h>

// LED Strip configuration
#define LED_STRIP_PIN       2       // Port A Yellow = G2
#define LED_STRIP_COUNT     120
#define LED_BRIGHTNESS      150      // 0-255

// LED animation modes
typedef enum {
    LED_MODE_RAINBOW = 0,
    LED_MODE_CANDY_CHASE,       // Red and white chase pattern
    LED_MODE_CANDY_CANE,        // 20 green, 20 red with white chase
    LED_MODE_COUNT              // Total number of modes
} led_mode_t;

void led_strip_init(void);
void led_strip_set_brightness(uint8_t brightness);
void led_strip_clear(void);
void led_strip_show(void);
void led_strip_set_pixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
void led_strip_start(void);
void led_strip_stop(void);
void led_strip_next_mode(void);
led_mode_t led_strip_get_mode(void);

#endif // LED_STRIP_H
