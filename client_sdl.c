#include "client_sdl.h"

// sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
/*
 * client_common.h에 선언된 내용
 * extern int pipe_client_to_sdl[2];
 * extern int pipe_sdl_to_client[2];
 * extern GameData *dataptr;
 * 오류 case 제외하고 pipe close 금지
*/

SDL_Window *win = NULL;
SDL_Renderer *ren = NULL;
TTF_Font *font = NULL;


void run_sdl() {
    // 이제 할 것
    // 실시간으로 업데이트하지 않음.
    // 본인 턴이 왔을 때 게임 데이터 받아와서 직접 판단.

    // 로케일 설정, UTF-8 인코딩 사용
    setlocale(LC_ALL, "");

    printf("[SDL] 프로세스 시작\n");

    int width = 1280, height = 720;
    if (init_sdl("octopus game", width, height) == false) {
        perror("SDL 초기화 오류");
        exit(EXIT_FAILURE);
    }

    SDL_RenderClear(ren);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Texture *testImage = load_texture("test.png", ren);

    SDL_RenderClear(ren);
    render_texture(testImage, 640, 360, ren);
    render_text("Hello World!", 640, 360, white);
    SDL_RenderPresent(ren); // 렌더링한 내용을 화면에 표시

    printf("[SDL] 준비 완료\n");
    SDL_Event e;
    bool quit = false;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                quit = true;
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                printf("[SDL] Clicked\n");
                char input_buffer[MSG_SIZE];
                strcpy(input_buffer, "[MOUSE]");

                if (write(pipe_sdl_to_client[1], input_buffer, MSG_SIZE) == -1) {
                    perror("[SDL] pipe write failed");
                    exit(EXIT_FAILURE);
                };
            }
        }

        char buffer[MSG_SIZE];
        // 비블로킹 pipe를 read
        ssize_t count = read(pipe_client_to_sdl[0], buffer, MSG_SIZE);
        if (count == -1) {
            // 진짜 오류
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("[SDL] pipe read failed");
                cleanup(); // 리소스 정리 후 종료
                exit(EXIT_FAILURE);
            } else
                continue; // 데이터가 없는 경우 루프 계속
        }
        if (count > 0) {
            buffer[count] = '\0'; // 문자열 종료
            SDL_RenderClear(ren); // 이전 렌더링 내용 지우기
            render_texture(testImage, 640, 360, ren);
            render_text(buffer, 640, 360, white);
            SDL_RenderPresent(ren); // 새 내용 화면에 표시
        }
        // sleep(1);  // 너무 빠른 루프를 방지
        usleep(1000 * 100);
    }
    cleanup();
}

bool init_sdl(const char *title, int width, int height) {
    // 비디오 서브시스템 초기화 지시
    if (SDL_Init(SDL_INIT_VIDEO) == -1) {
        fprintf(stderr, "SDL 초기화 오류: %s\n", SDL_GetError());
        return false;
    }

    // 폰트
    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF 초기화 오류: %s\n", TTF_GetError());
        return false;
    }

    // 이미지
    if (IMG_Init(IMG_INIT_PNG) == 0) {
        fprintf(stderr, "IMG 초기화 오류: %s\n", IMG_GetError());
        return false;
    }

    win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    if (win == NULL) {
        fprintf(stderr, "창 생성 실패: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == NULL) {
        fprintf(stderr, "렌더러 생성 실패: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return false;
    }

    font = TTF_OpenFont("Pretendard.ttf", 24);
    if (font == NULL) {
        fprintf(stderr, "폰트 로드 실패: %s\n", TTF_GetError());
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    return true;
}


void render_text(const char *message, int x, int y, SDL_Color color) {
    // SDL_ttf에서 UTF-8 문자열 렌더링
    SDL_Surface *surface = TTF_RenderUTF8_Solid(font, message, color);
    if (!surface) {
        fprintf(stderr, "TTF_RenderUTF8_Solid 오류: %s\n", TTF_GetError());
        return;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(ren, surface);
    if (!texture) {
        fprintf(stderr, "SDL_CreateTextureFromSurface 오류: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }
    SDL_Rect dest = {x, y, surface->w, surface->h};

    SDL_FreeSurface(surface);

    SDL_RenderCopy(ren, texture, NULL, &dest);
    SDL_DestroyTexture(texture);
}

SDL_Texture *load_texture(const char *file, SDL_Renderer *ren) {
    SDL_Texture *texture = IMG_LoadTexture(ren, file);
    if (texture == NULL) {
        fprintf(stderr, "이미지 로드 실패: %s\n", IMG_GetError());
    }
    return texture;
}

void render_texture(SDL_Texture *tex, int x, int y, SDL_Renderer *ren) {
    SDL_Rect dst;
    dst.x = x;
    dst.y = y;
    SDL_QueryTexture(tex, NULL, NULL, &dst.w, &dst.h);
    SDL_RenderCopy(ren, tex, NULL, &dst);
}

void cleanup() {
    TTF_CloseFont(font);
    TTF_Quit();
    IMG_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
}
