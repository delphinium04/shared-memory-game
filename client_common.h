//
// Created by delp on 11/27/24.
// common things related to game
//

#ifndef CLIENT_COMMON_H
#define CLIENT_COMMON_H

#define MAP_SIZE 27
#define MSG_SIZE 256
#define NOT_EXIST_WAY -1
#define NOT_MINI_GAME_ZONE -1 //미니게임

#define SIGTURNSTART SIGUSR1
#define SIGTURNEND SIGUSR2
#define SIGGAMEOVER SIGINT

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <wait.h>
#include <pthread.h>


typedef struct
{
    int pid[2];
    int pid_count;
    int server_pid;

    bool game_running;
    bool minigame_time; //미니게임에 돌입 했는지
    int map_snake[MAP_SIZE];
    int map_minigame[MAP_SIZE]; //미니게임이 실행되는 장소
    int player_position[2];
    int current_turn;
    int winner;
} GameData;

typedef struct
{
    int x;
    int y;
} Vector2;

extern int pipe_client_to_sdl[2];
extern int pipe_sdl_to_client[2];
extern int player_index; // 0 or 1
extern GameData *dataptr;

#endif //CLIENT_COMMON_H
