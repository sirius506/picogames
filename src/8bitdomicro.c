/**
 * @brief 8BitDo Micro Controller driver
 *
 * This controller supports BT interface only.
 * To enter pairing mode. Use Android mode method below.
 *
 * 1 Set mode switch to 'D' position
 * 2 Press power button
 * 3 Keep press pairing button three seconds.
 *   Confirm fast blinking LED.
 */
#include "pico/stdlib.h"
#include "pico/float.h"
#include "stdio.h"
#include "gamepad.h"
#include "btstack.h"

/*
ABYX                                 *
Buttons: a1 03 08 7f 7f 7f 7f 00 00 01 00 57	A	buttons[9] & 0x01
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 57
Buttons: a1 03 08 7f 7f 7f 7f 00 00 02 00 57	B	buttons[9] & 0x02
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 57
Buttons: a1 03 08 7f 7f 7f 7f 00 00 10 00 57	Y	buttons[9] & 0x10
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 57
Buttons: a1 03 08 7f 7f 7f 7f 00 00 08 00 57	X	buttpns[8] & 0x08
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 57

URDL               *  *
Buttons: a1 03 08 7f 00 7f 7f 00 00 00 00 57	U	buttons[4] == 0x00
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 57
Buttons: a1 03 08 ff 7f 7f 7f 00 00 00 00 57	R	buttons[3] == 0xFF
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 57
Buttons: a1 03 08 7f ff 7f 7f 00 00 00 00 57	D	buttpns[4] == 0xFF
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 57
Buttons: a1 03 08 00 7f 7f 7f 00 00 00 00 57	L	buttpns[3] == 0x00
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 57

U+R
Buttons: a1 03 08 ff 7f 7f 7f 00 00 00 00 58
Buttons: a1 03 08 ff 00 7f 7f 00 00 00 00 58	U+R
Buttons: a1 03 08 ff 7f 7f 7f 00 00 00 00 58
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 58

R+D
Buttons: a1 03 08 7f ff 7f 7f 00 00 00 00 57
Buttons: a1 03 08 ff ff 7f 7f 00 00 00 00 57
Buttons: a1 03 08 7f ff 7f 7f 00 00 00 00 57
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 57

D+L
Buttons: a1 03 08 00 7f 7f 7f 00 00 00 00 57
Buttons: a1 03 08 00 ff 7f 7f 00 00 00 00 57
Buttons: a1 03 08 7f ff 7f 7f 00 00 00 00 57
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 57

L+U
Buttons: a1 03 08 00 7f 7f 7f 00 00 00 00 57
Buttons: a1 03 08 00 00 7f 7f 00 00 00 00 57
Buttons: a1 03 08 00 7f 7f 7f 00 00 00 00 57
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 57

+-
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 08 57	buttons[10] & 0x08
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 57
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 04 57	buttons[10] & 0x04
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 57

home
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 10 57	buttons[10] & 0x10
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 57

L1, L2
Buttons: a1 03 08 7f 7f 7f 7f 00 00 40 00 57	buttons[9] & 0x40
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 57
Buttons: a1 03 08 7f 7f 7f 7f 00 ff 00 01 57	buttons[8] == 0xFF buttons[10] & 0x01
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 57

R1, R2
Buttons: a1 03 08 7f 7f 7f 7f 00 00 80 00 57	buttons[9] & 0x80
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 57
Buttons: a1 03 08 7f 7f 7f 7f ff 00 00 02 57	buttons[7] == 0xFF buttons[10] & 0x02
Buttons: a1 03 08 7f 7f 7f 7f 00 00 00 00 57

*/

struct micro_input_report {
  uint8_t buttons[12];
} __attribute__((packed));

typedef struct micro_input_report MICRO_INPUT_REPORT;

#define	REPORT_SIZE	sizeof(MICRO_INPUT_REPORT)

MICRO_INPUT_REPORT micro_cur_report;
MICRO_INPUT_REPORT micro_prev_report;

static void Micro_PadKey_Events(struct micro_input_report *rp, uint32_t vbutton);

#define	VBMASK_CHECK	(VBMASK_DOWN|VBMASK_RIGHT|VBMASK_LEFT| \
			 VBMASK_UP|VBMASK_PS|VBMASK_TRIANGLE| \
                         VBMASK_L1|VBMASK_L2|VBMASK_R1|VBMASK_R2| \
			 VBMASK_SHARE|VBMASK_OPTION| \
			 VBMASK_CIRCLE|VBMASK_CROSS|VBMASK_SQUARE)

