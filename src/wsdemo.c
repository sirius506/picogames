#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "pico/float.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "apds9960.h"
#include "lvgl.h"

#define PIf       3.14159265358979323846f

#define	IS_RGBW	false
#define	NUM_PIXELS 50
#define	LOOP_COUNT	1000

#define WS2812_PIN 20

#define	FEATURE_AUTO	0x80
#define	MODE_IDLE	0x00
#define	MODE_RUN	0x01

static uint16_t _rand16seed;

typedef struct {
  PIO  pio;
  uint sm;
  uint len;
  uint t;
  int ltime;
  int rtime;
  uint32_t lcolor;
  uint32_t rcolor;
  int hue;
  uint8_t  mode;
  int  loop;
  int  rand_time;
} PATT_PARAM;

PATT_PARAM PatternParameter;


uint32_t pixel_buffer[NUM_PIXELS];

queue_t gestevent_queue;

uint32_t get_boot_time()
{
  return to_ms_since_boot(get_absolute_time());
}

// fast 8-bit random number generator shamelessly borrowed from FastLED

uint8_t random8()
{
  _rand16seed = (_rand16seed * 2053) + 13849;
  return (uint8_t)((_rand16seed + (_rand16seed >> 8)) & 0xFF);
}

uint8_t random8_lim(uint8_t lim) {
  uint8_t r = random8();
  r = ((uint16_t)r * lim) >> 8;
  return r;
}

uint16_t random16() {
  return (uint16_t)random8() * 256 + random8();
}

// note random16_lim(lim) generates numbers in the range 0 to (lim - 1)
uint16_t random16_lim(uint16_t lim) {
  uint16_t r = random16();
  r = ((uint32_t)r * lim) >> 16;
  return r;
}

static uint32_t hv_to_rgb(int hue, int v)
{
  int max, min;
  uint32_t red, green, blue;
  uint32_t rgb;

  max = v;
  min = 0;

  if (hue <= 60)
  {
    red = max;
    green = hue * max / 60 + min;
    if (green > 255) green = 255;
    blue = min;
  }
  else if (hue > 60 && hue <= 120)
  {
    red = (120 - hue) * max / 60 + min;
    if (red > 255) red = 255;
    green = max;
    blue = min;
  }
  else if (hue > 120 && hue <= 180)
  {
    red = min;
    green = max;
    blue = (hue - 120) * max / 60 + min;
    if (blue > 255) blue = 255;
  }
  else if (hue > 180 && hue <= 240)
  {
    red = min;
    green = (240 - hue) * max / 60 + min;
    if (green > 255) green = 255;
    blue = max;
  }
  else if (hue > 240 && hue <= 300)
  {
    red = (hue - 240) * max / 60 + min;
    if (red > 255) red = 255;
    green = min;
    blue = max;
  }
  else
  {
    red = max;
    green = min;
    blue = (360 - hue) * max / 60 + min;
    if (blue > 255) blue = 255;
  }
  rgb = (red << 8) | (green << 16) | blue;
  return rgb;
}

static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb)
{
  pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
  return
        ((uint32_t) (r) << 8) |
        ((uint32_t) (g) << 16) |
        (uint32_t) (b);
}

static inline uint32_t urgbw_u32(uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
  return
        ((uint32_t) (r) << 8) |
        ((uint32_t) (g) << 16) |
        ((uint32_t) (w) << 24) |
        (uint32_t) (b);
}

void pattern_snakes(PATT_PARAM *pp, uint t)
{
  for (uint i = 0; i < pp->len; i++)
  {
    uint x = (i + (t >> 1)) % 64;
    if (x < 10)
      put_pixel(pp->pio, pp->sm, urgb_u32(0xff, 0, 0));
    else if (x >= 15 && x < 25)
      put_pixel(pp->pio, pp->sm, urgb_u32(0, 0xff, 0));
    else if (x >= 30 && x < 40)
      put_pixel(pp->pio, pp->sm, urgb_u32(0, 0, 0xff));
    else
      put_pixel(pp->pio, pp->sm, 0);
  }
}

