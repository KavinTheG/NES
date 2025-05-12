#include "renderer.h"

void Renderer_Init(Renderer *renderer, int w, int h, int scale) {
    SDL_Init(SDL_INIT_VIDEO);
    renderer->window = SDL_CreateWindow("NES",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        w * scale, h * scale, SDL_WINDOW_SHOWN);


    renderer->renderer = SDL_CreateRenderer(renderer->window, -1, SDL_RENDERER_ACCELERATED);

    renderer->texture = SDL_CreateTexture(renderer->renderer,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
        w, h);
    SDL_SetTextureBlendMode(renderer->texture, SDL_BLENDMODE_BLEND);  // Enable alpha blending
    SDL_RenderSetLogicalSize(renderer->renderer, w, h);
}

void Renderer_DrawFrame(Renderer *renderer, uint32_t frame_buffer[renderer->h][renderer->w]) {
    void* pixels;
    int pitch;
    SDL_LockTexture(renderer->texture, NULL, &pixels, &pitch);
    memcpy(pixels, frame_buffer, 240 * pitch); // 240 rows
    SDL_UnlockTexture(renderer->texture);

    SDL_RenderClear(renderer->renderer);
    SDL_RenderCopy(renderer->renderer, renderer->texture, NULL, NULL);
    SDL_RenderPresent(renderer->renderer);
}


void Renderer_Destroy(Renderer *renderer) {
    SDL_DestroyTexture(renderer->texture);
    SDL_DestroyRenderer(renderer->renderer);
    SDL_DestroyWindow(renderer->window);
    SDL_Quit();
}
