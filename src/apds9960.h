#define	DEV_ADDR	0x39
#define	DEV_ID		0xAB

#define	REG_ENABLE	0x80
#define	REG_ATIME	0x81
#define	REG_WTIME	0x83
#define	REG_AILTL	0x84
#define	REG_AILTH	0x85
#define	REG_AIHTL	0x86
#define	REG_AIHTH	0x87
#define	REG_PILT	0x89
#define	REG_PIHT	0x8B
#define	REG_PERS	0x8C
#define	REG_CONFIG1	0x8D
#define	REG_PPULSE	0x8E
#define	REG_CONTROL	0x8F
#define	REG_CONFIG2	0x90
#define	REG_ID		0x92
#define	REG_STATUS	0x93
#define	REG_PDATA	0x9C
#define	REG_POFFSET_UR	0x9D
#define	REG_POFFSET_DL	0x9E
#define	REG_CONFIG3	0x9F
#define	REG_PICLEAR	0xE5
#define	REG_AICLEAR	0xE7

#define	REG_GPENTH	0xA0
#define	REG_GEXTH	0xA1
#define	REG_GCONF1	0xA2
#define	REG_GCONF2	0xA3
#define	REG_GOFFSET_U	0xA4
#define	REG_GOFFSET_D	0xA5
#define	REG_GOFFSET_L	0xA7
#define	REG_GOFFSET_R	0xA9
#define	REG_GPULSE	0xA6
#define	REG_GCONF3	0xAA
#define	REG_GCONF4	0xAB
#define	REG_GFLVL	0xAE
#define	REG_GSTATUS	0xAF
#define	REG_GFIFO_U	0xFC
#define	REG_GFIFO_D	0xFD
#define	REG_GFIFO_L	0xFE
#define	REG_GFIFO_R	0xFF

#define	ENABLE_GEN	(1<<6)		// Gesture Enable
#define	ENABLE_PIEN	(1<<5)		// Priximity Interrupt Enable
#define	ENABLE_WEN	(1<<3)		// Wait Enable
#define	ENABLE_PEN	(1<<2)		// Proximity Detect Enable
#define	ENABLE_PON	(1<<0)		// Power ON

#define	CONTROL_LED100	(0)
#define	CONTROL_LED50	(1<<6)
#define	CONTROL_LED25	(2<<6)
#define	CONTROL_LED12_5	(3<<6)
#define	CONTROL_PGAIN1X	(0)
#define	CONTROL_PGAIN2X	(1<<2)
#define	CONTROL_PGAIN4X	(2<<2)
#define	CONTROL_PGAIN8X	(3<<2)
#define	CONTROL_AGAIN1X	(0)
#define	CONTROL_AGAIN4X	(1)
#define	CONTROL_AGAIN16X	(2)
#define	CONTROL_AGAIN64X	(3)

#define	CONFIG2_CPSIEN	(1<<6)		// Clear Photodiode Staturation Interrupt Enable
#define	CONFIG2_LED_BOOST100	(0)
#define	CONFIG2_LED_BOOST150	(1<<4)
#define	CONFIG2_LED_BOOST200	(2<<4)
#define	CONFIG2_LED_BOOST300	(3<<4)

#define	GCONF2_GGAIN1X	(0)		// Gesture Gain Control 1x
#define	GCONF2_GGAIN2X	(1<<5)		// Gesture Gain Control 2x
#define	GCONF2_GGAIN4X	(2<<5)		// Gesture Gain Control 4x
#define	GCONF2_GGAIN8X	(3<<5)		// Gesture Gain Control 8x
#define	GCONF2_GLDRIVE100	(0)	// Gesture LED Drive Strength 100mA
#define	GCONF2_GLDRIVE50	(1<<3)	// Gesture LED Drive Strength 50mA
#define	GCONF2_GLDRIVE25	(2<<3)	// Gesture LED Drive Strength 25mA
#define	GCONF2_GLDRIVE12	(3<<3)	// Gesture LED Drive Strength 12.5mA
#define	GCONF2_GWTIME0	(0)
#define	GCONF2_GWTIME28	(1)
#define	GCONF2_GWTIME56	(2)
#define	GCONF2_GWTIME84	(3)
#define	GCONF2_GWTIME140	(4)
#define	GCONF2_GWTIME224	(5)
#define	GCONF2_GWTIME308	(6)
#define	GCONF2_GWTIME392	(7)

/* Default values */
#define DEF_ATIME           219     // 103ms
#define DEF_WTIME           246     // 27ms
#define DEF_PROX_PPULSE     0x87    // 16us, 8 pulses
#define DEF_GESTURE_PPULSE  0x89    // 16us, 10 pulses
#define DEF_POFFSET_UR      0       // 0 offset
#define DEF_POFFSET_DL      0       // 0 offset      
#define DEF_CONFIG1         0x60    // No 12x wait (WTIME) factor
#define DEF_LDRIVE          CONTROL_LED100
#define DEF_PGAIN           CONTROL_PGAIN4X
#define DEF_AGAIN           CONTROL_AGAIN4X
#define DEF_PILT            0       // Low proximity threshold
#define DEF_PIHT            50      // High proximity threshold
#define DEF_AILT            0xFFFF  // Force interrupt for calibration
#define DEF_AIHT            0
#define DEF_PERS            0x11    // 2 consecutive prox or ALS for int.
#define DEF_CONFIG2         0x01    // No saturation interrupts or LED boost  
#define DEF_CONFIG3         0       // Enable all photodiodes, no SAI
#if 1
#define DEF_GPENTH          40      // Threshold for entering gesture mode
#define DEF_GEXTH           30      // Threshold for exiting gesture mode    
#else
#define DEF_GPENTH          45      // Threshold for entering gesture mode
#define DEF_GEXTH           30      // Threshold for exiting gesture mode    
#endif
#define DEF_GCONF1          0x40    // 4 gesture events for int., 1 for exit
#define DEF_GGAIN           GGAIN_4X
#define DEF_GLDRIVE         LED_DRIVE_100MA
#define DEF_GWTIME          GWTIME_2_8MS
#define DEF_GOFFSET         0       // No offset scaling for gesture mode
#define DEF_GPULSE          0xC9    // 32us, 10 pulses
#define DEF_GCONF3          0       // All photodiodes active during gesture
#define DEF_GIEN            0       // Disable gesture interrupts

#define	FIFO_PAUSE_TIME	30
#define GESTURE_THRESHOLD_OUT   10
#define GESTURE_SENSITIVITY_1   50
#define GESTURE_SENSITIVITY_2   20

struct s_gesture_data {
  uint8_t u_data[32];
  uint8_t d_data[32];
  uint8_t l_data[32];
  uint8_t r_data[32];
  uint8_t index;
  uint8_t total_gestures;
  uint8_t in_threshold;
  uint8_t out_threshold;
};

typedef uint8_t GESTEVENT;

/* Direction definitions */
enum {
  DIR_NONE,
  DIR_LEFT,
  DIR_RIGHT,
  DIR_UP,
  DIR_DOWN,
  DIR_NEAR,
  DIR_FAR,
  DIR_ALL
};

/* State definitions */
enum {
  NA_STATE,
  NEAR_STATE,
  FAR_STATE,
  ALL_STATE
};

void apds_run_loop(void);
