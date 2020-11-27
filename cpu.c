#include <stdio.h>
#include "clock.h"
#include "defs.h"
#include "io.h"
#include "memory.h"

#include "cpu.h"

#define SHIFT_S 7
#define SHIFT_Z 6
#define SHIFT_H 4
#define SHIFT_P 2
#define SHIFT_V SHIFT_P
#define SHIFT_N 1
#define SHIFT_C 0

#define FLAG_S (1 << SHIFT_S)
#define FLAG_Z (1 << SHIFT_Z)
#define FLAG_H (1 << SHIFT_H)
#define FLAG_P (1 << SHIFT_P)
#define FLAG_V FLAG_P
#define FLAG_N (1 << SHIFT_N)
#define FLAG_C (1 << SHIFT_C)


/* Indicates bitmask to (not) set FLAG_P for any byte. */
const u8_t parity[256] = {
  0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
  0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
  0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
  0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
  0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
  0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
  0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
  0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
  0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
  0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
  0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
  0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
  0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
  0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
  0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
  0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04
};


/* TODO: Add a big-endian version. */
#define MAKE_REG(both, high, low) \
  union {                         \
    struct {                      \
      u8_t low;                   \
      u8_t high;                  \
    } b;      /* Byte. */         \
    u16_t w;  /* Word. */         \
  } both


typedef struct {
  /* Combined 8-bit and 16-bit registers. */
  MAKE_REG(af, a, f);
  MAKE_REG(hl, h, l);
  MAKE_REG(bc, b, c);
  MAKE_REG(de, d, e);

  /* Shadow registers. */
  MAKE_REG(af_, a, f);
  MAKE_REG(hl_, h, l);
  MAKE_REG(bc_, b, c);
  MAKE_REG(de_, d, e);

  /* Hidden, internal registers. */
  MAKE_REG(wz, w, z);

  /* Interrupt mode and memory refresh registers. */
  u8_t i;
  u8_t r;

  /* Index registers. */
  u16_t ix;
  u16_t iy;

  /* Additional registers. */
  u16_t pc;
  u16_t sp;

  /* Interrupt stuff. */
  int iff1;
  int iff2;
} cpu_t;


/* Local-global cpu for fast reference. */
static cpu_t cpu;


/* Convenient shortcuts. */
#define A    cpu.af.b.a
#define F    cpu.af.b.f
#define B    cpu.bc.b.b
#define C    cpu.bc.b.c
#define D    cpu.de.b.d
#define E    cpu.de.b.e
#define H    cpu.hl.b.h
#define L    cpu.hl.b.l
#define W    cpu.wz.b.w
#define Z    cpu.wz.b.z

#define BC   cpu.bc.w
#define DE   cpu.de.w
#define HL   cpu.hl.w
#define WZ   cpu.wz.w

#define BC_  cpu.bc_.w
#define DE_  cpu.de_.w
#define HL_  cpu.hl_.w

#define I    cpu.i
#define R    cpu.r

#define IX   cpu.ix
#define IY   cpu.iy

#define PC   cpu.pc
#define SP   cpu.sp

#define IFF1 cpu.iff1
#define IFF2 cpu.iff2


int cpu_init(void) {
  cpu_reset();
  return 0;
}


void cpu_finit(void) {
}


void cpu_reset(void) {
  cpu.iff1 = 0;
  cpu.pc   = 0;
  cpu.i    = 0;
  cpu.r    = 0;
  clock_ticks(3);
}


int cpu_run(u32_t ticks, s32_t* ticks_left) {
  u8_t opcode;

#include "opcodes.c"

  return 0;
}
