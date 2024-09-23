/*
 * rgb_led_ctrl.c
 *
 * Created: 17.11.2023 16:51:53
 *  Author: Florian
 */ 
#include <rgb_led_ctrl.h>

static uint8_t mode = MODE_DICE;
static uint8_t max_dice_number = 6;
static uint8_t dice_number = 1;
static char dice_color = 'r';

static uint8_t bat_lvl = 1;
static uint8_t bat_blink = 0;
static char bat_color = 'g';

static uint8_t test_matrix[7] = { // [LED_INDEX]
	1, 2, 3, 4, 5, 6, 7
};

static uint8_t dice_matrix[6][6] = { // [NUMBER][LED_INDEX]
	{4, 0, 0, 0, 0, 0},
	{1, 7, 0, 0, 0, 0},
	{3, 4, 6, 0, 0, 0},
	{1, 3, 6, 7, 0, 0},
	{1, 3, 6, 7, 4, 0},
	{1, 2, 3, 5, 6, 7}
};

static uint8_t bat_lvl_matrix[7][6] = { // [NUMBER][LED_INDEX]
	{0, 0, 0, 0, 0, 0},
	{6, 7, 0, 0, 0, 0},
	{6, 7, 0, 0, 0, 0},
	{2, 6, 5, 7, 0, 0},
	{2, 6, 5, 7, 0, 0},
	{1, 2, 6, 3, 5, 7},
	{1, 2, 6, 3, 5, 7}
};

void toggle_bat_blink(void)
{
	bat_blink ^= 1;
}

void set_bat_blink(uint8_t b)
{
	bat_blink = b;
}

void set_bat_lvl(uint8_t lvl)
{
	bat_lvl = lvl;
}

uint8_t get_bat_lvl(void)
{
	return bat_lvl;
}

void set_bat_color(char rgb)
{
	bat_color = rgb;
}

uint8_t get_mode(void)
{
	return mode;
}

void set_mode(uint8_t _mode)
{
	mode = _mode;
}

void set_max_dice_number(uint8_t number){
	max_dice_number = number;
}

uint8_t get_max_dice_number(void){
	return max_dice_number;
}

uint8_t get_dice_number(void){
	return dice_number;
}

void set_dice_number(uint8_t number){
	dice_number = number;
}

char get_dice_color(void){
	return dice_color;
}

void set_dice_color(char color){
	dice_color = color;
}

void dice_timer_routine(void){
	static uint8_t index = 0;
	
	switch(mode){
		case MODE_DICE:
			set_single_rgb_led(dice_matrix[dice_number - 1][index], dice_color);
			if (++index > (max_dice_number - 1))
			index = 0;
			break;
			
		case MODE_BAT_LVL:
			set_single_rgb_led(bat_lvl_matrix[bat_lvl - bat_blink][index], bat_color);
			if (++index > 5)
			index = 0;
			break;
			
		case MODE_TEST:
			set_single_rgb_led(test_matrix[index], dice_color);
			if (++index > 6)
			index = 0;
			break;
		
		case MODE_CUSTOM:
			break;
			
	}

}

void clear_all_led_pins(void){
	PORTB.OUTCLR = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN5_bm;
	PORTC.OUTCLR = PIN1_bm | PIN2_bm | PIN3_bm;
	PORTA.OUTCLR = PIN5_bm | PIN6_bm;
}

void leds_off(void){
	PORTB.OUTSET = PIN1_bm | PIN2_bm | PIN0_bm;
}

void next_color(void){
	
	if (bat_lvl < 3 && mode != MODE_TEST)
	{
		dice_color = 'r';
	} else {
		switch(dice_color){
			case 'r':
			dice_color = 'g';
			break;
			case 'g':
			dice_color = 'b';
			break;
			case 'b':
			dice_color = 'r';
			break;
		}
	}
}

void set_single_rgb_led(uint8_t led_index, char rgb){
	switch (rgb)
	{
		case 'r':
		PORTB.OUTSET = PIN1_bm | PIN2_bm;
		PORTB.OUTCLR = PIN0_bm;
		break;
		
		case 'g':
		PORTB.OUTSET = PIN0_bm | PIN2_bm;
		PORTB.OUTCLR = PIN1_bm;
		break;
		
		case 'b':
		PORTB.OUTSET = PIN0_bm | PIN1_bm;
		PORTB.OUTCLR = PIN2_bm;
		break;
	}
	
	PORTB.OUTCLR = PIN3_bm | PIN5_bm;
	PORTC.OUTCLR = PIN1_bm | PIN2_bm | PIN3_bm;
	PORTA.OUTCLR = PIN5_bm | PIN6_bm;
	
	switch (led_index)
	{
		case 1:
		LED1_EN_set_level(1);
		break;
		
		case 2:
		LED2_EN_set_level(1);
		break;
		
		case 3:
		LED3_EN_set_level(1);
		break;
		
		case 4:
		LED4_EN_set_level(1);
		break;
		
		case 5:
		LED5_EN_set_level(1);
		break;
		
		case 6:
		LED6_EN_set_level(1);
		break;
		
		case 7:
		LED7_EN_set_level(1);
		break;
	}
}