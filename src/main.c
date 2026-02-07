/*
 * Pico Games
 *
 * Collection of Raspberry Pi Pico games
 * developped by KenKenMkIISR (https:github.com/KenKenMkIISR).
 *
 * Here, five games have ported to Pico W to support wireless game controllers,
 * bluetooth pairing and game selection GUI has added.
 */
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "btapi.h"
#include "picogames.h"

extern int btstack_main(int argc, const char *argv[]);

queue_t padevent_queue;

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

void post_padevent(PADKEY_EVENT *padevent)
{
  PADEVENT event;

  if (!queue_is_full(&padevent_queue))
  {
    event.type = padevent->type;
    if (padevent->type == PAD_KEY_VBMASK)
    {
      event.key_code = 0;
      event.vmask = padevent->vmask;
      event.ptr = NULL;
    }
    else
    {
      event.key_code = padevent->lvkey;
      event.vmask = 0;
      event.ptr = NULL;
    }
    queue_try_add(&padevent_queue, &event);
  }
}

void post_vkeymask(uint32_t mask)
{
  PADEVENT event;

  if (!queue_is_full(&padevent_queue))
  {
    event.type = PAD_KEY_VBMASK;
    event.key_code = 0;
    event.vmask = mask;
    event.ptr = NULL;

    queue_try_add(&padevent_queue, &event);
  }
}

static PADEVENT pevent;

int check_pad_connect()
{
  if (queue_is_empty(&padevent_queue))
    return 0;
  queue_remove_blocking(&padevent_queue, &pevent);
  if (pevent.type == PAD_CONNECT)
    return 1;
  return 0;
}

PADEVENT *read_pad_event()
{
  PADEVENT *evp = NULL;

  if (queue_is_empty(&padevent_queue))
    return NULL;
  queue_remove_blocking(&padevent_queue, &pevent);

  switch (pevent.type)
  {
  case PAD_DISCONNECT:
    watchdog_enable(1, 1);
    break;
  case PAD_KEY_PRESS:
  case PAD_KEY_RELEASE:
    evp = &pevent;
    break;
  default:
    break;
  }
  return evp;
}

int64_t alarm_callback(alarm_id_t id, void *user_data)
{
  post_btreq(BB_CONN);
  return 0;
}

uint32_t get_pad_vmask()
{
  static uint32_t old_mask;
  static alarm_id_t aid;

  if (queue_is_empty(&padevent_queue))
    return old_mask;
  queue_remove_blocking(&padevent_queue, &pevent);

  if (aid)
  {
    cancel_alarm(aid);
    aid = 0;
  }

  if (pevent.type == PAD_DISCONNECT)
  {
    watchdog_enable(2, 1);
    return old_mask;
  }
  else if (pevent.type == PAD_KEY_VBMASK)
  {
    if (pevent.vmask == VBMASK_SHARE || pevent.vmask == VBMASK_OPTION)
    {
      aid = add_alarm_in_ms(2000, alarm_callback, NULL, false);
      return old_mask;
    }
  }
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
  extern int run_menu();

  PADEVENT *evp;
  board_init();

  int sel_game = run_menu();

#if 0
  init_graphic();

  LCD_WriteComm(0x37); //画面中央にするためスクロール設定
  LCD_WriteData2(272);

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
#endif
}
