#include <SDL2/SDL.h>
#include <string.h>
#include "copper.h"
#include "cpu.h"
#include "defs.h"
#include "layer2.h"
#include "log.h"
#include "palette.h"
#include "slu.h"
#include "sprites.h"
#include "tilemap.h"
#include "ula.h"


#define HORIZONTAL_RETRACE  192  /* Pixels or clock ticks. */
#define VERTICAL_RETRACE      8  /* Lines.                 */


typedef enum {
  E_BLEND_MODE_ULA = 0,
  E_BLEND_MODE_NONE,
  E_BLEND_MODE_ULA_TILEMAP_MIX,
  E_BLEND_MODE_TILEMAP
} blend_mode_t;


typedef struct {
  SDL_Renderer*        renderer;
  SDL_Texture*         texture;
  u16_t*               frame_buffer;
  slu_layer_priority_t layer_priority;
  u32_t                beam_row;
  u32_t                beam_column;
  int                  is_beam_visible;
  u16_t                fallback_rgba;
  int                  line_irq_active;
  int                  line_irq_enabled;
  u16_t                line_irq_row;
  int                  stencil_mode;
  blend_mode_t         blend_mode;
  u8_t                 transparent_rgb8;
} self_t;


static self_t self;


int slu_init(SDL_Renderer* renderer, SDL_Texture* texture) {
  memset(&self, 0, sizeof(self));

  self.frame_buffer = malloc(FRAME_BUFFER_SIZE);
  if (self.frame_buffer == NULL) {
    log_err("slu: out of memory\n");
    return -1;
  }

  self.renderer         = renderer;
  self.texture          = texture;
  self.transparent_rgb8 = 0xE3;

  return 0;
}


void slu_finit(void) {
  if (self.frame_buffer != NULL) {
    free(self.frame_buffer);
    self.frame_buffer = NULL;
  }
}


static void slu_blit(void) {
  SDL_Rect source_rect = {
    .x = 0,
    .y = 0,
    .w = WINDOW_WIDTH,
    .h = WINDOW_HEIGHT / 2
  };
  void* pixels;
  int   pitch;

  if (SDL_LockTexture(self.texture, NULL, &pixels, &pitch) != 0) {
    log_err("slu: SDL_LockTexture error: %s\n", SDL_GetError());
    return;
  }

  memcpy(pixels, self.frame_buffer, FRAME_BUFFER_SIZE);
  SDL_UnlockTexture(self.texture);

  if (SDL_RenderCopy(self.renderer, self.texture, &source_rect, NULL) != 0) {
    log_err("slu: SDL_RenderCopy error: %s\n", SDL_GetError());
    return;
  }

  SDL_RenderPresent(self.renderer);
}


static void slu_beam_advance(void) {
  u16_t rows;
  u16_t columns;

  /**
   * Beam (0, 0) is the top-left pixel of the (typically) 256x192 content
   * area.
   */
  ula_display_size_get(&rows, &columns);

  /* Advance beam one pixel horizontally. */
  if (++self.beam_column < columns) {
    return;
  }

  /* Advance beam to beginning of next line. */
  self.beam_column = 0;
  if (++self.beam_row < rows) {
    return;
  }

  /* Move beam to top left of display. */
  self.beam_row = 0;

  /* Update display. */
  slu_blit();

  /* Notify the ULA that we completed a frame. */
  ula_did_complete_frame();  
}


static void slu_irq(void) {
  if (self.line_irq_enabled && self.beam_row == self.line_irq_row) {
    if (self.beam_column == 0) {
      self.line_irq_active = 1;
      cpu_irq();
    }
  } else {
    self.line_irq_active = 0;
  }
}


