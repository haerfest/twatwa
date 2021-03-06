#ifndef __DEFS_H
#define __DEFS_H


/**
 * The Next supports different layers that can be overlaid to generate each
 * frame. These layers do not necessarily share the same horizontal pixel size:
 *
 * Layer     | Big px | Norm px | Half px | Bordr | Hor.sc. | Ver.sc.
 * ----------|--------|---------|---------|-------|---------|--------
 * Layer 0   |        | 256x192 |         |  Yes  | 704x298 | 704x596
 * Layer 1,0 | 128x96 |         |         |  Yes  | 704x298 | 704x596
 * Layer 1,1 |        | 256x192 |         |  Yes  | 704x298 | 704x596
 * Layer 1,2 |        |         | 512x192 |  Yes  | 704x298 | 704x596
 * Layer 1,3 |        | 256x192 |         |  Yes  | 704x298 | 704x596
 * Layer 2   |        | 256x192 |         |  Yes  | 704x298 | 704x596
 *   "       |        | 320x256 |         |   No  | 640x256 | 640x512
 *   "       |        |         | 640x256 |   No  | 640x256 | 640x512
 * Layer 3,0 |        | 320x256 |         |   No  | 640x256 | 640x512
 * Layer 3,1 |        |         | 640x256 |   No  | 640x256 | 640x512
 * Layer 3,2 |        | 320x256 | 640x256 |   No  | 640x256 | 640x512
 * Layer 3,3 |        |         | 640x256 |   No  | 640x256 | 640x512
 *
 * A full (scaled) TV frame for the layers with borders is 704x596 pixels,
 * hence this frame buffer width below. For typical 50 Hz 48K display timings:
 *
 * 704 = 2 x (left border (32) + content (256) + right border  (64))
 * 596 = 2 x (top border  (49) + content (192) + bottom border (57))
 *
 * Since no TV ever showed *all* those border pixels, we cut out a 640x512
 * rectangle and display that. Actually, we only draw at 704x298 and let SDL
 * take care of the vertical doubling when blitting.
 */
#define FRAME_BUFFER_WIDTH           640
#define FRAME_BUFFER_HEIGHT          256
#define FRAME_BUFFER_SIZE            (FRAME_BUFFER_HEIGHT * FRAME_BUFFER_WIDTH * 2)

#define OVERSCAN_TOP                 24
#define OVERSCAN_BOTTOM              24
#define OVERSCAN_LEFT                 0
#define OVERSCAN_RIGHT               64

#define WINDOW_WIDTH                 FRAME_BUFFER_WIDTH
#define WINDOW_HEIGHT                (FRAME_BUFFER_HEIGHT * 2)

#define FULLSCREEN_MIN_WIDTH         (2 * WINDOW_WIDTH)
#define FULLSCREEN_MIN_HEIGHT        (2 * WINDOW_HEIGHT)
#define FULLSCREEN_MIN_REFRESH_RATE  60


typedef enum {
  E_RESET_SOFT = 0,
  E_RESET_HARD
} reset_t;


typedef enum {
  E_MACHINE_TYPE_FIRST       = 0,
  E_MACHINE_TYPE_CONFIG_MODE = E_MACHINE_TYPE_FIRST,
  E_MACHINE_TYPE_ZX_48K,
  E_MACHINE_TYPE_ZX_128K_PLUS2,
  E_MACHINE_TYPE_ZX_PLUS2A_PLUS2B_PLUS3,
  E_MACHINE_TYPE_PENTAGON,
  E_MACHINE_TYPE_LAST        = E_MACHINE_TYPE_PENTAGON
} machine_type_t;


typedef enum {
  E_TIMING_FIRST    = 0,
  E_TIMING_VGA_BASE = E_TIMING_FIRST,
  E_TIMING_VGA_1,
  E_TIMING_VGA_2,
  E_TIMING_VGA_3,
  E_TIMING_VGA_4,
  E_TIMING_VGA_5,
  E_TIMING_VGA_6,
  E_TIMING_HDMI,
  E_TIMING_LAST     = E_TIMING_HDMI
} timing_t;


typedef enum {
  E_CPU_SPEED_FIRST = 0,
  E_CPU_SPEED_3MHZ  = E_CPU_SPEED_FIRST,  /* 3.5 MHz */
  E_CPU_SPEED_7MHZ,
  E_CPU_SPEED_14MHZ,
  E_CPU_SPEED_28MHZ,
  E_CPU_SPEED_LAST  = E_CPU_SPEED_28MHZ
} cpu_speed_t;


typedef unsigned char      u8_t;
typedef unsigned short     u16_t;
typedef unsigned int       u32_t;
typedef unsigned long long u64_t;

typedef signed char        s8_t;
typedef signed short       s16_t;
typedef signed int         s32_t;
typedef signed long long   s64_t;


#endif  /* __DEFS_H */
