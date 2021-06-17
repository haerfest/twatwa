#ifndef __PALETTE_H
#define __PALETTE_H


#include "defs.h"


typedef enum {
  E_PALETTE_ULA_FIRST = 0,
  E_PALETTE_LAYER2_FIRST,
  E_PALETTE_SPRITES_FIRST,
  E_PALETTE_TILEMAP_FIRST,
  E_PALETTE_ULA_SECOND,
  E_PALETTE_LAYER2_SECOND,
  E_PALETTE_SPRITES_SECOND,
  E_PALETTE_TILEMAP_SECOND
} palette_t;


typedef struct {
  u8_t  rgb8;
  u16_t rgb9;
  u16_t rgb16;
  u8_t  is_layer2_priority;
} palette_entry_t;


int  palette_init(void);
void palette_finit(void);

const palette_entry_t* palette_read(palette_t palette, u8_t index);
void                   palette_write_rgb8(palette_t palette, u8_t index, u8_t  rgb);
void                   palette_write_rgb9(palette_t palette, u8_t index, u8_t rgb);
u16_t                  palette_rgb8_rgb16(u8_t rgb8);

#endif  /* __PALETTE_H */
