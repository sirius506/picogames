#ifndef PICOGAMES_H
#define PICOGAMES_H

#include "hwconfig.h"
#include "LCDdriver.h"
#include "graphlib.h"
#include "gamepad.h"

#define	KEYUP	 VBMASK_UP
#define	KEYDOWN	 VBMASK_DOWN
#define	KEYLEFT	 VBMASK_LEFT
#define	KEYRIGHT VBMASK_RIGHT
#define	KEYSTART VBMASK_SQUARE
#define KEYFIRE  VBMASK_CIRCLE

PADEVENT *get_pad_event();

#define clearscreen() LCD_Clear(0)

#define	PWM_WRAP (SYS_CLK_HZ/31250)

void board_init();
void sound_init();
int apds_init();
void sound_on(uint16_t f);
void sound_off(void);
void lcd_port_init();
uint32_t get_pad_vmask();
void wait60thsec(unsigned short n);
void set_font_data(const unsigned char *ptr);
int check_pad_connect();
uint8_t touch_read_irq();
void touch_cs(int val);
uint8_t touch_xchg_byte(uint8_t val);
void lcd_send_data(const uint8_t *cmd, int cmd_size, uint8_t *bp, int dlen);

/* Entry for each games */
void inv_main(void);
void hakomusu_main(void);
void pacman_main(void);
void tetris_main(void);
void peg_main(void);

#endif
