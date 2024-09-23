#include <atmel_start.h>
#include <rgb_led_ctrl.h>
#include <avr/sleep.h>
#include <adc_basic.h>
#include <driver_init.h>
#include <compiler.h>
#include <avr/eeprom.h>
#include <lfsr_random.h>

#define DELAY_SLOWDOWN		47
#define DELAY_SLOWDOWN_A    67
#define DELAY_SLOWDOWN_B    5
#define DELAY_SHUFFLE		11
#define TIMEOUT_SHUFFLE		100
#define BTN_MIN_HOLD_TIME	10
#define MENU_WAIT_TIME		400
#define MENU_TIMEOUT		67
#define BAT_LEVEL_TIME      413
#define DELAY_BAT_BLINK     33
#define DELAY_MENU_BLINK    20
#define DELAY_MENU_BLINK_NEXT_COLOR 7

#define MENU_PRESSES_NEEDED 3
#define EXECS_TILL_NEW_SEED 10	// 5
#define BLINK_COUNT			2	// 3
#define EEPROM_ADDRESS		37
// #define ENABLE_MENU
// #define ENABLE_SHOW_BAT_LVL
// #define ENABLE_SHUFFLE_NUMBER_IS_RANDOM

void read_cell_voltage(void);
void prepare_sleep(void);
void goto_sleep(void);
void reset_state(void);
void enable_rng_adc_channels(void);
void disable_rng_adc_channels(void);
void generate_new_seed(void);
void start_init_after_sleep(void);
void adc_disable(void);
void adc_enable(void);
uint8_t get_random_dice_number();
void read_max_dice_number(void);
void save_max_dice_number(void);

volatile uint16_t	_executions			= 0;
volatile uint16_t   _t_hold_key			= 0;
volatile uint16_t   _t_menu_timeout		= 0;
volatile uint16_t   adc_result			= 0;
volatile uint8_t	blink_flag			= 0;
volatile uint8_t	blink_count			= 0;
volatile uint8_t	number				= 1;
volatile uint8_t	delay_slowdown		= DELAY_SLOWDOWN;
volatile uint8_t	current_dice_state	= 0;
volatile uint8_t	previous_dice_state	= 0;
volatile uint8_t	button_presses_menu	= 0;
volatile uint8_t	button_last_state   = 0;
volatile uint8_t    exit_menu      		= 0;
volatile uint8_t    display_bat_lvl		= 0;

