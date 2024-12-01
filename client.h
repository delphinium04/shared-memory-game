//
// Created by delp on 11/27/24.
//

#ifndef CLIENT_H
#include "client_common.h"
#define CLIENT_H
#define MAX_LINE_LENGTH 1024
#define MAX_QUIZZES 100

typedef struct Quiz {
    char problem[MSG_SIZE];
    char answer_list[4][MSG_SIZE];
    int answer_index;
}Quiz;

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

_Bool typing();
_Bool random_quiz();
_Bool updown();
_Bool start_mini_game();
#endif //CLIENT_H
