#ifndef __NEXTREG_H
#define __NEXTREG_H


#include "defs.h"


#define NEXTREG_SELECT  0x243B
#define NEXTREG_DATA    0x253B


/* See https://gitlab.com/SpectrumNext/ZX_Spectrum_Next_FPGA/-/raw/master/cores/zxnext/nextreg.txt */
typedef enum {
  E_NEXTREG_REGISTER_MACHINE_ID                            = 0x00,
  E_NEXTREG_REGISTER_CORE_VERSION                          = 0x01,
  E_NEXTREG_REGISTER_RESET                                 = 0x02,
  E_NEXTREG_REGISTER_MACHINE_TYPE                          = 0x03,
  E_NEXTREG_REGISTER_CONFIG_MAPPING                        = 0x04,
  E_NEXTREG_REGISTER_PERIPHERAL_1_SETTING                  = 0x05,
  E_NEXTREG_REGISTER_PERIPHERAL_2_SETTING                  = 0x06,
  E_NEXTREG_REGISTER_CPU_SPEED                             = 0x07,
  E_NEXTREG_REGISTER_PERIPHERAL_3_SETTING                  = 0x08,
  E_NEXTREG_REGISTER_PERIPHERAL_4_SETTING                  = 0x09,
  E_NEXTREG_REGISTER_PERIPHERAL_5_SETTING                  = 0x0A,
  E_NEXTREG_REGISTER_CORE_VERSION_SUB_MINOR                = 0x0E,
  E_NEXTREG_REGISTER_CORE_BOOT                             = 0x10,
  E_NEXTREG_REGISTER_VIDEO_TIMING                          = 0x11,
  E_NEXTREG_REGISTER_LAYER2_ACTIVE_RAM_BANK                = 0x12,
  E_NEXTREG_REGISTER_LAYER2_SHADOW_RAM_BANK                = 0x13,
  E_NEXTREG_REGISTER_GLOBAL_TRANSPARENCY_COLOUR            = 0x14,
  E_NEXTREG_REGISTER_SPRITE_LAYERS_SYSTEM                  = 0x15,
  E_NEXTREG_REGISTER_CLIP_WINDOW_ULA                       = 0x1A,
  E_NEXTREG_REGISTER_CLIP_WINDOW_CONTROL                   = 0x1C,
  E_NEXTREG_REGISTER_OFFSET_X_MSB                          = 0X2F,
  E_NEXTREG_REGISTER_OFFSET_X_LSB                          = 0X30,
  E_NEXTREG_REGISTER_OFFSET_Y                              = 0X31,
  E_NEXTREG_REGISTER_PALETTE_INDEX                         = 0x40,
  E_NEXTREG_REGISTER_PALETTE_VALUE_8BITS                   = 0x41,
  E_NEXTREG_REGISTER_PALETTE_CONTROL                       = 0x43,
  E_NEXTREG_REGISTER_PALETTE_VALUE_9BITS                   = 0x44,
  E_NEXTREG_REGISTER_FALLBACK_COLOUR                       = 0x4A,
  E_NEXTREG_REGISTER_TILEMAP_TRANSPARENCY_INDEX            = 0x4C,
  E_NEXTREG_REGISTER_MMU_SLOT0_CONTROL                     = 0x50,
  E_NEXTREG_REGISTER_MMU_SLOT1_CONTROL                     = 0x51,
  E_NEXTREG_REGISTER_MMU_SLOT2_CONTROL                     = 0x52,
  E_NEXTREG_REGISTER_MMU_SLOT3_CONTROL                     = 0x53,
  E_NEXTREG_REGISTER_MMU_SLOT4_CONTROL                     = 0x54,
  E_NEXTREG_REGISTER_MMU_SLOT5_CONTROL                     = 0x55,
  E_NEXTREG_REGISTER_MMU_SLOT6_CONTROL                     = 0x56,
  E_NEXTREG_REGISTER_MMU_SLOT7_CONTROL                     = 0x57,
  E_NEXTREG_REGISTER_ULA_CONTROL                           = 0x68,
  E_NEXTREG_REGISTER_TILEMAP_CONTROL                       = 0x6B,
  E_NEXTREG_REGISTER_TILEMAP_DEFAULT_TILEMAP_ATTRIBUTE     = 0x6C,
  E_NEXTREG_REGISTER_TILEMAP_TILEMAP_BASE_ADDRESS          = 0x6E,
  E_NEXTREG_REGISTER_TILEMAP_TILE_DEFINITIONS_BASE_ADDRESS = 0x6F,
  E_NEXTREG_REGISTER_LAYER2_CONTROL                        = 0x70,
  E_NEXTREG_REGISTER_INTERNAL_PORT_DECODING_0              = 0x82,
  E_NEXTREG_REGISTER_INTERNAL_PORT_DECODING_1              = 0x83,
  E_NEXTREG_REGISTER_INTERNAL_PORT_DECODING_2              = 0x84,
  E_NEXTREG_REGISTER_INTERNAL_PORT_DECODING_3              = 0x85,
  E_NEXTREG_REGISTER_ALTERNATE_ROM                         = 0x8C,
  E_NEXTREG_REGISTER_SPECTRUM_MEMORY_MAPPING               = 0x8E,
  E_NEXTREG_REGISTER_MEMORY_MAPPING_MODE                   = 0x8F
} nextreg_register_t;


int  nextreg_init(void);
void nextreg_finit(void);
void nextreg_data_write(u16_t address, u8_t value);
u8_t nextreg_data_read(u16_t address);
void nextreg_select_write(u16_t address, u8_t value);
u8_t nextreg_select_read(u16_t address);
void nextreg_write_internal(u8_t reg, u8_t value);
u8_t nextreg_read_internal(u8_t reg);


#endif  /* __NEXTREG_H */
