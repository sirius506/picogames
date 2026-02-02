/**
 * @brief SONY DUALSHOCK 4 Controller driver
 */
#include "pico/stdlib.h"
#include "pico/float.h"
#include "stdio.h"
#include "gamepad.h"
#include "btstack.h"

#define	SAMPLE_PERIOD	(0.00250f)
#define	SAMPLE_RATE	(400)
#define le16_to_cpu(x)  (x)

//#define ENABLE_OUTPUT_REPORT

uint8_t ds4calibdata[DS4_FEATURE_REPORT_CALIBRATION_SIZE+4];

static void DS4_PadKey_Events(struct ds4_input_report *rp, uint8_t hat, uint32_t vbutton, HID_REPORT *rep);

static struct ds4_data DsData;

/* 0x08:  No button
 * 0x00:  Up
 * 0x01:  RightUp
 * 0x02:  Right
 * 0x03:  RightDown
 * 0x04:  Down
 * 0x05:  LeftDown
 * 0x06:  Left
 * 0x07:  LeftUp
 */

/*
 * @brief Hat key to Virtual button mask conversion table
 */
static const uint32_t hatmap[16] = {
  VBMASK_UP,   VBMASK_UP|VBMASK_RIGHT,  VBMASK_RIGHT, VBMASK_RIGHT|VBMASK_DOWN,
  VBMASK_DOWN, VBMASK_LEFT|VBMASK_DOWN, VBMASK_LEFT,  VBMASK_UP|VBMASK_LEFT,
  0, 0, 0, 0,
  0, 0, 0, 0,
};

#define	VBMASK_CHECK	(VBMASK_DOWN|VBMASK_RIGHT|VBMASK_LEFT| \
			 VBMASK_UP|VBMASK_PS|VBMASK_TRIANGLE| \
                         VBMASK_L1|VBMASK_R1| \
			 VBMASK_CIRCLE|VBMASK_CROSS|VBMASK_SQUARE)

#ifdef ENABLE_OUTPUT_REPORT
void process_output_report(int hid_mode)
{
  struct ds4_bt_output_report *brp;
  struct ds4_output_report *rp;
  GAMEPAD_BUFFERS *gpb = GetGamePadBuffer();
  uint8_t cval[3];

  if (gpb->out_toggle)
    brp = (struct ds4_bt_output_report *)(gpb->OutputReportBuffer + sizeof(struct ds4_bt_output_report));
  else
    brp = (struct ds4_bt_output_report *)gpb->OutputReportBuffer;

  memset(brp, 0, sizeof(*brp));
  brp->report_id = DS4_OUTPUT_REPORT_BT;
  brp->hw_control = DS4_OUTPUT_HWCTL_HID | DS4_OUTPUT_HWCTL_CRC32;
  rp = &brp->out_report;

  gpb->out_toggle ^= 1;

  brp->report_id = DS4_OUTPUT_REPORT_BT;
  rp->valid_flag0 = DS4_OUTPUT_VALID_FLAG0_LED;


  int len;

  len = sizeof(struct ds4_bt_output_report);
  brp->crc = bt_comp_crc((uint8_t *)brp, len);
  btapi_send_report((uint8_t *)brp, len);
}
#endif

static uint8_t prev_blevel;

static void DualShockBtDisconnect()
{
  prev_blevel = 0;
}

static void process_accel(struct ds4_input_report *rp)
{
  struct ds4_data *ds = &DsData;
  int raw_data;
  float calib_data;
  for (int i = 0; i < 3; i++)
  {
    raw_data = le16_to_cpu(rp->accel[i]);
    calib_data = (float)(raw_data - ds->accel_calib_data[i].bias) * ds->accel_calib_data[i].sensitivity;
    AccelVal[i] = calib_data / DS4_ACC_RES_PER_G;
  }
}

/*
 * Decode DualShock Input report
 */
