#include <SDL2/SDL.h>
#include <stdint.h>

typedef struct Renderer {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    int w,h;
} Renderer;

void Renderer_Init(Renderer *renderer, int w, int h, int scale);
void Renderer_DrawFrame(Renderer *renderer, uint32_t frame_buffer[renderer->h][renderer->w]);
void Renderer_Destroy(Renderer *renderer);