void pattern_random(PATT_PARAM *pp, uint t)
{
  if (t % 8)
    return;
  for (uint i = 0; i < pp->len; ++i)
  {
    put_pixel(pp->pio, pp->sm, rand());
  }   
}

void pattern_sparkle(PATT_PARAM *pp, uint t)
{
  if (t % 8)
    return;
  for (uint i = 0; i < pp->len; ++i)
  {
    put_pixel(pp->pio, pp->sm, rand() % 16? 0 : 0xffffffff);
  }   
}

void pattern_greys(PATT_PARAM *pp, uint t)
{
  uint max = 100;
  t %= max;
  for (uint i = 0; i < pp->len; ++i)
  {
    put_pixel(pp->pio, pp->sm, t * 0x10101);
    if (++t >= max) t = 0;
  }   
}

uint32_t color_wheel(uint8_t pos)
{
  pos = 255 - pos;
  if (pos < 85) {
   return ((uint32_t)(255 - pos * 3) << 16) | (uint32_t)(0) << 8 | (pos * 3);
  } else if (pos < 170) {
   pos -=  85;
   return ((uint32_t)(0) << 16) | ((uint32_t)(pos * 3) << 8) | (255 - pos * 3);
 } else {
   pos -= 170;
   return ((uint32_t)(pos * 3) << 16) | ((uint32_t)(255 - pos * 3) << 8) | (0);
 }
}

void pattern_rainbow(PATT_PARAM *pp, uint t)
{
  uint32_t color = color_wheel(t & 0xff);

  for (uint i = 0; i < pp->len; ++i)
  {
    put_pixel(pp->pio, pp->sm, color);
  }
}

void init_huecircle(PATT_PARAM *pp)
{
  if ((pp->mode & FEATURE_AUTO) && (pp->loop == LOOP_COUNT))
  {
    pp->hue = 0;
    pp->lcolor = hv_to_rgb(pp->hue, 200);
    pp->rand_time = 100;
  }
}

void pattern_huecircle(PATT_PARAM *pp, uint t)
{
  uint32_t color;

  if ((pp->mode & FEATURE_AUTO) && (pp->loop == LOOP_COUNT))
  {
    if (pp->rand_time > 0)
    {
      pp->rand_time--;
      if (pp->rand_time == 0)
      {
         pp->hue += random8_lim(180);
         pp->hue %= 360;
         pp->rand_time = random8_lim(200);
         if (pp->rand_time < 50) pp->rand_time += 50;
         pp->lcolor = hv_to_rgb(pp->hue, random8_lim(200));
      }
    }
    color = pp->lcolor;
  }
  else
  {
#if 0
    color = stick_to_rgb();
#else
         pp->hue += random8_lim(180);
         pp->hue %= 360;
         pp->rand_time = random8_lim(200);
         if (pp->rand_time < 50) pp->rand_time += 50;
         pp->lcolor = hv_to_rgb(pp->hue, random8_lim(200));
    color = pp->lcolor;
#endif
  }

  for (uint i = 0; i < pp->len; ++i)
  {
    put_pixel(pp->pio, pp->sm, color);
  }
}

void init_shoot(PATT_PARAM *pp)
{
  if ((pp->mode & FEATURE_AUTO) && (pp->loop == LOOP_COUNT))
  {
    pp->rand_time = random8_lim(100)+1;
    pp->rtime = 0;
    pp->rcolor = rand();
  }
}

void pattern_shoot(PATT_PARAM *pp, uint t)
{
  int flag = 0;

  if (pp->mode & FEATURE_AUTO)
  {
    if (pp->rand_time > 0)
    {
      pp->rand_time--;
      if (pp->rand_time == 0)
      {
        pp->rand_time = random8_lim(100) + 1;
        if (pp->rand_time & 4)
        {
          pp->ltime = NUM_PIXELS - 1;
          pp->hue = 0;
        }
        else
        {
          pp->rtime = 0;
          pp->hue = 0;
        }
      }
    }
  }

  for (uint i = 0; i < pp->len; ++i)
  {
    if ((i == pp->ltime) || (i == pp->ltime + 1))
    {
      put_pixel(pp->pio, pp->sm, hv_to_rgb(pp->hue, 200));
      flag |= 1;
    }
    else if ((i == pp->rtime) || (i == pp->rtime + 1))
    {
      put_pixel(pp->pio, pp->sm, hv_to_rgb(pp->hue, 200));
      flag |= 2;
    }
    else
      put_pixel(pp->pio, pp->sm, 0x0);
  }
  if (flag & 1)
      pp->ltime--;
  if (flag & 2)
      pp->rtime++;
  pp->hue += 12;
  pp->hue %= 360;

}