static void DualShockDecodeInputReport(HID_REPORT *report)
{
  DS4_INPUT_REPORT *rp;
  static uint32_t in_seq;
  uint8_t blevel;
  uint8_t ctype;
  static int dcount;

  if ((report->len != DS4_INPUT_REPORT_BT_SIZE) && (report->len != DS4_INPUT_REPORT_BT_SIZE+1))
  {
    return;
  }

  report->ptr++;

  ctype = report->ptr[0];
  if (ctype != DS4_INPUT_REPORT_BT)
    return;

  rp = (DS4_INPUT_REPORT *)(report->ptr + 3);
 
  if (ctype != DS4_INPUT_REPORT_BT)
    return;

  dcount++;

  {
    uint8_t hat;
    uint32_t vbutton;

    hat = (rp->buttons[0] & 0x0f);
    vbutton = (rp->buttons[2] & 0x03) << 12;	/* Home, Pad */
    vbutton |= rp->buttons[1] << 4;		/* L1, R1, L2, R2, Share, Option, L3, R3 */
    vbutton |= (rp->buttons[0] & 0xf0)>> 4;	/* Square, Cross, Circle, Triangle */
    vbutton |= hatmap[hat];

    DS4_PadKey_Events(rp, hat, vbutton, report);
  }

  if ((rp->status[0] & 0x0F) != prev_blevel)
  {
    blevel = rp->status[0] & 0x0F;
    if (blevel > 9) blevel = 9;
    blevel = blevel / 2;
printf("Battery: 0x%02x, %d\n", rp->status[0], blevel);
#if 0
    postGuiEventMessage(GUIEV_ICON_CHANGE, ICON_BATTERY | blevel, NULL, NULL);
#endif
    prev_blevel = rp->status[0] & 0x0F;
  }

  process_accel(rp);

  if (report->ptr[0] == DS4_INPUT_REPORT_BT)
  {
#ifdef ENABLE_OUTPUT_REPORT
    if ((in_seq & 3) == 0)
      process_output_report(report->hid_mode);
#endif
    in_seq++;
  }
}

static uint32_t last_button;
#ifdef USE_PAD_TIMER
static int pad_timer;
#endif
static int16_t left_xinc, left_yinc;
static int16_t right_xinc, right_yinc;
#if 0
extern lv_indev_data_t tp_data;

static void decode_tp(struct ds4_input_report *rp, HID_REPORT *rep)
{
  UNUSED(rp);
  struct ds4_bt_input_report *bt_rep;
  struct ds4_touch_report *tp;
  int num_report;
  uint16_t state, xpos, ypos;

  if (rep->ptr[0] != DS4_INPUT_REPORT_BT)
    return;

  bt_rep = (struct ds4_bt_input_report *)rep->ptr;
  num_report = bt_rep->num_touch_reports;
  tp = bt_rep->reports;

  if (num_report > 0 && tp)
  {
    state = (tp->points[0].contact & 0x80)? LV_INDEV_STATE_RELEASED : LV_INDEV_STATE_PRESSED;
    xpos = (tp->points[0].x_hi << 8) | (tp->points[0].x_lo);
    ypos = (tp->points[0].y_hi << 4) | (tp->points[0].y_lo);
    xpos = xpos * 480 / DS4_TOUCHPAD_WIDTH;
    ypos = ypos * 320 / DS4_TOUCHPAD_HEIGHT;
  }
  else
  {
    state = LV_INDEV_STATE_RELEASED;
  }
  if (tp_data.state != state)
  {
    if (state == LV_INDEV_STATE_PRESSED)
    {
      tp_data.state = state;
      gamepad_grab_owner();
    }
    else if (gamepad_is_owner())
    {
      tp_data.state = state;
      gamepad_ungrab_owner();
    }
  }
  if (state == LV_INDEV_STATE_PRESSED)
  {
      tp_data.point.x = xpos;
      tp_data.point.y = ypos;
  }
}
#endif

