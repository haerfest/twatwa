#include <SDL2/SDL.h>
#include "clock.h"
#include "cpu.h"
#include "defs.h"
#include "divmmc.h"
#include "io.h"
#include "memory.h"
#include "mmu.h"
#include "nextreg.h"
#include "ula.h"
#include "utils.h"


#define WINDOW_WIDTH  (32 + 256 + 64)
#define WINDOW_HEIGHT 312


typedef struct {
  SDL_Window*   window;
  SDL_Renderer* renderer;
  SDL_Texture*  texture;
} main_t;


static main_t mine;


static int main_init(void) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_Log("SDL_Init: %s\n", SDL_GetError());
    goto exit;
  }

  if (SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &mine.window, &mine.renderer) != 0) {
    fprintf(stderr, "main: SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
    goto exit_sdl;
  }

  mine.texture = SDL_CreateTexture(mine.renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, WINDOW_WIDTH, WINDOW_HEIGHT);
  if (mine.texture == NULL) {
    fprintf(stderr, "main: SDL_CreateTexture error: %s\n", SDL_GetError());
    goto exit_window_and_renderer;
  }

  if (utils_init() != 0) {
    goto exit_texture;
  }

  if (nextreg_init() != 0) {
    goto exit_utils;
  }

  if (divmmc_init() != 0) {
    goto exit_nextreg;
  }

  if (io_init() != 0) {
    goto exit_divmmc;
  }

  if (mmu_init() != 0) {
    goto exit_io;
  }

  if (memory_init() != 0) {
    goto exit_mmu;
  }

  if (clock_init() != 0) {
    goto exit_memory;
  }

  if (ula_init(mine.renderer, mine.texture) != 0) {
    goto exit_clock;
  }

  if (cpu_init() != 0) {
    goto exit_ula;
  }

  return 0;

exit_ula:
  ula_finit();
exit_clock:
  clock_finit();
exit_memory:
  memory_finit();
exit_mmu:
  mmu_finit();
exit_io:
  io_finit();
exit_divmmc:
  divmmc_finit();
exit_nextreg:
  nextreg_finit();
exit_utils:
  utils_finit();
exit_texture:
  SDL_DestroyTexture(mine.texture);
exit_window_and_renderer:
  SDL_DestroyRenderer(mine.renderer);
  SDL_DestroyWindow(mine.window);
exit_sdl:
  SDL_Quit();
exit:
  return -1;
}


static void main_eventloop(void) {
  SDL_Event event;
  u32_t     t0;
  u32_t     t1;
  u32_t     consumed;
  s32_t     ticks_left;

  ticks_left = 0;
  for (;;) {
    if (SDL_QuitRequested() == SDL_TRUE) {
      break;
    }

    t0 = SDL_GetTicks();
    if (cpu_run(1000000 + ticks_left, &ticks_left) != 0) {
      break;
    }
    t1 = SDL_GetTicks();

#if 0
    /* Assuming 3.5 MHz, 1,000,000 ticks take 286 milliseconds. */
    consumed = t1 - t0;
    SDL_Delay(286 - consumed);
#endif
  }
}


static void main_finit(void) {
  cpu_finit();
  ula_finit();
  clock_finit();
  memory_finit();
  mmu_finit();
  io_finit();
  divmmc_finit();
  nextreg_finit();
  utils_finit();

  SDL_DestroyTexture(mine.texture);
  SDL_DestroyRenderer(mine.renderer);
  SDL_DestroyWindow(mine.window);
  SDL_Quit();
}


int main(int argc, char* argv[]) {
  if (main_init() != 0) {
    return 1;
  }

  main_eventloop();
  main_finit();

  return 0;
}
