#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "picogames.h"
#include "pico/multicore.h"
#include "hardware/i2c.h"
#include "apds9960.h"

struct s_gesture_data GestureData;

static int gesture_motion_;
static int gesture_ud_delta_;
static int gesture_lr_delta_;
static int gesture_state_;
static int gesture_near_count_;
static int gesture_far_count_;
static int gesture_lr_count_;
static int gesture_ud_count_;

static volatile int isr_flag;

static const uint8_t CmdReadFifo[] = { REG_GFLVL };
static const uint8_t CmdGmode[] =    { REG_GCONF4 };
static const uint8_t CmdGesture[] =  { REG_GFIFO_U };
static const uint8_t CmdReadID[] =   { REG_ID };
static const uint8_t CmdGStatus[] =  { REG_GSTATUS };

struct sInit {
  uint8_t regno;
  uint8_t regval;
};

struct sInit InitVars[] = {
  { REG_ENABLE, 0 },		/* Disable all features */
  { REG_ATIME, DEF_ATIME },
  { REG_WTIME, DEF_WTIME },
  { REG_PPULSE, DEF_PROX_PPULSE },
  { REG_POFFSET_UR, DEF_POFFSET_UR },
  { REG_POFFSET_DL, DEF_POFFSET_DL },
  { REG_CONFIG1,   DEF_CONFIG1 },
  { REG_GCONF3,  DEF_GCONF3 },
  { REG_CONTROL,   DEF_LDRIVE | DEF_PGAIN | DEF_AGAIN },
  { REG_PILT,      DEF_PILT },
  { REG_PIHT,      DEF_PIHT },
  { REG_AILTL,     DEF_AILT & 0xFF},
  { REG_AILTH,     (DEF_AILT >> 8) & 0xFF},
  { REG_PERS,      DEF_PERS },
  { REG_CONFIG2,   DEF_CONFIG2 },
  { REG_CONFIG3,   DEF_CONFIG3 },

  /* Set default values for gesture control registers */
  { REG_GPENTH,    DEF_GPENTH },
  { REG_GEXTH,     DEF_GEXTH },
  { REG_GCONF1,    DEF_GCONF1 },
  { REG_GCONF2,    GCONF2_GGAIN4X|GCONF2_GLDRIVE100|GCONF2_GWTIME28 },
  { REG_GOFFSET_U, DEF_GOFFSET },
  { REG_GOFFSET_D, DEF_GOFFSET },
  { REG_GOFFSET_L, DEF_GOFFSET },
  { REG_GOFFSET_R, DEF_GOFFSET },
  { REG_GPULSE,    DEF_GPULSE },
  { REG_CONFIG3,   DEF_CONFIG3 },
  { 0, 0 },
};

struct sInit GestureVars[] = {
  { REG_ENABLE,  0 },
  { REG_WTIME,   0xFF },
  { REG_PPULSE,  DEF_GESTURE_PPULSE },
  { REG_CONFIG2, CONFIG2_LED_BOOST100| 1 },
  { REG_GCONF4,  0x01 << 2 },	/* GFIFO_CLEAR */
  { REG_GCONF4,  0x01 << 1 },   /* GIEN */
  { REG_GCONF4,  0x03 },	/* GIEN, GMODE */
  { 0, 0 },
};

uint8_t udlr[4];
uint8_t udlr_old[4];
uint8_t udlr_peak[4];
uint8_t udlr_peak_end;
int phase_counter;
int direction;

/**
 * @brief Resets all the parameters in the gesture data member
 */
void resetGestureParameters()
{
  struct s_gesture_data *gdp = &GestureData;

  gdp->index = 0;
  gdp->total_gestures = 0;

  gesture_ud_delta_ = 0;
  gesture_lr_delta_ = 0;

  gesture_ud_count_ = 0;
  gesture_lr_count_ = 0;

  gesture_near_count_ = 0;
  gesture_far_count_ = 0;

  gesture_state_ = 0;
  gesture_motion_ = DIR_NONE;
}

static uint8_t apds_read_reg(uint8_t regno)
{
  uint8_t regval;

  i2c_write_blocking(APDS_I2C, DEV_ADDR, &regno, 1, true);
  i2c_read_blocking(APDS_I2C, DEV_ADDR, &regval, 1, false);
  return regval;
}

void apds_write_regs(struct sInit *sptr)
{
  while (sptr->regno != 0)
  {
    i2c_write_blocking(APDS_I2C, DEV_ADDR, (uint8_t *)sptr, 2, false);
    sptr++;
  }
}