inline void setPixelColor(int pos, uint32_t color)
{
  if (pos >= 0 && pos < NUM_PIXELS)
    pixel_buffer[pos] = color;
}

void init_rainbow_fireworks(PATT_PARAM *pp)
{
  for (int i = 0; i < pp->len; i++)
  {
    uint32_t color = pixel_buffer[i];
    color = (color >> 1) & 0x7F7F7F7F;   // fade all pixels
    pixel_buffer[i] = color;

   // search for the fading red pixels, and create the appropriate neighbhoring pixels
   if (color == 0x007F00) {
     setPixelColor(i-1, 0x7FFF00);	// orange
     setPixelColor(i+1, 0x7FFF00);
   } else if (color == 0x002F00) {
     setPixelColor(i-2, 0xFFFF00);	// yellow
     setPixelColor(i+2, 0xFFFF00);
   } else if (color == 0x001F00) {
     setPixelColor(i-3, 0xFF0000);	// green
     setPixelColor(i+3, 0xFF0000);
   } else if (color == 0x000F00) {
     setPixelColor(i-4, 0x0000FF);	// blue
     setPixelColor(i+4, 0x0000FF);
   } else if (color == 0x000700) {
     setPixelColor(i-5, 0x004B82);	// indigo
     setPixelColor(i+5, 0x004B82);
   } else if (color == 0x000300) {
     setPixelColor(i-6, 0x0094D3);	// violet
     setPixelColor(i+6, 0x0094D3);
   }
  }
  // occasionarry create a random red pixel
  if (random8_lim(4) == 0) {
    uint16_t rand16 = random16_lim(pp->len - 12);
    uint16_t index = 6 + rand16;
    pixel_buffer[index] = 0x00FF00;
  }
}

void pattern_rainbow_fireworks(PATT_PARAM *pp, uint t)
{
  uint32_t pval;

  for (int i = 0; i < pp->len; i++)
  {
      pval = pixel_buffer[i];
      put_pixel(pp->pio, pp->sm, pval);
  }
}

typedef void (*initfunc)(PATT_PARAM *pp);
typedef void (*pattern)(PATT_PARAM *pp, uint t);

const struct {
  initfunc init;
  pattern pat;
  const char *name;
} pattern_table[] = {
#if 0
  { init_shoot, pattern_shoot,     "Shoot" },
  { init_huecircle, pattern_huecircle, "Hue circle" },
#endif
  { NULL, pattern_snakes,    "Snakes!" },
  { NULL, pattern_random,    "Random data" },
  { NULL, pattern_sparkle,   "Sparkles" },
  { NULL, pattern_greys,     "Greys" },
  { NULL, pattern_rainbow,   "Rainbow" },
  { init_rainbow_fireworks, pattern_rainbow_fireworks, "Rainbow Fireworks" },
};

#if 0
#define	NUM_PAT	8
#else
#define	NUM_PAT	6
#endif

void post_gesture_event(int evcode)
{
  GESTEVENT event;

  if (!queue_is_full(&gestevent_queue))
  {
    event = evcode;

    queue_try_add(&gestevent_queue, &event);
  }
}

#define	NUM_DINDEX	7
#define	DEF_DINDEX	3
	
int wsdelay[NUM_DINDEX] = { 4, 6, 8, 10, 20, 30, 50};

void wsdemo_main()
{
  GESTEVENT event;
  PATT_PARAM *pp = &PatternParameter;

  PIO pio;
  uint sm;
  uint offset;
  uint32_t ctime, ltime;
  lv_obj_t *label;
  const lv_font_t *tfont;

printf("wsmain started.\n");
  pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, WS2812_PIN, 1, true);

  queue_init(&gestevent_queue, sizeof(GESTEVENT), 4);

  ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

  pp->pio = pio;
