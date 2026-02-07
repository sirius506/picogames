#define VERTICAL 0
#define HORIZONTAL 1
#define LCD_ALIGNMENT VERTICAL
//#define LCD_ALIGNMENT HORIZONTAL

#if LCD_ALIGNMENT == VERTICAL
	#define X_RES 240 // 横方向解像度
	#define Y_RES 320 // 縦方向解像度
#else
	#define X_RES 320 // 横方向解像度
	#define Y_RES 240 // 縦方向解像度
#endif

#define	LCD_SCK	  10
#define	LCD_MOSI  11
#define	LCD_MISO  12
#define LCD_CS    13 //GPIO13
#define LCD_DC    14 //GPIO14
#define LCD_RESET 15 //GPIO15
#define SPICH spi1

#define	TOUCH_IRQ	2
#define	TOUCH_MOSI	3
#define	TOUCH_MISO	4
#define	TOUCH_CS	5
#define	TOUCH_SCK	6
#define	TOUCH_SPI	spi0

void LCD_WriteComm(unsigned char comm);
void LCD_WriteComm2(unsigned char *comm, int commlen, unsigned char *param, int paramlen);
void LCD_WriteData(unsigned char data);
void LCD_WriteData2(unsigned short data);
void LCD_WriteData3(unsigned char *cmd, int cmd_size, unsigned char *param, int param_size);
void LCD_WriteDataN(unsigned char *b,int n);
void LCD_Init(void);
void LCD_SetCursor(unsigned short x, unsigned short y);
void LCD_Clear(unsigned short color);
void drawPixel(unsigned short x, unsigned short y, unsigned short color);
unsigned short getColor(unsigned short x, unsigned short y);
