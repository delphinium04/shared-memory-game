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
SDL_Texture* load_texture(const char *file);
void render_texture(SDL_Texture *tex, int x, int y);
void render_player(int idx);

void assign_stage_position();
void run_sdl();
bool init_sdl();
void cleanup();

void write_to_client(char* message);
int read_from_client(char* buffer);

void update();
void render_mini_game();
void macro_show_game(SDL_Texture* texture);
void render_game_over();

#endif //CLIENT_SDL_H
