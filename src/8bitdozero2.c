/**
 * @brief 8BitDo Zero 2 Controller driver
 *
 * This controller supports BT interface only.
 * To enter pairing mode. Use MacOS mode method below.
 *
 * 1 Pres A and Start buttons to power on.
 * 2 Confirm blinking, then press Select buttons for three seconds.
 */
#include "pico/stdlib.h"
#include "pico/float.h"
#include "stdio.h"
#include "gamepad.h"
#include "btstack.h"

/*
 * button[0] : Left & Right
 *             0x00: Left
 *             0x80: Not pressed
 *             0xFF: Right
 *
 * button[1] : Up & Down
 *             0x00: Up
 *             0x80: Not pressed
 *             0xFF: Down
 *
 * button[4] : A, B, X, Y
 *             0x08: Not pressed
 *             0x18: Y
 *             0x28: B
 *             0x48: A
 *             0x88: X
 *
 * button[5] : L1, R1, Select, Start
 *             0x00: Not pressed
 *             0x01: L1
 *             0x02: R1
 *             0x10: Select
 *             0x20: Start
 */

struct zero2_input_report {
  uint8_t buttons[8];
} __attribute__((packed));

typedef struct zero2_input_report ZERO2_INPUT_REPORT;

#define	REPORT_SIZE	sizeof(ZERO2_INPUT_REPORT)

ZERO2_INPUT_REPORT zero2_cur_report;
ZERO2_INPUT_REPORT zero2_prev_report;

static void Zero2_PadKey_Events(struct zero2_input_report *rp, uint32_t vbutton);

#define	VBMASK_CHECK	(VBMASK_DOWN|VBMASK_RIGHT|VBMASK_LEFT| \
			 VBMASK_UP|VBMASK_PS|VBMASK_TRIANGLE| \
                         VBMASK_L1|VBMASK_R1| \
			 VBMASK_CIRCLE|VBMASK_CROSS|VBMASK_SQUARE)

static void Zero2BtDisconnect()
{
}

/*
 * Decode Zero2 Input report
 */
static void Zero2DecodeInputReport(HID_REPORT *report)
{
  ZERO2_INPUT_REPORT *rp;

  report->ptr++;

  if (report->ptr[0] == 0x01)
  {

    memcpy(&zero2_cur_report, report->ptr + 1, REPORT_SIZE);

    rp = &zero2_cur_report;
 
    if (memcmp(&zero2_prev_report, &zero2_cur_report, REPORT_SIZE))
    {
      uint32_t vbutton = 0;

#ifdef DEBUG_8BIT
      debug_printf("Buttons: %02x %02x %02x %02x", rp->buttons[0], rp->buttons[1], rp->buttons[2], rp->buttons[3]);
      debug_printf(" %02x %02x %02x %02x\n", rp->buttons[4], rp->buttons[5], rp->buttons[6], rp->buttons[7]);
#endif
      memcpy(&zero2_prev_report, &zero2_cur_report, REPORT_SIZE);

      if (rp->buttons[0] == 0x00)
        vbutton |= VBMASK_LEFT;
      else if (rp->buttons[0] == 0xff)
        vbutton |= VBMASK_RIGHT;

      if (rp->buttons[1] == 0x00)
        vbutton |= VBMASK_UP;
      else if (rp->buttons[1] == 0xff)
        vbutton |= VBMASK_DOWN;

      if (rp->buttons[4] & 0x80)
        vbutton |= VBMASK_TRIANGLE;
      if (rp->buttons[4] & 0x40)
        vbutton |= VBMASK_CIRCLE;
      if (rp->buttons[4] & 0x20)
        vbutton |= VBMASK_CROSS;
      if (rp->buttons[4] & 0x10)
        vbutton |= VBMASK_SQUARE;

      if (rp->buttons[5] & 0x01)
        vbutton |= VBMASK_L1;
      if (rp->buttons[5] & 0x02)
        vbutton |= VBMASK_R1;
      if (rp->buttons[5] & 0x10)
        vbutton |= VBMASK_SHARE;
      if (rp->buttons[5] & 0x20)
        vbutton |= VBMASK_PS;

      Zero2_PadKey_Events(rp, vbutton);
    }
  }
}

static uint32_t last_button;
static int pad_timer;

/**
 * @brief Convert HID input report to LVGL kaycode
 */
static void Zero2_PadKey_Events(struct zero2_input_report *rp, uint32_t vbutton)
{
  UNUSED(rp);

  if (vbutton != last_button)
  {
    uint32_t changed;
    const PADKEY_DATA *padkey = GetPadKeyTable();

    changed = last_button ^ vbutton;
    changed &= VBMASK_CHECK;

    post_vkeymask(vbutton);
    last_button = vbutton;
  }
}

void Zero2BtSetup(uint16_t hid_host_cid)
{
  UNUSED(hid_host_cid);
}

void Zero2BtProcessCalibReport(const uint8_t *bp, int len)
{
  UNUSED(bp);
  UNUSED(len);
}

const struct sGamePadDriver Zero2Driver = {
  "8BitDo Zero 2",
  0,
  Zero2DecodeInputReport,
  Zero2BtSetup,
  Zero2BtProcessCalibReport,
  Zero2BtDisconnect,
};
