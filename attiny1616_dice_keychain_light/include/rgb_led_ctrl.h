/*
 * rgb_led_ctrl.h
 *
 * Created: 17.11.2023 16:52:05
 *  Author: Florian
 */ 
#include <inttypes.h>
#include <atmel_start_pins.h>

#ifndef RGB_LED_CTRL_H_
#define RGB_LED_CTRL_H_

#define MODE_DICE	 0
#define MODE_CUSTOM  1
#define MODE_BAT_LVL 2
#define MODE_TEST	 3

char get_dice_color(void);
void set_dice_color(char color);
void next_color(void);

uint8_t get_dice_number(void);
void set_dice_number(uint8_t number);

void dice_timer_routine(void);
void leds_off(void);
void clear_all_led_pins(void);
void set_single_rgb_led(uint8_t led_index, char rgb);

void set_max_dice_number(uint8_t number);
uint8_t get_max_dice_number(void);

uint8_t get_mode(void);
void set_mode(uint8_t mode);

void set_bat_lvl(uint8_t lvl);
uint8_t get_bat_lvl(void);
void set_bat_color(char rgb);
void toggle_bat_blink(void);
void set_bat_blink(uint8_t b);

#endif /* RGB_LED_CTRL_H_ */