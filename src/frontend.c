#include "frontend.h"
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <stddef.h>
#include <stdint.h>

#define NES_A 0x01
#define NES_B 0x02
#define NES_SELECT 0x04
#define NES_START 0x08
#define NES_UP 0x10
#define NES_DOWN 0x20
#define NES_LEFT 0x40
#define NES_RIGHT 0x80

#define AUDIO_BUFFER_SIZE 8192
static int16_t audio_buffer[AUDIO_BUFFER_SIZE];
static size_t write_pos = 0;
static size_t read_pos = 0;

const static int keyboard[8] = {
    SDL_SCANCODE_Z,  SDL_SCANCODE_X,    SDL_SCANCODE_C,    SDL_SCANCODE_V,
    SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT};

void audio_buffer_add(int16_t sample) {
  size_t next = (write_pos + 1) % AUDIO_BUFFER_SIZE;
  if (next != read_pos) {
    audio_buffer[write_pos] = sample;
    write_pos = next;
  }
}

void audio_callback(void *userdata, Uint8 *stream, int len) {
  int16_t *out = (int16_t *)stream;
  int samples = len / sizeof(int16_t);

  for (int i = 0; i < samples; ++i) {
    if (read_pos != write_pos) {
      out[i] = audio_buffer[read_pos];
      read_pos = (read_pos + 1) % AUDIO_BUFFER_SIZE;
    } else {
      out[i] = 0;
    }
  }
}

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
  frontend->w = w;
  frontend->h = h;

  SDL_AudioSpec spec = {
      .freq = 44100,
      .format = AUDIO_S16SYS,
      .channels = 1,
      .samples = 512,
      .callback = audio_callback,
      .userdata = NULL,
  };

  if (SDL_OpenAudio(&spec, NULL) < 0) {
    SDL_Log("SDL_OpenAudio failed: %s", SDL_GetError());
  }

  SDL_PauseAudio(0); // Start audio playback
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

int Frontend_HandleInput(Frontend *frontend) {
  SDL_PumpEvents();
  const Uint8 *keystate = SDL_GetKeyboardState(NULL);
  frontend->controller = 0;

  if (keystate[SDL_SCANCODE_Q])
    return 1;
  if (keystate[keyboard[0]])
    frontend->controller |= NES_A;
  if (keystate[keyboard[1]])
    frontend->controller |= NES_B;
  if (keystate[keyboard[2]])
    frontend->controller |= NES_SELECT;
  if (keystate[keyboard[3]])
    frontend->controller |= NES_START;
  if (keystate[keyboard[4]])
    frontend->controller |= NES_UP;
  if (keystate[keyboard[5]])
    frontend->controller |= NES_DOWN;
  if (keystate[keyboard[6]])
    frontend->controller |= NES_LEFT;
  if (keystate[keyboard[7]])
    frontend->controller |= NES_RIGHT;
  return 0;
}

void Frontend_Destroy(Frontend *frontend) {
  SDL_DestroyTexture(frontend->texture);
  SDL_DestroyRenderer(frontend->renderer);
  SDL_DestroyWindow(frontend->window);
  SDL_Quit();
}
