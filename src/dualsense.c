/**
 * @brief SONY Dual Sense Controller driver
 */
#include "pico/stdlib.h"
#include "pico/float.h"
#include "stdio.h"
#include "gamepad.h"
#include "btstack.h"

static int initial_report;
static struct dsense_data DsData;
static int16_t calibVals[17];

static void DualSense_PadKey_Events(struct dualsense_input_report *rp, uint8_t hat, uint32_t vbutton);

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
                         VBMASK_L1|VBMASK_R1|VBMASK_L2|VBMASK_R2| \
                         VBMASK_SHARE|VBMASK_OPTION| \
                         VBMASK_TOUCH|VBMASK_MUTE| \
			 VBMASK_CIRCLE|VBMASK_CROSS|VBMASK_SQUARE)

#define	TRIG_FEEDBACK	0x21

#define	le16_to_cpu(x)	(x)

void TriggerFeedbackSetup(uint8_t *dst, int position, int strength)
{
  if (position > 9 || position < 0)
    return;
  if (strength > 8 || strength < 0)
    return;
  if (strength > 0)
  {
    uint8_t forceValue = (strength - 1) & 0x07;
    uint32_t forceZones = 0;
    uint16_t activeZones = 0;

    for (int i = position; i < 10; i++)
    {
      forceZones |= (uint32_t)(forceValue << (3 * i));
      activeZones |= (uint16_t)(i << i);
    }
    dst[0] = TRIG_FEEDBACK;
    dst[1] = (activeZones & 0xff);
    dst[2] = (activeZones >> 8) & 0xff;
    dst[3] = (forceZones & 0xff);
    dst[4] = (forceZones >> 8) & 0xff;
    dst[5] = (forceZones >> 16) & 0xff;
    dst[6] = (forceZones >> 24) & 0xff;
    dst[7] = 0x00;
    dst[8] = 0x00;
    dst[9] = 0x00;
    dst[10] = 0x00;
  }
}

#if 0
static void set_joystick_params()
{
  SDL_Joystick *joystick = SDL_GetJoystickPtr();

  joystick->name = "DualSence";
  joystick->naxes = 1;
  joystick->nhats = 1;
  joystick->nbuttons = NUM_VBUTTONS;
  joystick->hats = 0;
  joystick->buttons = 0;;
}
#endif

static uint8_t prev_blevel;

static void DualSenseBtDisconnect()
{
  prev_blevel = 0;
}

static void process_accel(struct dualsense_input_report *rp)
{
  struct dsense_data *ds = &DsData;
  int raw_data;
  float calib_data;
  for (int i = 0; i < 3; i++)
  {
    raw_data = le16_to_cpu(rp->accel[i]);
    calib_data = (float)(raw_data - ds->accel_calib_data[i].bias) * ds->accel_calib_data[i].sensitivity;
    AccelVal[i] = calib_data / DS_ACC_RES_PER_G;
  }
}

/*
 * Decode DualSense Input report
 */
static void decode_report(struct dualsense_input_report *rp)
{
  uint8_t hat;
  uint32_t vbutton;
  uint8_t blevel;

  hat = (rp->buttons[0] & 0x0f);
  vbutton = (rp->buttons[2] & 0x0f) << 12;	/* PS, Touch, Mute */
  vbutton |= rp->buttons[1] << 4;		/* L1, R1, L2, R2, L3, R3, Create, Option */
  vbutton |= (rp->buttons[0] & 0xf0)>> 4;	/* Square, Cross, Circle, Triangle */
  vbutton |= hatmap[hat];

  DualSense_PadKey_Events(rp, hat, vbutton);

  if (rp->battery_level != prev_blevel)
  {
    blevel = rp->battery_level & 0x0F;
    printf("Battery: 0x%02x\n", blevel);
    if (blevel > 9) blevel = 9;
    blevel = blevel / 2;
#if 0
    postGuiEventMessage(GUIEV_ICON_CHANGE, ICON_BATTERY | blevel, NULL, NULL);
#endif
    prev_blevel = rp->battery_level;
  }

  process_accel(rp);
}

#ifdef ENABLE_OUTPUT_REPORT
static void process_bt_reports(uint8_t hid_mode);
#endif

static void DualSenseDecodeInputReport(HID_REPORT *report)
{
  struct dualsense_input_report *rp;
  static int dcount;

  if ((report->len != DS_INPUT_REPORT_BT_SIZE) && (report->len != (DS_INPUT_REPORT_BT_SIZE+1)))
  {
#if 0
    debug_printf("Bad report size..\n");
#endif
    return;
  }

  report->ptr += 1;
  rp = (struct dualsense_input_report *)report->ptr;


  if (rp->report_id != DS_INPUT_REPORT_BT)
  {
    printf("Bad report ID. (%x) @ %x\n", rp->report_id, rp);
    return;
  }

  dcount++;

  decode_report(rp);
#ifdef ENABLE_OUTPUT_REPORT
  process_bt_reports(report->hid_mode);
#endif
}

