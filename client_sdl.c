#include "client_sdl.h"

extern int sdl_pipe[2];
extern int status_pipe[2];
extern GameData *dataptr;

SDL_Window *win = NULL;
SDL_Renderer *ren = NULL;

bool init_sdl(const char *title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL 초기화 오류: %s\n", SDL_GetError());
        return -1;
    }

    win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    if (win == NULL) {
        fprintf(stderr, "창 생성 실패: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == NULL) {
        SDL_DestroyWindow(win);
        fprintf(stderr, "렌더러 생성 실패: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    return 0;
}

void run_sdl_client_loop() {
    printf("SDL 프로세스 시작\n");
    close(sdl_pipe[1]); // 쓰기 닫음
    close(status_pipe[0]); // 읽기 닫음

    if (init_sdl("octopus_game", 1280, 640) == -1) {
        perror("SDL 초기화 오류");
        exit(5);
    }

    SDL_RenderClear(ren);
    printf("SDL 준비 완료\n");

    // receive_sdl_command(sdl_pipe[0]); // 안에서 종료 명령을 입력 받을 때까지 계속 루프
}

void wait_response() {
    while (1) {
        char buffer[MSG_SIZE];
        if (read(status_pipe[0], buffer, MSG_SIZE - 1) == -1) {
            perror("[Parent] read");
            exit(1);
        }
        if (strncmp(buffer, "CMD_SDL_DONE", strlen("CMD_SDL_DONE")) == 0) {
            printf("[Parent] Received: %s\n", buffer);
            break;
        }
        sleep(1);
    }
}

void send_response() {
    if (write(status_pipe[1], "CMD_SDL_DONE", strlen("CMD_SDL_DONE") + 1) == -1) {
        perror("[SDL] write");
        exit(1);
    }
}