void apds_set_mode(uint8_t mbits, int enable)
{
  uint8_t cmode;
  uint8_t cmd_buffer[2];

  cmd_buffer[0] = REG_ENABLE;
  i2c_write_blocking(APDS_I2C, DEV_ADDR, cmd_buffer, 1, true);
  i2c_read_blocking(APDS_I2C, DEV_ADDR, &cmode, 1, false); // read current mode

  enable &= 1;
  {
    if (enable)
    {
      cmode |= mbits;
    }
    else
    {
      cmode &= ~mbits;
    }
    cmd_buffer[1] = cmode;
    i2c_write_blocking(APDS_I2C, DEV_ADDR, cmd_buffer, 2, false);
  }
}

/* Enable gesture mode
       Set ENABLE to 0 (power off)
       Set WTIME to 0xFF
       Set AUX to LED_BOOST_300
       Enable PON, WEN, PEN, GEN in ENABLE
*/
void apds_enable_gesture_sensor()
{
  resetGestureParameters();
  apds_write_regs(GestureVars);
  apds_set_mode(ENABLE_PON, 1);
  apds_set_mode(ENABLE_WEN, 1);
  apds_set_mode(ENABLE_PEN, 1);
  apds_set_mode(ENABLE_GEN, 1);
}


void apds_reset_vars()
{
  for (int i = 0; i < 4; i++)
  {
    udlr_old[i] = 0;
  }
  udlr_peak_end = 0;
  phase_counter = 0;
}


int apds_init()
{
  int res;
  uint8_t id;

  gpio_init(APDS_INT);

  i2c_init(APDS_I2C, 100*1000);
  gpio_set_function(APDS_SCL, GPIO_FUNC_I2C);
  gpio_set_function(APDS_SDA, GPIO_FUNC_I2C);


  res = i2c_write_timeout_us(APDS_I2C, DEV_ADDR, CmdReadID, 1, true, 5*1000);
  res = i2c_read_timeout_us(APDS_I2C, DEV_ADDR, &id, 1, false, 5 * 1000);

  if (res == 1 && id == DEV_ID)
  {
    printf("APDS-9960 detected.\n");

    apds_write_regs(InitVars);
    apds_reset_vars();
    return 1;
  }
  return 0;
}

const static char *direc_string[4] = { "Up", "Down", "Left", "Right" };

int isGestureAvailable()
{
  uint8_t gstatus;

  /* Read GSTATUS register */
  i2c_write_blocking(APDS_I2C, DEV_ADDR, CmdGStatus, 1, true);
  i2c_read_blocking(APDS_I2C, DEV_ADDR, &gstatus, 1, false);

  return gstatus & 1;
}

/**
 * @brief Processes the raw gesture data to determine swipe direction
 *
 * @return True if near or far state seen. False otherwise.
 */
