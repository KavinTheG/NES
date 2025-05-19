#include "frontend.h"

void Frontend_Init(Frontend *frontend, int w, int h, int scale) {
  SDL_Init(SDL_INIT_VIDEO);
  frontend->window =
      SDL_CreateWindow("NES", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       w * scale, h * scale, SDL_WINDOW_SHOWN);

  frontend->renderer =
      SDL_CreateRenderer(frontend->window, -1, SDL_RENDERER_ACCELERATED);

  frontend->texture =
      SDL_CreateTexture(frontend->renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_STREAMING, w, h);
  SDL_SetTextureBlendMode(frontend->texture,
                          SDL_BLENDMODE_BLEND); // Enable alpha blending
  SDL_RenderSetLogicalSize(frontend->renderer, w, h);
}

void Frontend_DrawFrame(Frontend *frontend,
                        uint32_t frame_buffer[frontend->h][frontend->w]) {
  void *pixels;
  int pitch;
  SDL_LockTexture(frontend->texture, NULL, &pixels, &pitch);
  memcpy(pixels, frame_buffer, 240 * pitch); // 240 rows
  SDL_UnlockTexture(frontend->texture);

  SDL_RenderClear(frontend->renderer);
  SDL_RenderCopy(frontend->renderer, frontend->texture, NULL, NULL);
  SDL_RenderPresent(frontend->renderer);
}

void Frontend_Destroy(Frontend *frontend) {
  SDL_DestroyTexture(frontend->texture);
  SDL_DestroyRenderer(frontend->renderer);
  SDL_DestroyWindow(frontend->window);
  SDL_Quit();
}