#if 1
  pp->mode = MODE_IDLE;
#else
  pp->mode = MODE_RUN | FEATURE_AUTO;
#endif
  pp->sm = sm;
  pp->len = NUM_PIXELS;
  pp->loop = LOOP_COUNT;

  int t = 0;
  int pat = 0;
  int dir = 1;
  int new = 1;
  int dindex = DEF_DINDEX;

  /* Make sure to turn off all LEDs */
  for (int i = 0; i <= NUM_PIXELS; i++)
  {
    put_pixel(pp->pio, pp->sm, urgb_u32(0, 0, 0));
  }


  ctime = ltime = 0;
  tfont = &lv_font_montserrat_16;

  label = lv_label_create(lv_screen_active());
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(label, tfont, 0);
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 100);

  while (1)
  {
    if (pp->mode & MODE_RUN)
    {
      if (new)
      {
        printf("%s %s\n", pattern_table[pat].name, (dir == 1)? "(forward)" : "(backward)");
        lv_label_set_text_static(label, pattern_table[pat].name);
        new = 0;
        lv_timer_handler();
      }

      do
      {
        if (pattern_table[pat].init)
        {
          pattern_table[pat].init(pp);
printf("pattern init done.\n");
        }
        pattern_table[pat].pat(pp, t);
//printf("%s: 1\n", __FUNCTION__);

#if 0
        sleep_ms(wsdelay[dindex]);
#else
        lv_timer_handler();
        sleep_ms(wsdelay[dindex]);
#endif

        t += dir;
//printf("%s: 2\n", __FUNCTION__);
        if (pp->mode & FEATURE_AUTO)
        {
          if (pp->loop > 0)
          {
            pp->loop--;
            if (pp->loop == 0)
            {
              pat++;
              if (pat >= NUM_PAT)
                pat = 0;
              dindex = DEF_DINDEX;
              pp->loop = LOOP_COUNT;
              printf("%s %s\n", pattern_table[pat].name, (dir == 1)? "(forward)" : "(backward)");
              lv_label_set_text_static(label, pattern_table[pat].name);
              lv_timer_handler();
            }
          }
        }
//printf("%s: 4\n", __FUNCTION__);
      }
      while (queue_is_empty(&gestevent_queue));
    }
    else
    {
      while (queue_is_empty(&gestevent_queue))
      {
        lv_timer_handler();
        sleep_ms(5);
      }
    }
printf("%5: 2\n", __FUNCTION__);

    queue_remove_blocking(&gestevent_queue, &event);
    ctime = get_boot_time();

    if (ctime - ltime < 500)
      continue;

printf("%6: 2\n", __FUNCTION__);
    switch (event)
    {
    case DIR_NEAR:
      dindex = DEF_DINDEX;
      pp->mode = MODE_RUN;
      printf("Near\n");
      new = 1;
      break;
    case DIR_FAR:
      printf("Far\n");
      pp->mode = MODE_IDLE;
      for (int i = 0; i <= NUM_PIXELS; i++)
      {
        put_pixel(pp->pio, pp->sm, urgb_u32(0, 0, 0));
      }
      break;
    case DIR_UP:
      if (pp->mode & MODE_RUN)
      {
      printf("Up\n");
        pat++;
        if (pat >= NUM_PAT)
          pat = 0;
        dindex = DEF_DINDEX;
        new = 1;
      }
      break;
    case DIR_DOWN:
      if (pp->mode & MODE_RUN)
      {
      printf("Down\n");
        pat--;
        if (pat < 0) pat = NUM_PAT-1;
        dindex = DEF_DINDEX;
        new = 1;
      }
      break;
    case DIR_RIGHT:
      if (pp->mode & MODE_RUN)
      {
      printf("Right\n");
        if (dindex < NUM_DINDEX - 1)
          dindex++;
        new = 0;
      }
      break;
    case DIR_LEFT:
      if (pp->mode & MODE_RUN)
      {
      printf("Left\n");
        if (dindex > 0) dindex--;
        new = 0;
      }
      break;
    default:
      break;
    }
#if 0
    sleep_ms(10);
#else
    lv_timer_handler();
    sleep_ms(10);
#endif
  }
}
