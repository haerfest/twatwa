#ifndef __SLU_H
#define __SLU_H


#include <SDL2/SDL.h>
#include "defs.h"
#include "palette.h"


typedef enum {
  E_SLU_LAYER_PRIORITY_SLU = 0,  /** Sprites over Layer 2 over ULA.                                    */
  E_SLU_LAYER_PRIORITY_LSU,      /** Layer 2 over Sprites over ULA.                                    */
  E_SLU_LAYER_PRIORITY_SUL,      /** Sprites over ULA over Layer 2.                                    */
  E_SLU_LAYER_PRIORITY_LUS,      /** Layer 2 over ULA over Sprites.                                    */
  E_SLU_LAYER_PRIORITY_USL,      /** ULA over Sprites over Layer 2.                                    */
  E_SLU_LAYER_PRIORITY_ULS,      /** ULA over Layer 2 over Sprites.                                    */
  E_SLU_LAYER_PRIORITY_BLEND,    /** Sprites over (Layer 2 + ULA combined), colours clamped to 7.      */
  E_SLU_LAYER_PRIORITY_BLEND_5   /** Sprites over (Layer 2 + ULA combined), colours clamped to (0, 7). */
} slu_layer_priority_t;


int                    slu_init(SDL_Renderer* renderer, SDL_Texture* texture);
void                   slu_finit(void);
void                   slu_run(u32_t ticks_14mhz);
void                   slu_layer_priority_set(slu_layer_priority_t priority);
slu_layer_priority_t   slu_layer_priority_get(void);
void                   slu_transparency_fallback_colour_write(u8_t value);
u32_t                  slu_active_video_line_get(void);
void                   slu_line_interrupt_enable_set(int enable);
u8_t                   slu_line_interrupt_control_read(void);
void                   slu_line_interrupt_control_write(u8_t value);
u8_t                   slu_line_interrupt_value_lsb_read(void);
void                   slu_line_interrupt_value_lsb_write(u8_t value);
void                   slu_ula_control_write(u8_t value);
void                   slu_transparent_set(u8_t rgb8);
const palette_entry_t* slu_transparent_get(void);
void                   slu_reset(reset_t reset);
void                   slu_display_size_set(unsigned int rows, unsigned int columns);


#endif  /* __SLU_H */
