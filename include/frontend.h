#include <SDL2/SDL.h>
#include <stdint.h>

typedef struct Frontend {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;

  int w, h;
} Frontend;

void Frontend_Init(Frontend *frontend, int w, int h, int scale);
void Frontend_DrawFrame(Frontend *frontend,
                        uint32_t frame_buffer[frontend->h][frontend->w]);
void Frontend_Destroy(Frontend *frontend);