bool processGestureData()
{
    uint8_t u_first = 1;
    uint8_t d_first = 1;
    uint8_t l_first = 1;
    uint8_t r_first = 1;
    uint8_t u_last = 1;
    uint8_t d_last = 1;
    uint8_t l_last = 1;
    uint8_t r_last = 1;
    int ud_ratio_first;
    int lr_ratio_first;
    int ud_ratio_last;
    int lr_ratio_last;
    int ud_delta;
    int lr_delta;
    int i;

    /* If we have less than 4 total gestures, that's not enough */
    if( GestureData.total_gestures <= 4 ) {
        return false;
    }

#if 0
printf("process: %d\n", GestureData.total_gestures);
#endif
    /* Check to make sure our data isn't out of bounds */
    if( (GestureData.total_gestures <= 32) && \
        (GestureData.total_gestures > 0) ) {

        /* Find the first value in U/D/L/R above the threshold */
        for( i = 0; i < GestureData.total_gestures; i++ ) {
            if( (GestureData.u_data[i] > GESTURE_THRESHOLD_OUT) &&
                (GestureData.d_data[i] > GESTURE_THRESHOLD_OUT) ||
                (GestureData.l_data[i] > GESTURE_THRESHOLD_OUT) &&
                (GestureData.r_data[i] > GESTURE_THRESHOLD_OUT) ) {

                u_first = GestureData.u_data[i];
                d_first = GestureData.d_data[i];
                l_first = GestureData.l_data[i];
                r_first = GestureData.r_data[i];
                break;
            }
        }

        /* Find the last value in U/D/L/R above the threshold */
        for( i = GestureData.total_gestures - 1; i >= 0; i-- ) {
            if( (GestureData.u_data[i] > GESTURE_THRESHOLD_OUT) &&
                (GestureData.d_data[i] > GESTURE_THRESHOLD_OUT) ||
                (GestureData.l_data[i] > GESTURE_THRESHOLD_OUT) &&
                (GestureData.r_data[i] > GESTURE_THRESHOLD_OUT) ) {

                u_last = GestureData.u_data[i];
                d_last = GestureData.d_data[i];
                l_last = GestureData.l_data[i];
                r_last = GestureData.r_data[i];
                break;
            }
        }
    }

    /* Calculate the first vs. last ratio of up/down and left/right */
    ud_ratio_first = ((u_first - d_first) * 100) / (u_first + d_first);
    lr_ratio_first = ((l_first - r_first) * 100) / (l_first + r_first);
    ud_ratio_last = ((u_last - d_last) * 100) / (u_last + d_last);
    lr_ratio_last = ((l_last - r_last) * 100) / (l_last + r_last);

#if xDEBUG
    Serial.print(F("Last Values: "));
    Serial.print(F("U:"));
    Serial.print(u_last);
    Serial.print(F(" D:"));
    Serial.print(d_last);
    Serial.print(F(" L:"));
    Serial.print(l_last);
    Serial.print(F(" R:"));
    Serial.println(r_last);

    Serial.print(F("Ratios: "));
    Serial.print(F("UD Fi: "));
    Serial.print(ud_ratio_first);
    Serial.print(F(" UD La: "));
    Serial.print(ud_ratio_last);
    Serial.print(F(" LR Fi: "));
    Serial.print(lr_ratio_first);
    Serial.print(F(" LR La: "));
    Serial.println(lr_ratio_last);
#endif

    /* Determine the difference between the first and last ratios */
    ud_delta = ud_ratio_last - ud_ratio_first;
    lr_delta = lr_ratio_last - lr_ratio_first;

#if xDEBUG
    Serial.print("Deltas: ");
    Serial.print("UD: ");
    Serial.print(ud_delta);
    Serial.print(" LR: ");
    Serial.println(lr_delta);
#endif

    /* Accumulate the UD and LR delta values */
    gesture_ud_delta_ += ud_delta;
    gesture_lr_delta_ += lr_delta;

#if xDEBUG
    Serial.print("Accumulations: ");
    Serial.print("UD: ");
    Serial.print(gesture_ud_delta_);
    Serial.print(" LR: ");
    Serial.println(gesture_lr_delta_);
#endif

    /* Determine U/D gesture */
    if( gesture_ud_delta_ >= GESTURE_SENSITIVITY_1 ) {
        gesture_ud_count_ = 1;
    } else if( gesture_ud_delta_ <= -GESTURE_SENSITIVITY_1 ) {
        gesture_ud_count_ = -1;
    } else {
        gesture_ud_count_ = 0;
    }

    /* Determine L/R gesture */
    if( gesture_lr_delta_ >= GESTURE_SENSITIVITY_1 ) {
        gesture_lr_count_ = 1;
    } else if( gesture_lr_delta_ <= -GESTURE_SENSITIVITY_1 ) {
        gesture_lr_count_ = -1;
    } else {
        gesture_lr_count_ = 0;
    }

    /* Determine Near/Far gesture */
    if( (gesture_ud_count_ == 0) && (gesture_lr_count_ == 0) ) {
        if( (abs(ud_delta) < GESTURE_SENSITIVITY_2) && \
            (abs(lr_delta) < GESTURE_SENSITIVITY_2) ) {

            if( (ud_delta == 0) && (lr_delta == 0) ) {
                gesture_near_count_++;
            } else if( (ud_delta != 0) || (lr_delta != 0) ) {
                gesture_far_count_++;
            }

#if 0
printf("near = %d, far = %d\n", gesture_near_count_, gesture_far_count_);
#endif
            if( (gesture_near_count_ >= 10) && (gesture_far_count_ >= 2) ) {
                if( (ud_delta == 0) && (lr_delta == 0) ) {
                    gesture_state_ = NEAR_STATE;
                } else if( (ud_delta != 0) && (lr_delta != 0) ) {
                    gesture_state_ = FAR_STATE;
                }
                return true;
            }
        }
    } else {
        if( (abs(ud_delta) < GESTURE_SENSITIVITY_2) && \
            (abs(lr_delta) < GESTURE_SENSITIVITY_2) ) {

            if( (ud_delta == 0) && (lr_delta == 0) ) {
                gesture_near_count_++;
            }

            if( gesture_near_count_ >= 10 ) {
                gesture_ud_count_ = 0;
                gesture_lr_count_ = 0;
                gesture_ud_delta_ = 0;
                gesture_lr_delta_ = 0;
            }
        }
    }

#if DEBUG
    Serial.print("UD_CT: ");
    Serial.print(gesture_ud_count_);
    Serial.print(" LR_CT: ");
    Serial.print(gesture_lr_count_);
    Serial.print(" NEAR_CT: ");
    Serial.print(gesture_near_count_);
    Serial.print(" FAR_CT: ");
    Serial.println(gesture_far_count_);
    Serial.println("----------");
#endif

    return false;
}

/**
 * @brief Determines swipe direction or near/far state
 *
 * @return True if near/far event. False otherwise.
 */
