#ifndef  HWCONFIG_H
#define HWCONFIG_H

/* LCD SPI interface port */
#define	LCD_SCK	  10
#define	LCD_MOSI  11
#define	LCD_MISO  12
#define LCD_CS    13
#define LCD_DC    14
#define LCD_RESET 15
#define SPICH spi1

/* Touch SPI interface port */
#define	TOUCH_IRQ	2
#define	TOUCH_MOSI	3
#define	TOUCH_MISO	4
#define	TOUCH_CS	5
#define	TOUCH_SCK	6
#define	TOUCH_SPI	spi0

/* Gesture sensor I2C interface port */
#define	APDS_I2C	i2c0
#define	APDS_INT	18
#define	APDS_SCL	17
#define	APDS_SDA	16

/* PWM sound port */
#define SOUND_PORT 21
#define	SOUND_CHAN PWM_CHAN_B

#endif