u32_t slu_run(u32_t ticks_14mhz) {
  const palette_entry_t black = {
    .rgb8  = 0,
    .rgb9  = 0,
    .rgb16 = 0
  };

  u32_t tick;
  u32_t frame_buffer_row;
  u32_t frame_buffer_column;

  /* These are the same names as in the VHDL for consistency. */
  int              ula_en;
  int              ula_border;
  int              ula_clipped;
  int              ula_transparent;
  const palette_entry_t* ula_rgb;

  int              ulatm_transparent;
  const palette_entry_t* ulatm_rgb;

  int              ula_final_transparent;
  const palette_entry_t* ula_final_rgb;

  int              ula_mix_transparent;
  const palette_entry_t* ula_mix_rgb;

  int              tm_en;
  int              tm_transparent;
  const palette_entry_t* tm_rgb;
  int              tm_pixel_en;
  int              tm_pixel_textmode;
  int              tm_pixel_below;

  int              sprite_transparent;
  u16_t            sprite_rgb16;
  int              sprite_pixel_en;

  int              layer2_pixel_en;
  int              layer2_priority;
  int              layer2_transparent;
  const palette_entry_t* layer2_rgb;

  int              stencil_transparent;
  palette_entry_t  stencil_rgb;

  int              mix_rgb_transparent;
  const palette_entry_t* mix_rgb;
  int              mix_top_transparent;
  const palette_entry_t* mix_top_rgb;
  int              mix_bot_transparent;
  const palette_entry_t* mix_bot_rgb;

  u8_t             mixer_r;
  u8_t             mixer_g;
  u8_t             mixer_b;

  u16_t            rgb_out;

  for (tick = 0; tick < ticks_14mhz; tick++) {
    slu_beam_advance();
    slu_irq();

    /* Copper runs at 28 MHz. */
    copper_tick(self.beam_row, self.beam_column);
    copper_tick(self.beam_row, self.beam_column);

    if (!ula_tick(self.beam_row, self.beam_column, &ula_en, &ula_border, &ula_clipped, &ula_rgb, &frame_buffer_row, &frame_buffer_column)) {
      /* Beam is outside frame buffer. */
      continue;
    }

    tilemap_tick(frame_buffer_row, frame_buffer_column, &tm_en, &tm_pixel_en, &tm_pixel_below, &tm_pixel_textmode, &tm_rgb);
    sprites_tick(frame_buffer_row, frame_buffer_column, &sprite_pixel_en, &sprite_rgb16);
    layer2_tick( frame_buffer_row, frame_buffer_column, &layer2_pixel_en, &layer2_rgb, &layer2_priority);

    ula_mix_transparent = ula_clipped || (ula_rgb->rgb8 == self.transparent_rgb8);
    ula_mix_rgb         = ula_mix_transparent ? ula_rgb : &black;

    ula_transparent = ula_mix_transparent || !ula_en;
    if (ula_transparent) {
      ula_rgb = &black;
    }

    tm_transparent = !tm_en || !tm_pixel_en || (tm_pixel_textmode && tm_rgb->rgb8 == self.transparent_rgb8);
    if (tm_transparent) {
      tm_rgb = &black;
    }

    stencil_transparent = ula_transparent || tm_transparent;
    stencil_rgb.rgb16   = !stencil_transparent ? (ula_rgb->rgb16 & tm_rgb->rgb16) : 0;

    ulatm_transparent = ula_transparent && tm_transparent;
    ulatm_rgb         = (!tm_transparent && (!tm_pixel_below || ula_transparent)) ? tm_rgb : ula_rgb;

    sprite_transparent = !sprite_pixel_en;
    if (sprite_transparent) {
      sprite_rgb16 = 0;
    }

    layer2_transparent = !layer2_pixel_en || (layer2_rgb->rgb8 == self.transparent_rgb8);
    if (layer2_transparent) {
      layer2_rgb      = &black;
      layer2_priority = 0;
    }

    if (self.stencil_mode && ula_en && tm_en) {
      ula_final_rgb         = &stencil_rgb;
      ula_final_transparent = stencil_transparent;
    } else {
      ula_final_rgb         = ulatm_rgb;
      ula_final_transparent = ulatm_transparent;
    }

    switch (self.blend_mode) {
      case E_BLEND_MODE_ULA:
        mix_rgb             = ula_mix_rgb;
        mix_rgb_transparent = ula_mix_transparent;
        mix_top_transparent = tm_transparent || tm_pixel_below;
        mix_top_rgb         = tm_rgb;
        mix_bot_transparent = tm_transparent || !tm_pixel_below;
        mix_bot_rgb         = tm_rgb;
        break;

      case E_BLEND_MODE_ULA_TILEMAP_MIX:
        mix_rgb             = ula_final_rgb;
        mix_rgb_transparent = ula_final_transparent;
        mix_top_transparent = 1;
        mix_top_rgb         = tm_rgb;
        mix_bot_transparent = 1;
        mix_bot_rgb         = tm_rgb;
        break;

      case E_BLEND_MODE_TILEMAP:
        mix_rgb             = tm_rgb;
        mix_rgb_transparent = tm_transparent;
        mix_top_transparent = ula_transparent || !tm_pixel_below;
        mix_top_rgb         = ula_rgb;
        mix_bot_transparent = ula_transparent || tm_pixel_below;
        mix_bot_rgb         = ula_rgb;
        break;

      default:
        mix_rgb             = 0;
        mix_rgb_transparent = 1;
        if (tm_pixel_below) {
          mix_top_transparent = ula_transparent;
          mix_top_rgb         = ula_rgb;
          mix_bot_transparent = tm_transparent;
          mix_bot_rgb         = tm_rgb;
        } else {
          mix_top_transparent = tm_transparent;
          mix_top_rgb         = tm_rgb;
          mix_bot_transparent = ula_transparent;
          mix_bot_rgb         = ula_rgb;
        }
        break;
    }

    mixer_r = ((layer2_rgb->rgb9 & 0x1C0) >> 2) | ((mix_rgb->rgb9 & 0x1C0) >> 6);
    mixer_g = ((layer2_rgb->rgb9 & 0x038) << 1) | ((mix_rgb->rgb9 & 0x038) >> 3);
    mixer_b = ((layer2_rgb->rgb9 & 0x007) << 4) |  (mix_rgb->rgb9 & 0x007);

    rgb_out = self.fallback_rgba;

    switch (self.layer_priority)
    {
      case E_SLU_LAYER_PRIORITY_SLU:
        if (layer2_priority) {
          rgb_out = layer2_rgb->rgb16;
        } else if (!sprite_transparent) {
          rgb_out = sprite_rgb16;
        } else if (!layer2_transparent) {
          rgb_out = layer2_rgb->rgb16;
        } else if (!ula_final_transparent) {
          rgb_out = ula_final_rgb->rgb16;
        }
        break;

      case E_SLU_LAYER_PRIORITY_LSU:
        if (!layer2_transparent) {
          rgb_out = layer2_rgb->rgb16;
        } else if (!sprite_transparent) {
          rgb_out = sprite_rgb16;
        } else if (!ula_final_transparent) {
          rgb_out = ula_final_rgb->rgb16;
        }
        break;

      case E_SLU_LAYER_PRIORITY_SUL:
        if (layer2_priority) {
          rgb_out = layer2_rgb->rgb16;
        } else if (!sprite_transparent) {
          rgb_out = sprite_rgb16;
        } else if (!ula_final_transparent) {
          rgb_out = ula_final_rgb->rgb16;
        } else if (!layer2_transparent) {
          rgb_out = layer2_rgb->rgb16;
        }
        break;

      case E_SLU_LAYER_PRIORITY_LUS:
        if (!layer2_transparent) {
          rgb_out = layer2_rgb->rgb16;
        } else if (!ula_final_transparent && !(ula_border && tm_transparent && !sprite_transparent)) {
          rgb_out = ula_final_rgb->rgb16;
        } else if (!sprite_transparent) {
          rgb_out = sprite_rgb16;
        }
        break;
        
      case E_SLU_LAYER_PRIORITY_USL:
        if (layer2_priority) {
          rgb_out = layer2_rgb->rgb16;
        } else if (!ula_final_transparent && !(ula_border && tm_transparent && !sprite_transparent)) {
          rgb_out = ula_final_rgb->rgb16;
        } else if (!sprite_transparent) {
          rgb_out = sprite_rgb16;
        } else if (!layer2_transparent) {
          rgb_out = layer2_rgb->rgb16;
        }
        break;

      case E_SLU_LAYER_PRIORITY_ULS:
        if (layer2_priority) {
          rgb_out = layer2_rgb->rgb16;
        } else if (!ula_final_transparent && !(ula_border && tm_transparent && !sprite_transparent)) {
          rgb_out = ula_final_rgb->rgb16;
        } else if (!layer2_transparent) {
          rgb_out = layer2_rgb->rgb16;
        } else if (!sprite_transparent) {
          rgb_out = sprite_rgb16;
        }
        break;

      case E_SLU_LAYER_PRIORITY_BLEND:
        if (mixer_r & 0x08) mixer_r = 7;
        if (mixer_g & 0x08) mixer_g = 7;
        if (mixer_b & 0x08) mixer_b = 7;

        if (layer2_priority) {
          rgb_out = (mixer_r << 13) | (mixer_g << 9) | (mixer_b << 5);
        } else if (!mix_top_transparent) {
          rgb_out = mix_top_rgb->rgb16;
        } else if (!sprite_transparent) {
          rgb_out = sprite_rgb16;
        } else if (!mix_bot_transparent) {
          rgb_out = mix_bot_rgb->rgb16;
        } else if (!layer2_transparent) {
          rgb_out = (mixer_r << 13) | (mixer_g << 9) | (mixer_b << 5);
        }
        break;

      case E_SLU_LAYER_PRIORITY_BLEND_5:
        if (!mix_rgb_transparent) {
          if (mixer_r <= 4) {
            mixer_r = 0;
          } else if ((mixer_r & 0x0C) == 0x0C) {
            mixer_r = 7;
          } else {
            mixer_r = mixer_r - 5;
          }

          if (mixer_g <= 4) {
            mixer_g = 0;
          } else if ((mixer_g & 0x0C) == 0x0C) {
            mixer_g = 7;
          } else {
            mixer_g = mixer_g - 5;
          }

          if (mixer_b <= 4) {
            mixer_b = 0;
          } else if ((mixer_b & 0x0C) == 0x0C) {
            mixer_b = 7;
          } else {
            mixer_b = mixer_b - 5;
          }
        }

        if (layer2_priority) {
          rgb_out = (mixer_r << 13) | (mixer_g << 9) | (mixer_b << 5);
        } else if (!mix_top_transparent) {
          rgb_out = mix_top_rgb->rgb16;
        } else if (!sprite_transparent) {
          rgb_out = sprite_rgb16;
        } else if (!mix_bot_transparent) {
          rgb_out = mix_bot_rgb->rgb16;
        } else if (!layer2_transparent) {
          rgb_out = (mixer_r << 13) | (mixer_g << 9) | (mixer_b << 5);
        }
        break;
    }
    
    self.frame_buffer[frame_buffer_row * FRAME_BUFFER_WIDTH + frame_buffer_column] = rgb_out;
  }

  /* TODO Deal with ULA using 7 MHz clock, not always consuming all ticks. */
  return ticks_14mhz;
}


