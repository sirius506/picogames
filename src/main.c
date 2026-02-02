#include "btstack_run_loop.h"
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "btstack.h"
#include "btapi.h"
#include "picogames.h"
#include "graphlib.h"
#include "gamepad.h"
#include "LCDdriver.h"

extern int btstack_main(int argc, const char *argv[]);

queue_t padevent_queue;

static volatile LED_MODE led_mode;
static volatile bool led_state;
static struct repeating_timer led_timer;
static volatile alarm_id_t btn_timer;

#define	BLINK_PERIOD	250

btstack_packet_callback_registration_t hci_event_callback_registration;

extern void inv_main(void);
extern void hakomusu_main(void);
extern void pacman_main(void);
extern void tetris_main(void);
extern void peg_main(void);
void game_main(void);

void game_core_init()
{
  queue_init(&padevent_queue, sizeof(PADEVENT), 4);
  multicore_reset_core1();

  multicore_launch_core1(game_main);
}

void list_link_keys(void)
{
  bd_addr_t addr;
  link_key_t link_key;
  link_key_type_t type;
  btstack_link_key_iterator_t it;

  int ok = gap_link_key_iterator_init(&it);
  if (!ok)
  {
    printf("Link key iterator not implemented.\n");
    return;
  }
  while (gap_link_key_iterator_get_next(&it, addr, link_key, &type))
  {
    printf("%s -- type %u, key: ", bd_addr_to_str(addr), (int) type);
    printf_hexdump(link_key, 16);
  }
  printf(".\n");
  gap_link_key_iterator_done(&it);
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
  UNUSED(size);
  UNUSED(channel);
  bd_addr_t local_addr;

  if (packet_type != HCI_EVENT_PACKET) return;

  switch (hci_event_packet_get_type(packet))
  {
  case BTSTACK_EVENT_STATE:
    if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
    gap_local_bd_addr(local_addr);
    printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
    break;
  default:
    break;
  }
}

int main(int argc, const char **argv)
{
  stdio_init_all();

  if (cyw43_arch_init())
  {
    printf("failed to initialize cyw43_arch\n");
    return -1;
  }

  game_core_init();

  hci_event_callback_registration.callback = &packet_handler;
  hci_add_event_handler(&hci_event_callback_registration);

  btstack_main(argc, argv);
  btstack_run_loop_execute();
}

void post_event(uint16_t type, uint16_t code, void *ptr)
{
  PADEVENT event;

  if (!queue_is_full(&padevent_queue))
  {
    event.type = type;
    event.key_code = code;
    event.ptr = ptr;

    queue_try_add(&padevent_queue, &event);
  }
}

void post_vkeymask(uint32_t mask)
{
  PADEVENT event;

  if (!queue_is_full(&padevent_queue))
  {
    event.type = PAD_VBMASK;
    event.key_code = 0;
    event.vmask = mask;
    event.ptr = NULL;

    queue_try_add(&padevent_queue, &event);
  }
}

bool led_timer_callback(struct repeating_timer *t)
{
  if (led_mode == LED_BLINK)
  {
    if (led_state)
    {
      led_state = false;
    }
    else
    {
      led_state = true;
    }
    // !!
  }
}

void pico_set_led(LED_MODE new_mode)
{
  if (new_mode != led_mode)
  {
    if (led_mode == LED_BLINK)
      cancel_repeating_timer(&led_timer);

    switch (new_mode)
    {
    case LED_OFF:
     led_state = false;
     break;
    case LED_ON:
     led_state = true;
     break;
    case LED_BLINK:
     led_state = true;
     add_repeating_timer_ms(BLINK_PERIOD, led_timer_callback, NULL, &led_timer);
     break;
   } 
   led_mode = new_mode;
   // !!! Set LED
 }
}

static PADEVENT pevent;

PADEVENT *get_pad_event()
{
  if (queue_is_empty(&padevent_queue))
    return NULL;

  queue_remove_blocking(&padevent_queue, &pevent);
  if (pevent.type == PAD_DISCONNECT)
    watchdog_enable(1, 1);

  return &pevent;
}

uint32_t get_pad_vmask()
{
  static uint32_t old_mask;
  if (queue_is_empty(&padevent_queue))
    return old_mask;
  queue_remove_blocking(&padevent_queue, &pevent);

  if (pevent.type == PAD_DISCONNECT)
    watchdog_enable(1, 1);
  old_mask = pevent.vmask;
  return old_mask;
}

uint16_t get_pad_press()
{
  if (queue_is_empty(&padevent_queue))
    return 0;
  queue_remove_blocking(&padevent_queue, &pevent);
  if (pevent.type == PAD_KEY_PRESS)
    return pevent.key_code;
  return 0;
}

void wait60thsec(unsigned short n){
	// 60分のn秒ウェイト
	uint64_t t=to_us_since_boot(get_absolute_time())%16667;
	sleep_us(16667*n-t);
}

void game_main()
{
  PADEVENT *evp;
  board_init();

  LCD_WriteComm(0x37); //画面中央にするためスクロール設定
  LCD_WriteData2(272);

  printstr(64,80,7,0,"Pico Games");
  printstr(48,110,7,0,"Connect Gamepad");
  while(1) {
    evp = get_pad_event();
    if (evp)
    {
       if (evp->type == PAD_CONNECT)
         break;
    }
  }

  clearscreen();

  printstr(64,80,7,0,"Select Game");
  printstr(48,110,7,0,"Up:    Invader");
  printstr(48,135,7,0,"Down:  Peg-Solitaire");
  printstr(48,160,7,0,"Left:  Pacman");
  printstr(48,185,7,0,"Right: Tetris");

  uint32_t vmask;

  while(1) {
    vmask = get_pad_vmask();
    if (vmask & VBMASK_UP)
      inv_main();
    else if (vmask & VBMASK_DOWN)
      peg_main();
    else if (vmask & VBMASK_LEFT)
      pacman_main();
    else if (vmask & VBMASK_RIGHT)
      tetris_main();
  }
}
