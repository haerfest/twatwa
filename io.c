#include "ay.h"
#include "divmmc.h"
#include "nextreg.h"
#include "dac.h"
#include "defs.h"
#include "io.h"
#include "layer2.h"
#include "log.h"
#include "i2c.h"
#include "paging.h"
#include "spi.h"
#include "ula.h"


typedef struct {
  int is_port_7FFD_enabled;
  int is_port_BFFD_enabled;
  int is_port_FFFD_enabled;
} self_t;


static self_t self;


int io_init(void) {
  io_reset();
  return 0;
}


void io_reset(void) {
  self.is_port_7FFD_enabled = 1;
  self.is_port_BFFD_enabled = 1;
  self.is_port_FFFD_enabled = 1;
}


void io_finit(void) {
}


u8_t io_read(u16_t address) {
  log_dbg("io: read from $%04X\n", address);

  if ((address & 0x0001) == 0x0000) {
    return ula_read(address);
  }

  /* TODO: Bit 14 of address must be set on Plus 3? */
  if ((address & 0x8003) == 0x0001) {
    /* Typically 0x7FFD. */
    return paging_spectrum_128k_paging_read();
  }

  /* TODO: 0xBFFD is readable on +3. */
  if ((address & 0xC007) == 0xC005) {
    /* 0xFFFD */
    return self.is_port_FFFD_enabled ? ay_register_read() : 0xFF;
  }

  switch (address & 0x00FF) {
    case 0x00E3:
      return divmmc_control_read(address);

    case 0xE7:
      return spi_cs_read(address);

    case 0xEB:
      return spi_data_read(address);

    case 0xFF:
      return ula_timex_read(address);

    case 0x0F:
    case 0x1F:
    case 0x3F:
    case 0x4F:
    case 0x5F:
    case 0xB3:
    case 0xDF:
    case 0xF1:
    case 0xF3:
    case 0xF9:
    case 0xFB:
      return dac_read(address);

    default:
      break;
  }

  switch (address) {
    case 0x113B:
      return i2c_sda_read(address);

    case 0x123B:
      return layer2_read(address);

    case 0x1FFD:
      return paging_spectrum_plus_3_paging_read();

    case 0x243B:
      return nextreg_select_read(address);

    case 0x253B:
      return nextreg_data_read(address);

    case 0x7FFD:
      return paging_spectrum_128k_paging_read();

    case 0xDFFD:
      paging_spectrum_plus_3_paging_read();
      break;

    default:
      break;
  } 

  log_wrn("io: unimplemented read from $%04X\n", address);

  return 0xFF;
}


void io_write(u16_t address, u8_t value) {
  log_dbg("io: write of $%02X to $%04X\n", value, address);

  if ((address & 0x0001) == 0x0000) {
    ula_write(address, value);
    return;
  }

  /* TODO: Bit 14 of address must be set on Plus 3? */
  if ((address & 0x8003) == 0x0001) {
    /* Typically 0x7FFD. */
    if (self.is_port_7FFD_enabled) {
      paging_spectrum_128k_paging_write(value);
    }
    return;
  }

  switch (address & 0xC007) {
    case 0xC005:  /* 0xFFFD */
      if (self.is_port_FFFD_enabled) {
        ay_register_select(value);
      }
      return;

    case 0x8005:  /* 0xBFFD */
      if (self.is_port_BFFD_enabled) {
        ay_register_write(value);
      }
      return;

    default:
      break;
  }

  switch (address & 0x00FF) {
    case 0x00E3:
      divmmc_control_write(address, value);
      return;

    case 0xE7:
      spi_cs_write(address, value);
      return;

    case 0xEB:
      spi_data_write(address, value);
      return;

    case 0xFF:
      ula_timex_write(address, value);
      return;

    case 0x0F:
    case 0x1F:
    case 0x3F:
    case 0x4F:
    case 0x5F:
    case 0xB3:
    case 0xDF:
    case 0xF1:
    case 0xF3:
    case 0xF9:
    case 0xFB:
      dac_write(address, value);
      return;

    default:
      break;
  }

  switch (address) {
    case 0x103B:
      i2c_scl_write(address, value);
      return;

    case 0x113B:
      i2c_sda_write(address, value);
      return;

    case 0x123B:
      layer2_write(address, value);
      return;

#if 0
    case 0x1FFD:
      paging_spectrum_plus_3_paging_write(value);
      return;
#endif

    case 0x243B:
      nextreg_select_write(address, value);
      return;

    case 0x253B:
      nextreg_data_write(address, value);
      return;

#if 0
    case 0xDFFD:
      paging_spectrum_next_bank_extension_write(value);
      return;
#endif

    default:
      break;
  } 

  log_wrn("io: unimplemented write of $%02X to $%04X\n", value, address);
}


void io_port_enable(u16_t address, int enable) {
  switch (address) {
    case 0x7FFD:
      self.is_port_7FFD_enabled = enable;
      break;

    case 0xBFFD:
      self.is_port_BFFD_enabled = enable;
      break;

    case 0xFFFD:
      self.is_port_FFFD_enabled = enable;
      break;
      
    default:
      log_wrn("io: port $%04X en/disabling not implemented\n", address);
      break;
  }
}
    
