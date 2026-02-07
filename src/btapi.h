#ifndef _BTAPI_H_
#define	_BTAPI_H_
#include "btstack.h"

#define	BTREQ_DISC	1
#define	BTREQ_SCAN	2
#define	BTREQ_CLEAR	3

#define	BT_STATE_INIT	0
#define	BT_STATE_SCAN	1
#define	BT_STATE_HID_CONNECT	2
#define	BT_STATE_HID_CLOSING	4
#define	BT_STATE_HID_MASK	0x06

enum DEVICE_STATE {
  REMOTE_NAME_INIT, REMOTE_NAME_REQUEST, REMOTE_NAME_INQUIRED, REMOTE_NAME_FETCHED
};

typedef struct {
  bd_addr_t  bdaddr;
  uint32_t   CoD;
  uint16_t   cHandle;	/* connection handle */
} PEER_DEVICE;

struct device {
  bd_addr_t address;
  uint8_t pageScanRepetitionMode;
  uint16_t clockOffset;
  uint32_t CoD;
  enum DEVICE_STATE state;
};

typedef struct {
  uint16_t state;
  uint16_t hid_host_cid;
  int      deviceCount;
  PEER_DEVICE hidDevice;
} BTSTACK_INFO;


/*
 * Blue Button event
 */
typedef enum {
  BB_CONN = 1,		/* Connection control (disconnect) */
  BB_SCAN,		/* Start Scan (pairing mode) */
} BBEVENT;

typedef enum {
  LED_OFF = 0,		/* LED is off. No connection. */
  LED_BLINK,		/* LED is blinking. In pairing mode. */
  LED_ON,		/* LED is on. Connected with controller. */
} LED_MODE;

void post_btreq(BBEVENT code);
void pico_set_led(LED_MODE new_mode);

uint16_t get_btstack_state();
#endif
