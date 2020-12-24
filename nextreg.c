#include <stdio.h>
#include "clock.h"
#include "memory.h"
#include "mmu.h"
#include "defs.h"
#include "rom.h"
#include "ula.h"


/* See https://gitlab.com/thesmog358/tbblue/-/blob/master/docs/extra-hw/io-port-system/registers.txt */
#define REGISTER_MACHINE_ID              0x00
#define REGISTER_MACHINE_TYPE            0x03
#define REGISTER_PERIPHERAL_1_SETTING    0x05
#define REGISTER_PERIPHERAL_2_SETTING    0x06
#define REGISTER_CPU_SPEED               0x07
#define REGISTER_PERIPHERAL_3_SETTING    0x08
#define REGISTER_PERIPHERAL_4_SETTING    0x09
#define REGISTER_PERIPHERAL_5_SETTING    0x10
#define REGISTER_VIDEO_TIMING            0x11
#define REGISTER_MMU_SLOT0_CONTROL       0x50
#define REGISTER_MMU_SLOT1_CONTROL       0x51
#define REGISTER_MMU_SLOT2_CONTROL       0x52
#define REGISTER_MMU_SLOT3_CONTROL       0x53
#define REGISTER_MMU_SLOT4_CONTROL       0x54
#define REGISTER_MMU_SLOT5_CONTROL       0x55
#define REGISTER_MMU_SLOT6_CONTROL       0x56
#define REGISTER_MMU_SLOT7_CONTROL       0x57
#define REGISTER_ALTERNATE_ROM           0x8C
#define REGISTER_SPECTRUM_MEMORY_MAPPING 0x8E
#define REGISTER_MEMORY_MAPPING_MODE     0x8F


typedef struct {
  u8_t selected_register;
  int  configuration_mode;
} nextreg_t;


static nextreg_t self;


int nextreg_init(void) {
  self.selected_register  = 0x55;
  self.configuration_mode = 0;
  return 0;
}


void nextreg_finit(void) {
}


void nextreg_select_write(u16_t address, u8_t value) {
  self.selected_register = value;
}


u8_t nextreg_select_read(u16_t address) {
  return 0xFF;
}


static void nextreg_machine_type_write(u8_t value) {
  if (value & 0x80) {
    ula_display_timing_set((value >> 4) & 0x03);
  }

  self.configuration_mode = (value & 0x03) == 0x00;
  if (self.configuration_mode) {
    fprintf(stderr, "nextreg: configuration mode activated\n");
  }
}


static void nextreg_peripheral_1_setting_write(u8_t value) {
  ula_display_frequency_set(value & 0x04);
}


static void nextreg_cpu_speed_write(u8_t value) {
  clock_cpu_speed_set(value & 0x03);
}


static void nextreg_video_timing_write(u8_t value) {
  if (self.configuration_mode) {
    ula_video_timing_set(value & 0x03);
  }
}


static void nextreg_memory_all_ram(u8_t slot_1_bank_16k, u8_t slot_2_bank_16k, u8_t slot_3_bank_16k, u8_t slot_4_bank_16k) {
  mmu_bank_set(1, slot_1_bank_16k);
  mmu_bank_set(2, slot_2_bank_16k);
  mmu_bank_set(3, slot_3_bank_16k);
  mmu_bank_set(4, slot_4_bank_16k);
}


static void nextreg_spectrum_memory_mapping_write(u8_t value) {
  const int change_bank = value & 0x08;
  const int paging_mode = value & 0x04;
  
  if (change_bank) {
    mmu_bank_set(4, value >> 4);
  }

  if (paging_mode == 0) {
    rom_select(value & 0x03);
  } else {
    switch (value & 0x03) {
      case 0: nextreg_memory_all_ram(0, 1, 2, 3); break;
      case 1: nextreg_memory_all_ram(4, 5, 6, 7); break;
      case 2: nextreg_memory_all_ram(4, 5, 6, 3); break;
      case 3: nextreg_memory_all_ram(4, 7, 6, 3); break;
    }
  }
}


static void nextreg_alternate_rom_write(u8_t value) {
  const int  enable        = value & 0x80;
  const int  during_writes = value & 0x40;
  const u8_t lock_rom      = (value & 0x30) >> 4;

  rom_configure_altrom(enable, during_writes, lock_rom);
}


void nextreg_data_write(u16_t address, u8_t value) {
  switch (self.selected_register) {
    case REGISTER_MACHINE_TYPE:
      nextreg_machine_type_write(value);
      break;

    case REGISTER_PERIPHERAL_1_SETTING:
      nextreg_peripheral_1_setting_write(value);
      break;

    case REGISTER_CPU_SPEED:
      nextreg_cpu_speed_write(value);
      break;

    case REGISTER_VIDEO_TIMING:
      nextreg_video_timing_write(value);
      break;

    case REGISTER_MMU_SLOT0_CONTROL:
    case REGISTER_MMU_SLOT1_CONTROL:
    case REGISTER_MMU_SLOT2_CONTROL:
    case REGISTER_MMU_SLOT3_CONTROL:
    case REGISTER_MMU_SLOT4_CONTROL:
    case REGISTER_MMU_SLOT5_CONTROL:
    case REGISTER_MMU_SLOT6_CONTROL:
    case REGISTER_MMU_SLOT7_CONTROL:
      mmu_page_set(self.selected_register - REGISTER_MMU_SLOT0_CONTROL, value);
      break;

    case REGISTER_ALTERNATE_ROM:
      nextreg_alternate_rom_write(value);
      break;

    case REGISTER_SPECTRUM_MEMORY_MAPPING:
      nextreg_spectrum_memory_mapping_write(value);
      break;

    default:
      fprintf(stderr, "nextreg: unimplemented write of $%02X to Next register $%02X\n", value, self.selected_register);
      break;
  }
}


u8_t nextreg_data_read(u16_t address) {
  switch (self.selected_register) {
    case REGISTER_MACHINE_ID:
      return 0x08;  /* Emulator. */

    case REGISTER_MMU_SLOT0_CONTROL:
    case REGISTER_MMU_SLOT1_CONTROL:
    case REGISTER_MMU_SLOT2_CONTROL:
    case REGISTER_MMU_SLOT3_CONTROL:
    case REGISTER_MMU_SLOT4_CONTROL:
    case REGISTER_MMU_SLOT5_CONTROL:
    case REGISTER_MMU_SLOT6_CONTROL:
    case REGISTER_MMU_SLOT7_CONTROL:
      return mmu_page_get(self.selected_register - REGISTER_MMU_SLOT0_CONTROL);

    default:
      fprintf(stderr, "nextreg: unimplemented read from Next register $%02X\n", self.selected_register);
      break;
  }

  return 0xFF;
}


