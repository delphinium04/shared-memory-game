#include "client_sdl.h"

// sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev

// extern int sdl_pipe[2];
// extern int status_pipe[2];
// extern GameData *dataptr;

SDL_Window* win = NULL;
SDL_Renderer* ren = NULL;
TTF_Font* font = NULL;

void run_sdl_client_loop()
{
    printf("SDL 프로세스 시작\n");

    int width = 1280, height = 720;
    if (init_sdl("octopus game", width, height) == false)
    {
        perror("SDL 초기화 오류");
        exit(EXIT_FAILURE);
    }

    SDL_RenderClear(ren);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Texture *testImage = load_texture("test.png", ren);

    printf("SDL 준비 완료\n");
    SDL_Event e;
    bool quit = false;
    int x = 0;
    int y = 0;
    while (!quit)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                quit = true;
        }
        SDL_RenderClear(ren);
        SDL_Color white = {255, 255, 255, 255};
        render_texture(testImage, 1280 - x, 720 - y, ren);
        render_text("Hello World!", x++, y++, white);

        SDL_RenderPresent(ren); // 렌더링한 내용을 화면에 표시
    }
}


bool init_sdl(const char* title, int width, int height)
{
    // 비디오 서브시스템 초기화 지시
    if (SDL_Init(SDL_INIT_VIDEO) == -1)
    {
        fprintf(stderr, "SDL 초기화 오류: %s\n", SDL_GetError());
        return false;
    }

    // 폰트
    if (TTF_Init() == -1)
    {
        fprintf(stderr, "TTF 초기화 오류: %s\n", TTF_GetError());
        return false;
    }

    // 이미지
    if (IMG_Init(IMG_INIT_PNG) == 0)
    {
        fprintf(stderr, "IMG 초기화 오류: %s\n", IMG_GetError());
        return false;
    }

    win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    if (win == NULL)
    {
        fprintf(stderr, "창 생성 실패: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == NULL)
    {
        fprintf(stderr, "렌더러 생성 실패: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return false;
    }

    font = TTF_OpenFont("Pretendard.ttf", 24);
    if (font == NULL)
    {
        fprintf(stderr, "폰트 로드 실패: %s\n", TTF_GetError());
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    return true;
}


void render_text(const char* message, int x, int y, SDL_Color color)
{
    SDL_Surface* surface = TTF_RenderText_Solid(font, message, color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(ren, surface);
    SDL_Rect dest = {x, y, surface->w, surface->h};

    SDL_FreeSurface(surface);

    SDL_RenderCopy(ren, texture, NULL, &dest);
    SDL_DestroyTexture(texture);
}

SDL_Texture* load_texture(const char* file, SDL_Renderer* ren)
{
    SDL_Texture* texture = IMG_LoadTexture(ren, file);
    if (texture == NULL)
    {
        fprintf(stderr, "이미지 로드 실패: %s\n", IMG_GetError());
    }
    return texture;
}

void render_texture(SDL_Texture* tex, int x, int y, SDL_Renderer* ren)
{
    SDL_Rect dst;
    dst.x = x;
    dst.y = y;
    SDL_QueryTexture(tex, NULL, NULL, &dst.w, &dst.h);
    SDL_RenderCopy(ren, tex, NULL, &dst);
}


void cleanup()
{
    TTF_CloseFont(font);
    TTF_Quit();
    IMG_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
}

// void wait_response() {
//     while (1) {
//         char buffer[MSG_SIZE];
//         if (read(status_pipe[0], buffer, MSG_SIZE - 1) == -1) {
//             perror("[Parent] read");
//             exit(1);
//         }
//         if (strncmp(buffer, "CMD_SDL_DONE", strlen("CMD_SDL_DONE")) == 0) {
//             printf("[Parent] Received: %s\n", buffer);
//             break;
//         }
//         sleep(1);
//     }
// }

// void send_response() {
//     if (write(status_pipe[1], "CMD_SDL_DONE", strlen("CMD_SDL_DONE") + 1) == -1) {
//         perror("[SDL] write");
//         exit(1);
//     }
// }