// 10ms interrupt
ISR(TCA0_CMP0_vect)
{
	static uint16_t t_shuffle_number	= 0;
	static uint16_t t_slowdown_number	= 0;
	static uint16_t t_timeout			= 0;
	static uint16_t t_blink_menu		= 0;
	static uint16_t t_blink_menu_color	= 0;
	static uint16_t t_display_voltage	= 0;
	static uint16_t t_blink_voltage		= 0;
	static uint8_t  shuffle_number		= 1;
	
	switch (current_dice_state)
	{
		// CHECK IF BUTTON IS HELD LONG ENOUGH
		// WHEN BUTTON IS HELD LONG ENOUGH, GO TO STATE 2 (START SHUFFLE)
		case 1:
			// wait if button is held long enough
			if(BTN_get_level() == 1)
			{
				_t_hold_key++;
				if(_t_hold_key >= BTN_MIN_HOLD_TIME)
				{
					_t_hold_key = 0;
					current_dice_state = 2;
					start_init_after_sleep();
				}
			}
			else {
				_t_hold_key = 0;
			}
		break;
		
		// START SHUFFLEING NUMBERS (not random)
		case 2:
			t_slowdown_number = 0;

			if (++t_shuffle_number >= DELAY_SHUFFLE){
				t_shuffle_number = 0;
				next_color();
				set_dice_number(shuffle_number);
				#ifndef ENABLE_SHUFFLE_NUMBER_IS_RANDOM
					if (++shuffle_number > get_max_dice_number()){
						shuffle_number = 1;
					}
				#else
					shuffle_number = get_random_dice_number();
				#endif
			}

			if (++t_timeout > TIMEOUT_SHUFFLE){
				#ifdef ENABLE_SHOW_BAT_LVL
					display_bat_lvl = 1;
				#endif
				current_dice_state = 3;
			}
		break;
		
		// START SLOWING DOWN AND APPLY RANDOM NUMBER, THEN BLINK RESULT NUMBER
		case 3:
			t_timeout = 0;
			t_shuffle_number = 0;

			// set number once entering slowdown
			if (t_slowdown_number == 0)
			{
				number = get_random_dice_number();
			}

			// when button is released, start slowing down then blink the resulting number
			if (++t_slowdown_number >= delay_slowdown)
			{
				t_slowdown_number = 1;

				if (delay_slowdown <= (DELAY_SLOWDOWN_A-DELAY_SLOWDOWN))
				{
					if (++number > get_max_dice_number()){
						number = 1;
					}
					next_color();
					set_dice_number(number);
					delay_slowdown += DELAY_SLOWDOWN_B;
				}
				else
				{
					button_presses_menu = 0;
				
					if (++blink_count < (BLINK_COUNT * 2))
					{
						blink_flag ^= 1;
					}
					else
					{
						blink_flag = 1;
						if (display_bat_lvl == 1)
						{
							blink_flag = 0;
							previous_dice_state = 3;
							current_dice_state = 5;
							set_mode(MODE_BAT_LVL);
						}
						// button low level check necessary?
						else if (BTN_get_level() == 0 && display_bat_lvl == 0)
						{
							goto_sleep();
						}
					}
				}
			}
		break;
		
		// CHECK IF USER WANTS TO GO TO THE MENU
		case 4:
			if(exit_menu == 0)
			{
				if(++t_blink_menu > DELAY_MENU_BLINK) {
					blink_flag ^= 1;
					t_blink_menu = 0;
				}
				
				if(++t_blink_menu_color > DELAY_MENU_BLINK_NEXT_COLOR) {
					next_color();
					t_blink_menu_color = 0;
				}
				
				if(++_t_menu_timeout >= MENU_WAIT_TIME)
				{
					_t_menu_timeout = 0;
					exit_menu = 1;
				}
				} else {
				blink_flag = 0;
				set_dice_color('g');
				if(++_t_menu_timeout >= MENU_TIMEOUT)
				{
					_t_menu_timeout = 0;
					exit_menu = 0;
					save_max_dice_number();
					goto_sleep();
					set_dice_color('r');
				}
			}
		break;
		
		// DISPLAY CELL VOLTAGE LEVEL
		case 5:
			if (get_bat_lvl() % 2 == 1)
			{
				if(++t_blink_voltage > DELAY_BAT_BLINK) {
					toggle_bat_blink();
					t_blink_voltage = 0;
				}
			}
			
			if (++t_display_voltage > BAT_LEVEL_TIME)
			{
				t_display_voltage = 0;
				goto_sleep();
			}
		break;
		
	}

	/* The interrupt flag has to be cleared manually */
	TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm;
}

// 10us interrupt
ISR(TCB0_INT_vect)
{
	if (current_dice_state == 2 || current_dice_state == 3 || current_dice_state == 4 || current_dice_state == 5)
	{
		if (blink_flag == 0)
		{
			dice_timer_routine();
	}
	else
	{
		leds_off();
	}
}

	TCB0.INTFLAGS = TCB_CAPT_bm;
}

ISR(PORTA_PORT_vect)
{

	// be sure to wake up on rising edge, else go to sleep again
	if (current_dice_state == 0 && previous_dice_state == 0 && BTN_get_level() == 1)
	{
		if(button_last_state == 1){
			goto_sleep();
		} else {
			// after startup (or coming back from sleep)
			previous_dice_state = current_dice_state;
			current_dice_state = 1; // wait if button is held long enough
			TCA0.SINGLE.CTRLA |= (1 << TCA_SINGLE_ENABLE_bp);
		}
	}
	else if (current_dice_state == 1 && previous_dice_state == 0)
	{
		// when releasing button before min hold time is reached
		previous_dice_state = current_dice_state;
		current_dice_state = 4;
	}
	else if (current_dice_state == 2 && previous_dice_state == 0)
	{
		// when releasing button
		previous_dice_state = current_dice_state;
		current_dice_state = 3;
	}
	else if (current_dice_state == 3 && previous_dice_state == 2)
	{
		// pressing button before entering sleep mode (= reroll)
		reset_state();
		current_dice_state = 2;
		#ifdef ENABLE_MENU
			if(++button_presses_menu >= MENU_PRESSES_NEEDED){
				previous_dice_state = 3;
				current_dice_state = 4;
				button_presses_menu = 0;
				set_dice_number(get_max_dice_number());
			}
		#endif
	}
	else if (current_dice_state == 4 && previous_dice_state == 3)
	{
		#ifdef ENABLE_MENU
		if(BTN_get_level() == 1 && exit_menu == 0){
			_t_menu_timeout = 0; // reset timeout
			uint8_t nr = get_max_dice_number();
			if(++nr > 9){
				nr = 2;
			}
			set_dice_number(nr);
			set_max_dice_number(nr);
		}
		#endif
	}
	// prevent from going to sleep while number is blinking (happens after timeout when button is released)
	else if (current_dice_state != 3)
	{
		goto_sleep();
	}

	/* Clear interrupt flags */
	VPORTA_INTFLAGS = (1 << 0) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7);
}

