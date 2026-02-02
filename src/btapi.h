#ifndef _BTAPI_H_
#define	_BTAPI_H_

#define	BTREQ_DISC	1
#define	BTREQ_SCAN	2
#define	BTREQ_CLEAR	3

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
#endif