static void decode_stick(struct ds4_input_report *rp)
{
  int ix, iy, ax, ay;
  static int dcount;

  ix = rp->x - 128;
  iy = rp->y - 128;
  ax = (ix < 0)? -ix : ix;
  ay = (iy < 0)? -iy : iy;
  ax = (ax > 80) ? 1 : 0;
  ay = (ay > 80) ? 1 : 0;

  StickVal[STICK_LEFT].x = ix;
  StickVal[STICK_LEFT].y = iy;

  if (ax != 0)
  {
    if (ix < 0) ax = -1;
    if (ax != left_xinc)
    {
#if 0
      postGuiEventMessage(GUIEV_LEFT_XDIR, ax, NULL, NULL);
#endif
    }
  }
  left_xinc = ax;

  if (ay != 0)
  {
    if (iy < 0) ay = -1;
#if 0
    if (ay != left_yinc)
      postGuiEventMessage(GUIEV_LEFT_YDIR, ay, NULL, NULL);
#endif
  }
  left_yinc = ay;

  ix = rp->rx - 128;
  iy = rp->ry - 128;
  ax = (ix < 0)? -ix : ix;
  ay = (iy < 0)? -iy : iy;
  ax = (ax > 80) ? 1 : 0;
  ay = (ay > 80) ? 1 : 0;
  StickVal[STICK_RIGHT].x = ix;
  StickVal[STICK_RIGHT].y = iy;

  if (dcount % 50 == 0)
  {
    float fx, fy;

    fx = (float) ix / 128.0;
    fy = (float) iy / 128.0;
    StickVal[STICK_RIGHT].theta = atan2f(fx, fy);
    StickVal[STICK_RIGHT].radius = sqrtf(fx * fx + fy * fy);
    //printf("%f, %f\n", StickVal[STICK_RIGHT].theta, StickVal[STICK_RIGHT].radius * 128);
  }
  dcount++;

  if (ax != 0)
  {
    if (ix < 0) ax = -1;
#if 0
    if (ax != right_xinc)
      postGuiEventMessage(GUIEV_RIGHT_XDIR, ax, NULL, NULL);
#endif
  }
  right_xinc = ax;

  if (ay != 0)
  {
    if (iy < 0) ay = -1;
#if 0
    if (ay != right_yinc)
      postGuiEventMessage(GUIEV_RIGHT_YDIR, ay, NULL, NULL);
#endif
  }
  right_yinc = ay;
}

/**
 * @brief Convert HID input report to LVGL kaycode
 */
static void DS4_PadKey_Events(struct ds4_input_report *rp, uint8_t hat, uint32_t vbutton, HID_REPORT *rep)
{
  UNUSED(hat);

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
      }
      padkey++;
    }
#else
    post_vkeymask(vbutton);
#endif
    last_button = vbutton;
  }
#ifdef USE_PAD_TIMER
  else if (pad_data.state == LV_INDEV_STATE_PRESSED)
  {
    /* Key has been pressed */

    pad_timer++;
    if (pad_timer > 15)		/* Takes repeat start delay */
    {
      if ((pad_timer & 3) == 0)	/* inter repeat delay passed */
      {
        /* Generate release and press event */
        pad_data.state = LV_INDEV_STATE_RELEASED;
        send_padkey(&pad_data);
        pad_data.state = LV_INDEV_STATE_PRESSED;
        send_padkey(&pad_data);
      }
    }
  }
#endif
  decode_stick(rp);
#if 0
  decode_tp(rp, rep);
#endif
}

void DualShockBtSetup(uint16_t hid_host_cid)
{
  GAMEPAD_BUFFERS *gpb = GetGamePadBuffer();

  gpb->out_toggle = 0;

  hid_host_send_get_report(hid_host_cid, HID_REPORT_TYPE_FEATURE, DS4_FEATURE_REPORT_CALIBRATION_BT);
}

static int16_t calibVals[17];


static int16_t get_le16val(uint8_t *bp)
{
  int16_t val;
  
  val = bp[0] + (bp[1] << 8);
  return val;
}

