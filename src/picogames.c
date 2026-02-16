/*
 * Pico Games
 *
 * Board dependent routines.
 *
 */
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "picogames.h"

#define	Z_THRESHOLD	400
#define	Z_THRESHOLD_INT	75
#define	MSEC_THRESHOLD	3

#define	USE_DMA

uint pwm_slice_num;

int16_t xraw, yraw, zraw;
int32_t msraw;

static uint spi_dma;
static dma_channel_config  dma_config;

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

static volatile int isr_flag;

void touch_irq_callback(uint gpio, uint32_t events)
{
  printf("Got interrupt.\n");
  isr_flag = 1;
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

#ifdef USE_DMA
    spi_dma = dma_claim_unused_channel(true);
    dma_config = dma_channel_get_default_config(spi_dma);
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_8);
    channel_config_set_dreq(&dma_config, spi_get_dreq(SPICH, true));
#endif

    /* Setup touch port */
    spi_init(TOUCH_SPI, 10000 * 1000);
    gpio_set_function(TOUCH_MISO, GPIO_FUNC_SPI);
    gpio_set_function(TOUCH_SCK, GPIO_FUNC_SPI);
    gpio_set_function(TOUCH_MOSI, GPIO_FUNC_SPI);
    gpio_init(TOUCH_CS);
    gpio_put(TOUCH_CS, 1);
    gpio_set_dir(TOUCH_CS, GPIO_OUT);

    gpio_init(TOUCH_IRQ);
    gpio_set_dir(TOUCH_IRQ, GPIO_IN);
    gpio_pull_up(TOUCH_IRQ);
  
}

void lcd_send_data(const uint8_t *cmd, int cmd_size, uint8_t *bp, int dlen)
{
    lcd_dc_lo();
    lcd_cs_lo();
    if (cmd_size > 0)
	spi_write_blocking(SPICH, cmd, cmd_size);
    lcd_dc_hi();
    if (dlen > 0)
    {
      dma_channel_configure(spi_dma, &dma_config,
           &spi_get_hw(SPICH)->dr,
           bp,
           dlen,
           true);
      dma_channel_wait_for_finish_blocking(spi_dma);
    }
    lcd_cs_hi();
}

uint8_t touch_read_irq()
{
  return (uint8_t) gpio_get(TOUCH_IRQ);
}

void touch_cs(int val)
{
  asm volatile("nop \n nop \n nop");
  gpio_put(TOUCH_CS, val);
  asm volatile("nop \n nop \n nop");
}

uint8_t touch_xchg_byte(uint8_t val)
{
  uint8_t indata;
  uint8_t outdata = val;

  spi_write_read_blocking(TOUCH_SPI, &outdata, &indata, 1);
  return indata;
}

extern const unsigned char InvFontData[];

void board_init()
{
//sleep_ms(3000);
    set_font_data(InvFontData);
    sound_init();
    lcd_port_init();
    init_graphic(); //液晶利用開始
}
