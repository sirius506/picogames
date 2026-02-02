#ifndef GAMEBOARD_H
#define GAMEBOARD_H

#include "gamepad.h"

#define	KEYUP	 VBMASK_UP
#define	KEYDOWN	 VBMASK_DOWN
#define	KEYLEFT	 VBMASK_LEFT
#define	KEYRIGHT VBMASK_RIGHT
#define	KEYSTART VBMASK_PS
#define KEYFIRE  VBMASK_CIRCLE

PADEVENT *get_pad_event();

#define SOUND_PORT 21
#define	SOUND_CHAN PWM_CHAN_B

#define clearscreen() LCD_Clear(0)

#define PWM_WRAP 4000 // 125MHz/31.25KHz

void board_init();
void sound_init();
void sound_on(uint16_t f);
void sound_off(void);
void lcd_port_init();
uint16_t get_pad_press();
uint32_t get_pad_vmask();
void wait60thsec(unsigned short n);
void set_font_data(const unsigned char *ptr);

#endif
