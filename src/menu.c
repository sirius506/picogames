#include <pico/stdlib.h>
#include <pico/time.h>
#include "pico/critical_section.h"
#include "lvgl.h"
#include "picogames.h"
#include "XPT2046.h"
#include "btapi.h"
#ifdef DEBUG
#include <stdio.h>
#endif

#define	DBUF_LINES	60
#define	DBUF_SIZE	(240*2*DBUF_LINES)

#define	BLINK_PERIOD	250

static uint8_t disp_buffer1[DBUF_SIZE];
static uint8_t swap_buffer[DBUF_SIZE];

static struct repeating_timer blink_timer;
static lv_obj_t *button;

extern void set_hid_mode(uint8_t mode);

extern void inv_main(void);
extern void hakomusu_main(void);
extern void pacman_main(void);
extern void tetris_main(void);
extern void peg_main(void);

/* Declare Bluetooth button images */
LV_IMG_DECLARE(bluetooth_black)
LV_IMG_DECLARE(bluetooth_scan_black)
LV_IMG_DECLARE(bluetooth_scan_blue)

static void send_cmd_cb(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, const uint8_t *param, size_t param_size)
{
  LV_UNUSED(disp);

  LCD_WriteComm2((uint8_t *)cmd, cmd_size, (uint8_t *)param, param_size);
}

static void send_color_cb(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, uint8_t *param, size_t param_size)
{
  LV_UNUSED(disp);
  uint8_t *dst = swap_buffer;

  for (int i = 0; i < param_size; i+= 2)
  {
    *dst++ = param[1];
    *dst++ = param[0];
    param += 2;
  }
  LCD_WriteData3((uint8_t *)cmd, cmd_size, (uint8_t *)swap_buffer, param_size);

  lv_display_flush_ready(disp);
}

/*
 * Timer callback function to increment the LVGL tick
 */
bool lvgl_tick_callback(struct repeating_timer *t) {
  // Tell LVGL that 1 millisecond has elapsed
  lv_tick_inc(1);
  return true;
}

static uint32_t get_millis(void)
{
  critical_section_t crit_sec;
  critical_section_enter_blocking(&crit_sec);
  uint32_t ms = to_ms_since_boot(get_absolute_time());
  critical_section_exit(&crit_sec);
  return(ms);
}

static struct repeating_timer tick_timer;

/*
 *  Create a repeating timer that calls lvgl_tick_callback every 1ms
 */
void setup_lvgl_tick()
{

  lv_tick_set_cb(get_millis);

  add_repeating_timer_ms( 1, lvgl_tick_callback, NULL, &tick_timer);
}

static int blink_status;

bool blink_timer_callback(struct repeating_timer *t)
{
  const lv_image_dsc_t *next_image;

  blink_status ^= 1;
  next_image = blink_status?  &bluetooth_scan_blue : &bluetooth_scan_black;
  lv_imagebutton_set_src(button, LV_IMAGEBUTTON_STATE_RELEASED, NULL, next_image, NULL);
#if 0
  if (blink_status)
    lv_imagebutton_set_src(button, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &bluetooth_scan_blue, NULL);
  else
    lv_imagebutton_set_src(button, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &bluetooth_scan_black, NULL);
#endif
  return true;
}

static void button_cb(lv_event_t *e)
{
#ifdef DEBUG
  printf("Button clicked!\n");
#endif
  blink_status = 0;

  if (get_btstack_state() & BT_STATE_SCAN)
  {
     post_btreq(BB_SCAN);
     cancel_repeating_timer(&blink_timer);
     lv_imagebutton_set_src(button, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &bluetooth_black, NULL);
  }
  else
  {
     add_repeating_timer_ms(BLINK_PERIOD, blink_timer_callback, NULL, &blink_timer);
     post_btreq(BB_SCAN);
  }
}

typedef struct {
  char *name;
  lv_obj_t *button;
  void (*game)(void);
  int ypos;
} GAMETAB;

GAMETAB GameTable[5] = {
  { "Invader", NULL, inv_main, 75 },
  { "Pacman",  NULL, pacman_main, 125 },
  { "Tetris",  NULL, tetris_main, 175 },
  { "Peg Solitaire", NULL, peg_main, 225 },
  { "Hakoiri Musume", NULL, hakomusu_main, 275 },
};

static void (*sel_game)(void);

void menu_select_handler(lv_event_t *e)
{
  int index = (int) lv_event_get_user_data(e);

#ifdef DEBUG
  printf("%s\n", GameTable[index].name);
#endif
  sel_game = GameTable[index].game;
}

static void keypad_read(lv_indev_t *dev, lv_indev_data_t *data)
{
  UNUSED(dev);
  PADEVENT *evp;

  evp = read_pad_event();
  if (evp)
  {
    if (evp->type == PAD_KEY_PRESS)
    {
      data->key = evp->key_code;
      data->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
      data->key = evp->key_code;
      data->state = LV_INDEV_STATE_RELEASED;
    }
  }
}

int run_menu()
{
  lv_display_t *disp;
  lv_indev_t *indev;
  lv_indev_t *keydev;
  const lv_font_t *tfont;

  sleep_ms(300);

  lv_init();
  setup_lvgl_tick();

  disp = lv_ili9341_create(240, 320, 0, send_cmd_cb, send_color_cb);
  lv_display_set_buffers(disp, disp_buffer1, NULL, DBUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, xpt2046_read);

  keydev = lv_indev_create();
  lv_indev_set_type(keydev, LV_INDEV_TYPE_KEYPAD);
  lv_indev_set_read_cb(keydev, keypad_read);

  lv_obj_t *label;

  tfont = &lv_font_montserrat_16;
  label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "Pico Games");
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(label, tfont, 0);
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 40);

  label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "Please connect Game Controller\nor\nStart pairing");
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, -30);

  button = lv_imagebutton_create(lv_screen_active());
  lv_imagebutton_set_src(button, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &bluetooth_black, NULL);
  lv_obj_align(button, LV_ALIGN_CENTER, 0, 60);
  lv_obj_set_width(button, LV_SIZE_CONTENT);
  lv_obj_add_event_cb(button, button_cb, LV_EVENT_CLICKED, NULL);

  /* Wait for Gamepad connect event */

  while (1)
  {
    lv_timer_handler();
    lv_sleep_ms(100);
    if (check_pad_connect())
      break;
  }

  cancel_repeating_timer(&blink_timer);

  /* Create game menu screen. */

  lv_obj_t *scr;
  lv_obj_t *btn;
  lv_group_t *g;
  GAMETAB *gp = GameTable;

  scr = lv_obj_create(NULL);
  g = lv_group_create();

  label = lv_label_create(scr);
  lv_label_set_text(label, "Select game");
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(label, tfont, 0);
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 30);


  for (int i = 0; i < 5; i++)
  {
    btn = lv_button_create(scr);
    lv_group_add_obj(g, btn);
    gp->button = btn;
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, gp->ypos);
    lv_obj_add_event_cb(btn, menu_select_handler, LV_EVENT_CLICKED, (void *) i);

    label = lv_label_create(btn);
    lv_label_set_text(label, gp->name);
    lv_obj_center(label);
    gp++;
  }
  lv_screen_load(scr);

  lv_group_focus_obj(GameTable[0].button);
  lv_indev_set_group(keydev, g);

  while (1)
  {
    lv_timer_handler();
    lv_sleep_ms(100);
    if (sel_game)
     break;
  }

  cancel_repeating_timer(&tick_timer);

  set_hid_mode(HID_MODE_GAME);

  (*sel_game)();

  return 0;
}
