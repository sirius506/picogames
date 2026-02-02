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
  { VBMASK_DOWN,     PAD_KEY_DOWN, "Down" },
  { VBMASK_RIGHT,    PAD_KEY_RIGHT, "Right" },
  { VBMASK_LEFT,     PAD_KEY_LEFT, "Left" },
  { VBMASK_UP,       PAD_KEY_UP,   "Up" },
  { VBMASK_PS,       PAD_KEY_PS, "PS" },
  { VBMASK_TRIANGLE, PAD_KEY_TRIANGLE,  "Traiandle" },
  { VBMASK_CIRCLE,   PAD_KEY_CIRCLE,"Circle" },
  { VBMASK_CROSS,    PAD_KEY_CROSS,  "Cross" },
  { VBMASK_SQUARE,   PAD_KEY_SQUARE,"Square" },
  { VBMASK_L1,       PAD_KEY_L1, "L1" },
  { VBMASK_R1,       PAD_KEY_R1, "R1" },
  { VBMASK_L2,       PAD_KEY_L1, "L2" },
  { VBMASK_R2,       PAD_KEY_R1, "R2" },
  { VBMASK_L3,       PAD_KEY_L1, "L3" },
  { VBMASK_R3,       PAD_KEY_R1, "R3" },
  { VBMASK_SHARE,    PAD_KEY_SHARE,  "Share" },
  { VBMASK_OPTION,   PAD_KEY_OPTION, "Option" },
  { VBMASK_TOUCH,    PAD_KEY_TOUCH,  "Touch" },
  { VBMASK_MUTE,     PAD_KEY_MUTE,   "Mute" },
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
