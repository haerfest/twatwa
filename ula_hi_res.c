static int ula_display_mode_hi_res(u32_t row, u32_t column, u8_t* palette_index) {
  const u8_t  mask             = 1 << (7 - (column & 0x07));
  const u16_t display_offset   = ((row & 0xC0) << 5) | ((row & 0x07) << 8) | ((row & 0x38) << 2) | (((column >> 1) / 8) & 0x1F);;
  const u8_t* display_ram      = ((column / 8) & 0x01) ? self.display_ram_odd : self.display_ram;
  const u8_t  display_byte     = display_ram[display_offset];

  *palette_index = (display_byte & mask)
    ? 0  + 8 + self.hi_res_ink_colour
    : 16 + 8 + (~self.hi_res_ink_colour & 0x07);

  return 1;
}