int main(void)
{
	atmel_start_init();
	#ifdef ENABLE_MENU
		read_max_dice_number();
	#endif
	lfsr_seed(0xabcd);
	goto_sleep();
	while (1){}
}

void reset_state(void){
	set_mode(MODE_DICE);
	set_bat_blink(0);
	_t_hold_key = 0;
	current_dice_state = 0;
	previous_dice_state = 0;
	blink_flag = 0;
	blink_count = 0;
	delay_slowdown = DELAY_SLOWDOWN;
}

void goto_sleep(void){
	
	blink_flag = 1;
	button_presses_menu = 0;
	display_bat_lvl = 0;

	TCB0.CTRLA &= ~(1 << TCB_ENABLE_bp);
	TCA0.SINGLE.CTRLA &= ~(1 << TCA_SINGLE_ENABLE_bp);
	leds_off();
	reset_state();
	
	BTN_set_dir(PORT_DIR_OUT);
	BTN_set_level(false);

	if (++_executions >= EXECS_TILL_NEW_SEED)
	{
		BTN_set_isc(PORT_ISC_INPUT_DISABLE_gc);
		generate_new_seed();
		BTN_set_isc(PORT_ISC_BOTHEDGES_gc);
		_executions = 0;
	}

	adc_disable();

	PORTA.OUT = 0;
	PORTA.DIR = 0;
	PORTB.OUT = 0;
	PORTB.DIR = 0;
	PORTC.OUT = 0;
	PORTC.DIR = 0;
	for (uint8_t i = 0; i < 8; i++) {
		*((uint8_t *)&PORTA + 0x10 + i) = 0b00001100;
		*((uint8_t *)&PORTB + 0x10 + i) = 0b00001100;
		*((uint8_t *)&PORTC + 0x10 + i) = 0b00001100;
	}
		
	BTN_set_dir(PORT_DIR_IN);
	BTN_set_pull_mode(PORT_PULL_OFF);
	BTN_set_isc(PORT_ISC_BOTHEDGES_gc);
	
	button_last_state = BTN_get_level();

	sleep_cpu();
}

void generate_new_seed(void){
	adc_enable();
	enable_rng_adc_channels();
	uint16_t _seed = adc_result - number;
	for (uint8_t i = 0; i<2; i++)
	{
		_seed ^= (((ADC_0_get_conversion(9) & 0x07) << 3) + (ADC_0_get_conversion(3) & 0x07));
		_seed = (_seed << 6) + ((ADC_0_get_conversion(3) & 0x07) << 3) + ((ADC_0_get_conversion(9) & 0x07));
		_seed ^= TCA0.SINGLE.CNT;
	}
	disable_rng_adc_channels();
	adc_disable();
	lfsr_seed(_seed);
}

void read_cell_voltage(void){
	adc_enable();

	ADC_EN_set_level(false);
	ADC_EN_set_dir(PORT_DIR_OUT);

	// After the conversion is complete (RESRDY is '1'), the conversion result RES is available in the ADC Result Register (ADCn.RES). The result for a 10-bit conversion is given as:
	// RES = (1023 x VIN) / VREF, VREF = 1.5V
	// Multiply by 2 because of 50/50 voltage divider
	
	// throw away first conversion
	adc_result = ADC_0_get_conversion(1);
	adc_result = 0;
	
	for (uint8_t i = 0; i<5; i++)
	{
		adc_result += ADC_0_get_conversion(1);
	}
	adc_result /= 5;
	
	// see excel sheet for calculations
	if(adc_result > 1020) {
		set_bat_lvl(6);
	} else if (adc_result > 955){
		set_bat_lvl(5);
	} else if (adc_result > 921) {
		set_bat_lvl(4);
	} else if (adc_result > 887) {
		set_bat_lvl(3);
	} else if (adc_result > 818) {
		set_bat_lvl(2);
	} else {
		set_bat_lvl(1);
	}
	
	if(get_bat_lvl() < 3) {
		set_bat_color('r');
	} else {
		set_bat_color('g');
	}

	ADC_EN_set_level(false);
	ADC_EN_set_dir(PORT_DIR_IN);
	ADC_EN_set_pull_mode(PORT_PULL_UP);
	ADC_EN_set_isc(PORT_ISC_INPUT_DISABLE_gc);

	adc_disable();
}