static void MicroBtDisconnect()
{
printf("%s\n", __FUNCTION__);
}

/*
 * Decode Micro Input report
 */
static void MicroDecodeInputReport(HID_REPORT *report)
{
  MICRO_INPUT_REPORT *rp;

  if (report->ptr[0] == 0xa1)
  {

    memcpy(&micro_cur_report, report->ptr, REPORT_SIZE);

    rp = &micro_cur_report;
 
    if (memcmp(&micro_prev_report, &micro_cur_report, REPORT_SIZE))
    {
      uint32_t vbutton = 0;

#ifdef MICRO_DEBUG
      printf("Buttons: %02x %02x %02x %02x", rp->buttons[0], rp->buttons[1], rp->buttons[2], rp->buttons[3]);
      printf(" %02x %02x %02x %02x", rp->buttons[4], rp->buttons[5], rp->buttons[6], rp->buttons[7]);
      printf(" %02x %02x %02x %02x\n", rp->buttons[8], rp->buttons[9], rp->buttons[10], rp->buttons[11]);
#endif
      memcpy(&micro_prev_report, &micro_cur_report, REPORT_SIZE);

      if (rp->buttons[3] == 0x00)
        vbutton |= VBMASK_LEFT;
      else if (rp->buttons[3] == 0xff)
        vbutton |= VBMASK_RIGHT;

      if (rp->buttons[4] == 0x00)
        vbutton |= VBMASK_UP;
      else if (rp->buttons[4] == 0xff)
        vbutton |= VBMASK_DOWN;

      if (rp->buttons[8] & 0x08)
        vbutton |= VBMASK_TRIANGLE;
      if (rp->buttons[9] & 0x01)
        vbutton |= VBMASK_CIRCLE;
      if (rp->buttons[9] & 0x02)
        vbutton |= VBMASK_CROSS;
      if (rp->buttons[9] & 0x10)
        vbutton |= VBMASK_SQUARE;

      if (rp->buttons[9] & 0x40)
        vbutton |= VBMASK_L1;
      if (rp->buttons[10] & 0x01)
        vbutton |= VBMASK_L2;
      if (rp->buttons[9] & 0x80)
        vbutton |= VBMASK_R1;
      if (rp->buttons[10] & 0x02)
        vbutton |= VBMASK_R2;
      if (rp->buttons[10] & 0x04)
        vbutton |= VBMASK_SHARE;
      if (rp->buttons[10] & 0x08)
        vbutton |= VBMASK_OPTION;
      if (rp->buttons[10] & 0x10)
        vbutton |= VBMASK_PS;

      Micro_PadKey_Events(rp, vbutton);
    }
  }
}

static uint32_t last_button;
static int pad_timer;

/**
 * @brief Convert HID input report to LVGL kaycode
 */
static void Micro_PadKey_Events(struct micro_input_report *rp, uint32_t vbutton)
{
  UNUSED(rp);

  if (vbutton != last_button)
  {
    uint32_t changed;
    const PADKEY_DATA *padkey = GetPadKeyTable();

    changed = last_button ^ vbutton;
    changed &= VBMASK_CHECK;

#if 0
    while (changed && padkey->mask)
    {
      if (changed & padkey->mask)
      {
        changed &= ~padkey->mask;

        post_event((vbutton & padkey->mask)? PAD_KEY_PRESS : PAD_KEY_RELEASE, padkey->padcode, NULL);
printf("%s: %x\n", (vbutton & padkey->mask)? "Press" : "Release", padkey->padcode);
      }
      padkey++;
    }
#endif
    post_vkeymask(vbutton);
    last_button = vbutton;
  }
}

void MicroBtSetup(uint16_t hid_host_cid)
{
  UNUSED(hid_host_cid);
}

void MicroBtProcessCalibReport(const uint8_t *bp, int len)
{
  UNUSED(bp);
  UNUSED(len);
}

const struct sGamePadDriver MicroDriver = {
  "8BitDo Micro",
  0,
  MicroDecodeInputReport,
  MicroBtSetup,
  MicroBtProcessCalibReport,
  MicroBtDisconnect,
};
