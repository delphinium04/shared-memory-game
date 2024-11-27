//
// Created by delp on 11/27/24.
//

#ifndef CLIENT_SDL_H
#define CLIENT_SDL_H

#include "client_common.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <locale.h>

void render_text(const char *message, int x, int y, SDL_Color color);
SDL_Texture* load_texture(const char *file, SDL_Renderer *ren);
void render_texture(SDL_Texture *tex, int x, int y, SDL_Renderer *ren);

void run_sdl_client_loop();
bool init_sdl();
void cleanup();

#endif //CLIENT_SDL_H