uint8_t get_random_dice_number()
{
	return (lfsr_random() + TCA0.SINGLE.CNT) % get_max_dice_number() + 1;
}

void read_max_dice_number()
{
	uint8_t nr = eeprom_read_byte((uint8_t *)EEPROM_ADDRESS);
	if(nr >= 1 && nr <= 9)
	set_max_dice_number(nr);
}

void save_max_dice_number()
{
	eeprom_update_byte((uint8_t *)EEPROM_ADDRESS, get_max_dice_number());
}

void start_init_after_sleep(void){

	read_cell_voltage();

	/* PORT setting on PB5 */
	LED1_EN_set_level(false);
	LED1_EN_set_dir(PORT_DIR_OUT);

	/* PORT setting on PA5 */
	LED2_EN_set_level(false);
	LED2_EN_set_dir(PORT_DIR_OUT);

	/* PORT setting on PA6 */
	LED3_EN_set_level(false);
	LED3_EN_set_dir(PORT_DIR_OUT);

	/* PORT setting on PA7 */
	LED4_EN_set_level(false);
	LED4_EN_set_dir(PORT_DIR_OUT);

	/* PORT setting on PC0 */
	LED5_EN_set_level(false);
	LED5_EN_set_dir(PORT_DIR_OUT);

	/* PORT setting on PC1 */
	LED6_EN_set_level(false);
	LED6_EN_set_dir(PORT_DIR_OUT);

	/* PORT setting on PC2 */
	LED7_EN_set_level(false);
	LED7_EN_set_dir(PORT_DIR_OUT);

	/* PORT setting on PB0 */
	R_EN_set_level(true);
	R_EN_set_dir(PORT_DIR_OUT);

	/* PORT setting on PB1 */
	G_EN_set_level(true);
	G_EN_set_dir(PORT_DIR_OUT);

	/* PORT setting on PB2 */
	B_EN_set_level(true);
	B_EN_set_dir(PORT_DIR_OUT);

	TCB0.CTRLA |= (1 << TCB_ENABLE_bp);
}

void adc_enable()
{
	VREF_0_init();
	ADC_0_enable();
	ADC_set_isc(PORT_ISC_INPUT_DISABLE_gc);
	ADC_set_pull_mode(PORT_PULL_OFF);
}

void adc_disable()
{
	VREF_CTRLB = 0;
	ADC_0_disable();
	ADC_set_level(false);
	ADC_set_dir(PORT_DIR_IN);
	ADC_set_pull_mode(PORT_PULL_UP);
	ADC_set_isc(PORT_ISC_INPUT_DISABLE_gc);
}

void enable_rng_adc_channels(void){
	/* PORT setting on TP4 */
	TP4_set_level(false);
	TP4_set_dir(PORT_DIR_IN);
	TP4_set_pull_mode(PORT_PULL_OFF);
	TP4_set_isc(PORT_ISC_INTDISABLE_gc);

	/* PORT setting on PA0 */
	PA3_set_level(false);
	PA3_set_dir(PORT_DIR_IN);
	PA3_set_pull_mode(PORT_PULL_OFF);
	PA3_set_isc(PORT_ISC_INTDISABLE_gc);
}

void disable_rng_adc_channels(void){
	/* PORT setting on TP4 */
	TP4_set_level(false);
	TP4_set_dir(PORT_DIR_IN);
	TP4_set_pull_mode(PORT_PULL_UP);
	TP4_set_isc(PORT_ISC_INPUT_DISABLE_gc);

	/* PORT setting on PA0 */
	PA3_set_level(false);
	PA3_set_dir(PORT_DIR_IN);
	PA3_set_pull_mode(PORT_PULL_UP);
	PA3_set_isc(PORT_ISC_INPUT_DISABLE_gc);
}
