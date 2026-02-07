/**
 *   Game controller interface
 */
#include "pico/stdlib.h"
#include "stdio.h"
#include "btstack.h"
#include "gamepad.h"

/**
 * @brief List of supported gamepads.
 */
const struct sGamePad KnownGamePads[] = {
  { VID_SONY, PID_DUALSENSE, &DualSenseDriver },
  { VID_SONY, PID_DUALSHOCK, &DualShockDriver },
  { VID_SONY, PID_ZERO2,     &Zero2Driver },
  { VID_8BITDO, PID_8BITMICRO, &MicroDriver },
};

STICKVAL StickVal[2];
float    AccelVal[3];

/*
 * Map Virtual button bitmask to LVGL Keypad code
 */
static const PADKEY_DATA PadKeyDefs[] = {
  { VBMASK_DOWN,     LV_KEY_NEXT, "Down" },
  { VBMASK_RIGHT,    LV_KEY_NEXT, "Right" },
  { VBMASK_LEFT,     LV_KEY_PREV, "Left" },
  { VBMASK_UP,       LV_KEY_PREV, "Up" },
  { VBMASK_PS,       LV_KEY_HOME, "PS" },
  { VBMASK_TRIANGLE, LV_KEY_DEL,  "Traiandle" },
  { VBMASK_CIRCLE,   LV_KEY_ENTER,"Circle" },
  { VBMASK_CROSS,    LV_KEY_DEL,  "Cross" },
  { VBMASK_SQUARE,   LV_KEY_ENTER,"Square" },
  { VBMASK_L1,       LV_KEY_LEFT, "L1" },
  { VBMASK_R1,       LV_KEY_RIGHT,"R1" },
  { VBMASK_L2,       LV_KEY_LEFT, "L2" },
  { VBMASK_R2,       LV_KEY_RIGHT,"R2" },
  { VBMASK_L3,       LV_KEY_LEFT, "L3" },
  { VBMASK_R3,       LV_KEY_RIGHT,"R3" },
  { VBMASK_SHARE,    LV_KEY_ESC,  "Share" },
  { VBMASK_OPTION,   LV_KEY_ESC , "Option" },
  { VBMASK_TOUCH,    LV_KEY_ENTER,"Touch" },
  { VBMASK_MUTE,     LV_KEY_ESC,  "Mute" },
  { 0, 0 },			/* Data END marker */
};

static GAMEPAD_BUFFERS gamepadBuffer;

GAMEPAD_BUFFERS *GetGamePadBuffer()
{
  return &gamepadBuffer;
}

const PADKEY_DATA *GetPadKeyTable()
{
  return PadKeyDefs;
}

/**
 * @brief See if game pad is supported
 * @param vid: Vendor ID
 * @param pid: Product ID
 * @return Pointer to GAMEPAD_INFO or NULL
 */
const GAMEPAD_DRIVER *IsSupportedGamePad(uint16_t vid, uint16_t pid)
{
  unsigned int i;
  const struct sGamePad *gp = KnownGamePads;

  for (i = 0; i < sizeof(KnownGamePads)/sizeof(struct sGamePad); i++)
  {
    if (gp->vid == vid && gp->pid == pid)
    {
      return gp->padDriver;
    }
    gp++;
  }
  printf("This gamepad is not supported.\n");
  return NULL;
}

#ifdef ENABLE_REPORT
#define	PS_OUTPUT_CRC32_SEED	0xA2

static const uint8_t output_seed[] = { PS_OUTPUT_CRC32_SEED };

/**
 * @brief Compute CRC32 value for BT output report
 */
uint32_t bt_comp_crc(uint8_t *ptr, int len)
{
  uint32_t crcval;

  crcval = bsp_calc_crc((uint8_t *)output_seed, 1);
  crcval = bsp_accumulate_crc(ptr, len - 4);
  crcval = ~crcval;
  return crcval;
}
#endif
