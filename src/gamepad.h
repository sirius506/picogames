#ifndef GAMEPAD_H
#define	GAMEPAD_H
#include "dualsense_report.h"
#include "dualshock4_report.h"

#include "lvgl.h"

#define	HID_MODE_LVGL	0
#define	HID_MODE_GAME	1

#define	VID_SONY	0x054C
#define	VID_8BITDO	0x2DC8
#define	PID_DUALSHOCK	0x09CC
#define	PID_DUALSENSE	0x0CE6
#define	PID_ZERO2	0x05C4
#define	PID_8BITMICRO	0x9020

#define	PAD_KEY_SQUARE	0x41
#define	PAD_KEY_CROSS	0x42
#define	PAD_KEY_CIRCLE	0x43
#define	PAD_KEY_TRIANGLE	0x44
#define	PAD_KEY_L1	0x45
#define	PAD_KEY_R1	0x46
#define	PAD_KEY_L2	0x47
#define	PAD_KEY_R2	0x48
#define	PAD_KEY_SHARE	0x49
#define	PAD_KEY_OPTION	0x4A
#define	PAD_KEY_L3	0x4B
#define	PAD_KEY_R3	0x4C
#define	PAD_KEY_PS	0x4D
#define	PAD_KEY_TOUCH	0x4E
#define	PAD_KEY_MUTE	0x4F
#define	PAD_KEY_UP	0x50
#define	PAD_KEY_LEFT	0x51
#define	PAD_KEY_RIGHT	0x52
#define	PAD_KEY_DOWN	0x53

#define	FEATURE_STICK	0x01
#define	FEATURE_ACCEL	0x02
#define	FEATURE_GYRO	0x03
#define	FEATURE_AUTO	0x80

/*
 * Vertual Button bitmask definitions
 */
#define	VBMASK_SQUARE	(1<<0)
#define	VBMASK_CROSS	(1<<1)
#define	VBMASK_CIRCLE	(1<<2)
#define	VBMASK_TRIANGLE	(1<<3)
#define	VBMASK_L1	(1<<4)
#define	VBMASK_R1	(1<<5)
#define	VBMASK_L2	(1<<6)
#define	VBMASK_R2	(1<<7)
#define	VBMASK_SHARE	(1<<8)
#define	VBMASK_OPTION	(1<<9)
#define	VBMASK_L3	(1<<10)
#define	VBMASK_R3	(1<<11)
#define	VBMASK_PS	(1<<12)
#define VBMASK_TOUCH    (1<<13)
#define VBMASK_MUTE     (1<<14)
#define VBMASK_UP       (1<<15)
#define VBMASK_LEFT     (1<<16)
#define VBMASK_RIGHT    (1<<17)
#define VBMASK_DOWN     (1<<18)

#define	NUM_VBUTTONS	15	// Number of virtual buttons exclude direciton keys

typedef struct {
  uint8_t  *ptr;
  uint16_t len;
  uint8_t  hid_mode;
} HID_REPORT;

typedef struct sGamePadDriver {
  char *name;
  uint16_t  feature;
  void (*DecodeInputReport)(HID_REPORT *report);
  void (*btSetup)(uint16_t cid);
  void (*btProcessGetReport)(const uint8_t *report, int len);
  void (*btDisconnect)(void);
} GAMEPAD_DRIVER;

typedef struct {
  uint32_t mask;
  lv_key_t lvkey;
  char     *name;
} PADKEY_DATA;

struct sGamePad {
  uint16_t  vid;
  uint16_t  pid;
  const struct sGamePadDriver *padDriver;
};

struct gamepad_touch_point {
  uint8_t  contact;
  uint16_t xpos;
  uint16_t ypos;
};

struct gamepad_inputs {
  uint8_t x, y;
  uint8_t rx, ry;
  uint8_t z, rz;
  uint32_t vbutton;
  /* Motion sensors */
  int16_t gyro[3];
  int16_t accel[3];
  struct  gamepad_touch_point points[2];
  uint8_t Temperature;
  uint8_t battery_level;
};

typedef enum {
  PAD_KEY_VBMASK,
  PAD_KEY_PRESS,
  PAD_KEY_RELEASE,
  PAD_TOUCH_PRESS,
  PAD_TOUCH_RELEASE,
  PAD_LEFT_STICK,
  PAD_RIGHT_STICK,
  PAD_CONNECT,
  PAD_DISCONNECT,
} PADEVENT_TYPE;

typedef struct {
  PADEVENT_TYPE type;
  uint16_t  key_code;
  uint32_t  vmask;
  void      *ptr;
} PADEVENT;

typedef struct {
  PADEVENT_TYPE type;
  uint16_t  lvkey;
  uint16_t  cread;
  uint32_t  vmask;
  void      *ptr;
} PADKEY_EVENT;

typedef struct {
  uint16_t x;
  uint16_t y;
  float    theta;
  float    radius;
} STICKVAL;

#define	STICK_LEFT	0
#define	STICK_RIGHT	1

extern STICKVAL StickVal[2];
extern float AccelVal[3];

#define	INREP_SIZE	sizeof(struct dualsense_input_report)
#define	OUTREP_SIZE	sizeof(struct dualsense_btout_report)

typedef struct {
  uint8_t InputReportBuffer[INREP_SIZE];
  uint8_t OutputReportBuffer[OUTREP_SIZE*2];
  uint8_t out_toggle;
} GAMEPAD_BUFFERS;

extern GAMEPAD_BUFFERS *GetGamePadBuffer();

extern const struct sGamePadDriver DualShockDriver;
extern const struct sGamePadDriver DualSenseDriver;
extern const struct sGamePadDriver Zero2Driver;
extern const struct sGamePadDriver MicroDriver;

extern const GAMEPAD_DRIVER *IsSupportedGamePad(uint16_t vid, uint16_t pid);

extern const PADKEY_DATA *GetPadKeyTable();
extern void post_event(uint16_t type, uint16_t code, void *ptr);
extern void post_vkeymask(uint32_t mask);
extern void post_padevent(PADKEY_EVENT *padevent);
PADEVENT *read_pad_event();

#endif