u32_t slu_active_video_line_get(void) {
  return self.beam_row;
}


void slu_layer_priority_set(slu_layer_priority_t priority) {
  self.layer_priority = priority & 0x07;

  log_wrn("slu: layer priority %d\n", self.layer_priority);
}


slu_layer_priority_t slu_layer_priority_get(void) {
  return self.layer_priority;
}


void slu_transparency_fallback_colour_write(u8_t value) {
  self.fallback_rgba = palette_rgb8_rgb16(value);
}


u8_t slu_line_interrupt_control_read(void) {
  return (self.line_irq_active  ? 0x80 : 0x00) |
         (!ula_irq_enable_get() ? 0x04 : 0x00) |
         (self.line_irq_enabled ? 0x02 : 0x00) |
         (self.line_irq_row >> 8);
}


void slu_line_interrupt_control_write(u8_t value) {
  ula_irq_enable_set(!(value & 0x04));

  self.line_irq_row     = ((value & 0x01) << 8) | (self.line_irq_row & 0x00FF);
  self.line_irq_enabled = value & 0x02;

  if (!self.line_irq_enabled) {
    self.line_irq_active = 0;
  }
}


u8_t slu_line_interrupt_value_lsb_read(void) {
  return self.line_irq_row & 0x00FF;
}


void slu_line_interrupt_value_lsb_write(u8_t value) {
  self.line_irq_row = (self.line_irq_row & 0x0100) | value;
}


void slu_ula_control_write(u8_t value) {
  ula_enable_set((value & 0x80) == 0);
  self.blend_mode               = (value & 0x60) >> 5;
  self.stencil_mode             = value & 0x01;

  log_wrn("slu: blend mode %d\n", self.blend_mode);
}


void slu_transparent_rgb8_set(u8_t rgb) {
  self.transparent_rgb8 = rgb;
}