static uint8_t bt_seq;

static void bt_out_init(struct dualsense_btout_report *rp)
{
  memset(rp, 0, sizeof(*rp));
  rp->report_id = DS_OUTPUT_REPORT_BT;
  rp->tag = DS_OUTPUT_TAG;
  rp->seq_tag = bt_seq << 4;
  bt_seq++;
  if (bt_seq >= 16)
    bt_seq = 0;
}

#ifdef ENABLE_REPORT_OUTPUT
static void bt_emit_report(struct dualsense_btout_report *brp)
{
  int len;

  len = sizeof(*brp);
  brp->crc = bt_comp_crc((uint8_t *)brp, len);
  btapi_send_report((uint8_t *)brp, len);
}

static void SetBarColor(DS_OUTPUT_REPORT *rp, int seq, int mode)
{
  uint8_t cval[3];

  if (seq & 1)
  {
    rp->valid_flag1 = DS_OUTPUT_VALID_FLAG1_LIGHTBAR_CONTROL_ENABLE;

    {
      fft_getcolor(cval);
      rp->lightbar_red = cval[0];
      rp->lightbar_green = cval[1];
      rp->lightbar_blue = cval[2];
    }
  }
  else
  {
    rp->valid_flag1 = DS_OUTPUT_VALID_FLAG1_PLAYER_INDICATOR_CONTROL_ENABLE;
    {
      int vval;

      vval = fft_getcolor(cval);
      if (vval > 250)
        rp->player_leds = 0x1f;		// Turn on all LEDs
      else if (vval > 150)
        rp->player_leds = 0x0e;
      else if (vval > 20)
        rp->player_leds = 0x04;		// Center only
      else
        rp->player_leds = 0;		// Turn off all LEDs
    }
    else
    {
      rp->player_leds = (1 << 2);
    }
  }
}

static struct dualsense_btout_report *get_report_buffer()
{
  struct dualsense_btout_report *brp;
  GAMEPAD_BUFFERS *gbp = GetGamePadBuffer();

  brp = (struct dualsense_btout_report *)(gbp->OutputReportBuffer);
  if (gbp->out_toggle)
    brp++;
  gbp->out_toggle ^= 1;
  return brp;
}

static int BtUpdateBarColor(uint8_t hid_mode)
{
  struct dualsense_btout_report *brp;

  brp = get_report_buffer();
  bt_out_init(brp);
  brp->report_id = DS_OUTPUT_REPORT_BT;

  SetBarColor(&brp->com_report, bt_seq, hid_mode);

  bt_emit_report(brp);
  return 1;
}

static void fill_output_request(int init_num, DS_OUTPUT_REPORT *rp)
{
  switch (init_num)
  {
    case 0:
      rp->valid_flag2 = DS_OUTPUT_VALID_FLAG2_LIGHTBAR_SETUP_CONTROL_ENABLE;
      rp->lightbar_setup = DS_OUTPUT_LIGHTBAR_SETUP_LIGHT_OUT;
      break;
    case 1:
      rp->valid_flag1 = DS_OUTPUT_VALID_FLAG1_PLAYER_INDICATOR_CONTROL_ENABLE;
      rp->player_leds = 1 | (1 << 3);
      break;
    case 2:
      rp->valid_flag1 = DS_OUTPUT_VALID_FLAG1_LIGHTBAR_CONTROL_ENABLE;
      rp->lightbar_red = 0;
      rp->lightbar_green = 255;
      rp->lightbar_blue = 0;
      rp->led_brightness = 255;
      break;
    case 3:
      TriggerFeedbackSetup(rp->RightTriggerFFB,  3, 3);
      TriggerFeedbackSetup(rp->LeftTriggerFFB,  3, 3);
      rp->valid_flag0 = VALID_FLAG0_RIGHT_TRIGGER | VALID_FLAG0_LEFT_TRIGGER;
      break;
    default:
      break;
  }
}

static void process_bt_reports(uint8_t hid_mode)
{
  struct dualsense_btout_report *brp;
  DS_OUTPUT_REPORT *rp;

  if (initial_report < 4)
  {
    brp = get_report_buffer();
    bt_out_init(brp);
    rp = &brp->com_report;

    fill_output_request(initial_report, rp);
    initial_report++;

    bt_emit_report(brp);
    return;
  }
  BtUpdateBarColor(hid_mode);
}
#endif

static uint32_t last_button;
static int pad_timer;
static int16_t left_xinc, left_yinc;
static int16_t right_xinc, right_yinc;

#if 0
extern lv_indev_data_t tp_data;

