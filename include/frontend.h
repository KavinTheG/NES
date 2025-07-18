#include <SDL2/SDL.h>
#include <stdint.h>

typedef struct Frontend {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;

  uint32_t frame_start_tick;

  uint8_t controller;
  int w, h;
} Frontend;

void Frontend_Init(Frontend *frontend, int w, int h, int scale);
void Frontend_DrawFrame(Frontend *frontend,
                        uint32_t frame_buffer[frontend->h][frontend->w]);
int Frontend_HandleInput(Frontend *frontend);
void Frontend_SetFrameTickStart(Frontend *frontend);
void Frontend_Destroy(Frontend *frontend);

void audio_buffer_add(int16_t sample);
