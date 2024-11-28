//
// Created by delp on 11/27/24.
//

#ifndef CLIENT_H
#define CLIENT_H
#include "client_common.h"

void run_client();
void* wait_game_running(void* dataptr);
void read_from_sdl(char *buffer);
void write_to_sdl(char *message);

// 1 ~ max(inclusive)
int get_random_int(int max);
// SIGTURNSTART Handler
void turn_start(int sig);
// SIGGAMEEND Handler
void game_end(int sig);

#endif //CLIENT_H