static void decode_tp(struct dualsense_input_report *rp)
{
  struct dualsense_touch_point *tp;
  uint16_t state, xpos, ypos;

  tp = rp->points;

  state = (tp->contact & 0x80)? LV_INDEV_STATE_RELEASED : LV_INDEV_STATE_PRESSED;
  xpos = (tp->x_hi << 8) | (tp->x_lo);
  ypos = (tp->y_hi << 4) | (tp->y_lo);
  xpos = xpos * 480 / DS_TOUCHPAD_WIDTH;
  ypos = ypos * 320 / DS_TOUCHPAD_HEIGHT;

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

static void decode_stick(struct dualsense_input_report *rp)
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

#if 0
  if (dcount % 50 == 0)
  {
    //printf("andgle: %f\n", atan2f((float)ix/256.0, (float)iy / 256.0));
  }
#endif

  if (ax != 0)
  {
    if (ix < 0) ax = -1;
#if 0
    if (ax != left_xinc)
      postGuiEventMessage(GUIEV_LEFT_XDIR, ax, NULL, NULL);
#endif
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
 * @brief Convert HID input report to PAD_KEY events
 */
static void DualSense_PadKey_Events(struct dualsense_input_report *rp, uint8_t hat, uint32_t vbutton)
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
  decode_tp(rp);
#endif
}

void DualSenseBtSetup(uint16_t hid_host_cid)
{
  GAMEPAD_BUFFERS *gpb = GetGamePadBuffer();

  gpb->out_toggle = 0;

  initial_report = 0;
  hid_host_send_get_report(hid_host_cid, HID_REPORT_TYPE_FEATURE, DS_FEATURE_REPORT_CALIBRATION);
}

static inline int16_t get_le16val(uint8_t *bp)
{
  int16_t val;

  val = bp[0] + (bp[1] << 8);
  return val;
}

static void process_calibdata(struct dsense_data *ds, uint8_t *dp)
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
  calibVals[3] = gyro_pitch_plus = get_le16val(dp + 7);
  calibVals[4] = gyro_pitch_minus = get_le16val(dp + 9);
  calibVals[5] = gyro_yaw_plus = get_le16val(dp + 11);
  calibVals[6] = gyro_yaw_minus = get_le16val(dp + 13);
  calibVals[7] = gyro_roll_plus = get_le16val(dp + 15);
  calibVals[8] = gyro_roll_minus = get_le16val(dp + 17);
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
   * Data values will be normalized to 1/DS_GYRO_RES_PER_DEG_S degree/s.
   */

  flNumerator = (gyro_speed_plus + gyro_speed_minus) * DS_GYRO_RES_PER_DEG_S;

  ds->gyro_calib_data[0].bias = gyro_pitch_bias;
  ds->gyro_calib_data[0].sensitivity = flNumerator / (gyro_pitch_plus - gyro_pitch_minus);

  ds->gyro_calib_data[1].bias = gyro_yaw_bias;
  ds->gyro_calib_data[1].sensitivity = flNumerator / (gyro_yaw_plus - gyro_yaw_minus);

  ds->gyro_calib_data[2].bias = gyro_roll_bias;
  ds->gyro_calib_data[2].sensitivity = flNumerator / (gyro_roll_plus - gyro_roll_minus);

  /*
   * Set accelerometer calibration and normalization parameters.
   * Data values will be normalized to 1/DS_ACC_RES_PER_G g.
   */
  range_2g = acc_x_plus - acc_x_minus;
  ds->accel_calib_data[0].bias = acc_x_plus - range_2g / 2;
  ds->accel_calib_data[0].sensitivity = 2.0f * DS_ACC_RES_PER_G / (float)range_2g;

  range_2g = acc_y_plus - acc_y_minus;
  ds->accel_calib_data[1].bias = acc_y_plus - range_2g / 2;
  ds->accel_calib_data[1].sensitivity = 2.0f * DS_ACC_RES_PER_G / (float)range_2g;

  range_2g = acc_z_plus - acc_z_minus;
  ds->accel_calib_data[2].bias = acc_z_plus - range_2g / 2;
  ds->accel_calib_data[2].sensitivity = 2.0f * DS_ACC_RES_PER_G / (float)range_2g;
}

void DualSenseProcessCalibReport(const uint8_t *bp, int len)
{
  if (len == DS_FEATURE_REPORT_CALIBRATION_SIZE)
  {
    process_calibdata(&DsData, (uint8_t *)bp);
  }
}

const struct sGamePadDriver DualSenseDriver = {
  "DualSense",
  FEATURE_STICK | FEATURE_ACCEL,
  DualSenseDecodeInputReport,
  DualSenseBtSetup,
  DualSenseProcessCalibReport,
  DualSenseBtDisconnect,
};