static void process_calibdata(struct ds4_data *ds, uint8_t *dp)
{
  int16_t gyro_pitch_bias, gyro_yaw_bias, gyro_roll_bias;
  int16_t gyro_pitch_plus, gyro_pitch_minus;
  int16_t gyro_yaw_plus, gyro_yaw_minus;
  int16_t gyro_roll_plus, gyro_roll_minus;
  int16_t gyro_speed_plus, gyro_speed_minus;
  int16_t acc_x_plus, acc_x_minus;
  int16_t acc_y_plus, acc_y_minus;
  int16_t acc_z_plus, acc_z_minus;

  float flNumerator;
  int range_2g;

  calibVals[0] = gyro_pitch_bias = get_le16val(dp + 1);
  calibVals[1] = gyro_yaw_bias = get_le16val(dp + 3);
  calibVals[2] = gyro_roll_bias = get_le16val(dp + 5);

  {
    calibVals[3] = gyro_pitch_plus = get_le16val(dp + 7);
    calibVals[4] = gyro_pitch_minus = get_le16val(dp + 13);
    calibVals[5] = gyro_yaw_plus = get_le16val(dp + 9);
    calibVals[6] = gyro_yaw_minus = get_le16val(dp + 15);
    calibVals[7] = gyro_roll_plus = get_le16val(dp + 11);
    calibVals[8] = gyro_roll_minus = get_le16val(dp + 17);
  }

  calibVals[9] = gyro_speed_plus = get_le16val(dp + 19);
  calibVals[10] = gyro_speed_minus = get_le16val(dp + 21);
  calibVals[11] = acc_x_plus = get_le16val(dp + 23);
  calibVals[12] = acc_x_minus = get_le16val(dp + 25);
  calibVals[13] = acc_y_plus = get_le16val(dp + 27);
  calibVals[14] = acc_y_minus = get_le16val(dp + 29);
  calibVals[15] = acc_z_plus = get_le16val(dp + 31);
  calibVals[16] = acc_z_minus = get_le16val(dp + 33);

  /*
   * Set gyroscope calibration and normalization parameters.
   * Data values will be normalized to 1/DS4_GYRO_RES_PER_DEG_S degree/s.
   */

  flNumerator = (gyro_speed_plus + gyro_speed_minus) * DS4_GYRO_RES_PER_DEG_S;

  ds->gyro_calib_data[0].bias = gyro_pitch_bias;
  ds->gyro_calib_data[0].sensitivity = flNumerator / (gyro_pitch_plus - gyro_pitch_minus);

  ds->gyro_calib_data[1].bias = gyro_yaw_bias;
  ds->gyro_calib_data[1].sensitivity = flNumerator / (gyro_yaw_plus - gyro_yaw_minus);

  ds->gyro_calib_data[2].bias = gyro_roll_bias;
  ds->gyro_calib_data[2].sensitivity = flNumerator / (gyro_roll_plus - gyro_roll_minus);

  /*
   * Set accelerometer calibration and normalization parameters.
   * Data values will be normalized to 1/DS4_ACC_RES_PER_G g.
   */
  range_2g = acc_x_plus - acc_x_minus;
  ds->accel_calib_data[0].bias = acc_x_plus - range_2g / 2;
  ds->accel_calib_data[0].sensitivity = 2.0f * DS4_ACC_RES_PER_G / (float)range_2g;

  range_2g = acc_y_plus - acc_y_minus;
  ds->accel_calib_data[1].bias = acc_y_plus - range_2g / 2;
  ds->accel_calib_data[1].sensitivity = 2.0f * DS4_ACC_RES_PER_G / (float)range_2g;

  range_2g = acc_z_plus - acc_z_minus;
  ds->accel_calib_data[2].bias = acc_z_plus - range_2g / 2;
  ds->accel_calib_data[2].sensitivity = 2.0f * DS4_ACC_RES_PER_G / (float)range_2g;
}

void DualShockBtProcessCalibReport(const uint8_t *bp, int len)
{ 
  if (len == DS4_FEATURE_REPORT_CALIBRATION_SIZE+4)
  { 
    memcpy(ds4calibdata, bp, DS4_FEATURE_REPORT_CALIBRATION_SIZE + 4);
    process_calibdata(&DsData, ds4calibdata);
  }
}

const struct sGamePadDriver DualShockDriver = {
  "DualShock4",
  FEATURE_STICK | FEATURE_ACCEL,
  DualShockDecodeInputReport,
  DualShockBtSetup,
  DualShockBtProcessCalibReport,
  DualShockBtDisconnect,
};

