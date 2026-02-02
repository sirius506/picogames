#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "picogames.h"
#include "graphlib.h"
#include "LCDdriver.h"

uint pwm_slice_num;

void sound_init()
{
    // サウンド用PWM設定
    gpio_set_function(SOUND_PORT, GPIO_FUNC_PWM);
    pwm_slice_num = pwm_gpio_to_slice_num(SOUND_PORT);
    pwm_set_wrap(pwm_slice_num, PWM_WRAP-1);
    // duty 50%
    pwm_set_chan_level(pwm_slice_num, SOUND_CHAN, PWM_WRAP/2);
}

void sound_on(uint16_t f){
    pwm_set_clkdiv_int_frac(pwm_slice_num, f>>4, f&15);
    pwm_set_enabled(pwm_slice_num, true);
}

void sound_off(void){
    pwm_set_enabled(pwm_slice_num, false);
}

void lcd_port_init()
{
	// 液晶用ポート設定
    // Enable SPI 1 at 40 MHz and connect to GPIOs
    spi_init(SPICH, 40000 * 1000);
    gpio_set_function(LCD_MISO, GPIO_FUNC_SPI);
    gpio_set_function(LCD_SCK, GPIO_FUNC_SPI);
    gpio_set_function(LCD_MOSI, GPIO_FUNC_SPI);

    gpio_init(LCD_CS);
    gpio_put(LCD_CS, 1);
    gpio_set_dir(LCD_CS, GPIO_OUT);
    gpio_init(LCD_DC);
    gpio_put(LCD_DC, 1);
    gpio_set_dir(LCD_DC, GPIO_OUT);
    gpio_init(LCD_RESET);
    gpio_put(LCD_RESET, 1);
    gpio_set_dir(LCD_RESET, GPIO_OUT);
}

extern const unsigned char InvFontData[];

void board_init()
{
    set_font_data(InvFontData);
    sound_init();
    lcd_port_init();
    init_graphic(); //液晶利用開始
}