bool decodeGesture()
{
    /* Return if near or far event is detected */
    if( gesture_state_ == NEAR_STATE ) {
        gesture_motion_ = DIR_NEAR;
        return true;
    } else if ( gesture_state_ == FAR_STATE ) {
        gesture_motion_ = DIR_FAR;
        return true;
    }

    /* Determine swipe direction */
    if( (gesture_ud_count_ == -1) && (gesture_lr_count_ == 0) ) {
        gesture_motion_ = DIR_UP;
    } else if( (gesture_ud_count_ == 1) && (gesture_lr_count_ == 0) ) {
        gesture_motion_ = DIR_DOWN;
    } else if( (gesture_ud_count_ == 0) && (gesture_lr_count_ == 1) ) {
        gesture_motion_ = DIR_RIGHT;
    } else if( (gesture_ud_count_ == 0) && (gesture_lr_count_ == -1) ) {
        gesture_motion_ = DIR_LEFT;
    } else if( (gesture_ud_count_ == -1) && (gesture_lr_count_ == 1) ) {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) {
            gesture_motion_ = DIR_UP;
        } else {
            gesture_motion_ = DIR_RIGHT;
        }
    } else if( (gesture_ud_count_ == 1) && (gesture_lr_count_ == -1) ) {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) {
            gesture_motion_ = DIR_DOWN;
        } else {
            gesture_motion_ = DIR_LEFT;
        }
    } else if( (gesture_ud_count_ == -1) && (gesture_lr_count_ == -1) ) {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) {
            gesture_motion_ = DIR_UP;
        } else {
            gesture_motion_ = DIR_LEFT;
        }
    } else if( (gesture_ud_count_ == 1) && (gesture_lr_count_ == 1) ) {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) {
            gesture_motion_ = DIR_DOWN;
        } else {
            gesture_motion_ = DIR_RIGHT;
        }
    } else {
        return false;
    }

    return true;
}

int readGesture()
{
  uint8_t fifo_level = 0;
  uint8_t fifo_data[128];
  uint8_t gstatus;
  int bytes_read = 0;
  int motion;
  int i;
  struct s_gesture_data *gdp = &GestureData;

printf("%s: 0\n", __FUNCTION__);
  while (1)
  {
    if (isGestureAvailable())
    {
      sleep_ms(FIFO_PAUSE_TIME);

      fifo_level = apds_read_reg(REG_GFLVL);
      if (fifo_level > 0)
      {
        i2c_write_blocking(APDS_I2C, DEV_ADDR, CmdGesture, 1, true);
        i2c_read_blocking(APDS_I2C, DEV_ADDR, fifo_data, 4 * fifo_level, false);

        int n = 0;
        for (i = 0; i < fifo_level; i++)
        {
          gdp->u_data[gdp->index] = fifo_data[n++];
          gdp->d_data[gdp->index] = fifo_data[n++];
          gdp->l_data[gdp->index] = fifo_data[n++];
          gdp->r_data[gdp->index] = fifo_data[n++];
          gdp->index++;
          gdp->total_gestures++;
        }

        if (processGestureData())
        {
          decodeGesture();
#if 0
printf("decoded: %d\n", gesture_motion_);
#endif
          gdp->index = 0;
          gdp->total_gestures = 0;
        }
        else if (gdp->total_gestures > 25)
        {
          gdp->index = 0;
          gdp->total_gestures = 0;
        }
      }
    } else {
#if 0
      /* Determine best guessed gesture and clean up */
      sleep_ms(FIFO_PAUSE_TIME);
#endif
      decodeGesture();
      motion = gesture_motion_;
      resetGestureParameters();
printf("guess %d\n", motion);
      return motion;
    }
  }
}

extern void post_gesture_event(int evcode);

void handle_Gesture()
{
  if (isGestureAvailable())
  {
    int event = readGesture();

    if (event > DIR_NONE && event < DIR_ALL)
    {
      post_gesture_event(event);
    }
  }
}

void gpio_callback(uint gpio, uint32_t events)
{
  gpio_set_irq_enabled(APDS_INT, GPIO_IRQ_EDGE_FALL, false);
  isr_flag = 1;
}

void apds_run_loop(void)
{
  gpio_set_irq_enabled_with_callback(APDS_INT, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
  apds_enable_gesture_sensor();

  sleep_ms(FIFO_PAUSE_TIME);

  while (1)
  {
    __wfi();
    if (isr_flag)
    {
printf("%s 3\n", __FUNCTION__);
      handle_Gesture();
      isr_flag = 0;
      gpio_set_irq_enabled(APDS_INT, GPIO_IRQ_EDGE_FALL, true);
      sleep_ms(FIFO_PAUSE_TIME);
    }
  }
}
