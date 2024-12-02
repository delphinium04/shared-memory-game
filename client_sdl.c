#include "client_sdl.h"

// sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
/*
 * client_common.h에 선언된 내용
 * extern int pipe_client_to_sdl[2];
 * extern int pipe_sdl_to_client[2];
 * extern GameData *dataptr;
 * extern int player_index;
 * 오류 case 제외하고 pipe close 금지
*/

SDL_Window *WIN = NULL;
SDL_Renderer *REN = NULL;

TTF_Font *FONT_DEFAULT = NULL;
SDL_Color COLOR_BLACK = {0, 0,0, 255};

SDL_Texture *TEX_BOARD = NULL;
SDL_Texture *TEX_GUARD = NULL;
SDL_Texture *TEX_P1_RIGHT = NULL;
SDL_Texture *TEX_P1_LEFT = NULL;
SDL_Texture *TEX_P2_RIGHT = NULL;
SDL_Texture *TEX_P2_LEFT = NULL;
SDL_Texture *Minigame_begin_texture = NULL;
SDL_Texture *Quiz_textures[8] = {NULL};
SDL_Texture *typing_textures[10] = {NULL};
SDL_Texture *updown_textures[11] = {NULL};
SDL_Texture *winlose_textures[4] = {NULL};

const char *winlose_imagePaths[4] = {"./src/1PWIN.png", "./src/1PLOSE.png", "./src/2PWIN.png", "./src/2PLOSE.png"};

const char *Quiz_imagePaths[8] = {
    "./src/octoquiz_0.png", "./src/octoquiz_1.png", "./src/octoquiz_2.png", "./src/octoquiz_3.png",
    "./src/octoquiz_4.png", "./src/octoquiz_5.png", "./src/octoquiz_corr.png", "./src/octoquiz_fail.png"
};

const char *typing_imagePaths[10] = {
    "./src/octotype_01.png", "./src/octotype_02.png", "./src/octotype_1.png",
    "./src/octotype_2.png", "./src/octotype_3.png", "./src/octotype_4.png", "./src/octotype_5.png",
    "./src/octotype_corr.png",
    "./src/octotype_fail.png", "./src/octotype_timeout.png"
};

const char *updown_imagePaths[11] = {
    "./src/octoupdown_1.png", "./src/octoupdown_2.png", "./src/octoupdown_3.png",
    "./src/octoupdown_4.png", "./src/octoupdown_5.png", "./src/octoupdown_corr1.png", "./src/octoupdown_corr2.png",
    "./src/octoupdown_down.png", "./src/octoupdown_up.png", "./src/octoupdown_fail.png", "./src/octoupdown_final.png"
};


const int TEX_PLAYER_WIDTH = 80, TEX_PLAYER_HEIGHT = 150;
const int SCREEN_WIDTH = 1280, SCREEN_HEIGHT = 720;
Vector2 stage_position[MAP_SIZE];

// 비블로킹 pipe reading 할 때 이전 내용 저장용 변수
char client_msg[MSG_SIZE];

bool init_sdl(const char *title) {
    // 비디오 서브시스템 초기화 지시
    if (SDL_Init(SDL_INIT_VIDEO) == -1) {
        fprintf(stderr, "SDL 초기화 오류: %s\n", SDL_GetError());
        return false;
    }

    WIN = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT,
                           SDL_WINDOW_SHOWN);
    if (WIN == NULL) {
        fprintf(stderr, "창 생성 실패: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    REN = SDL_CreateRenderer(WIN, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (REN == NULL) {
        fprintf(stderr, "렌더러 생성 실패: %s\n", SDL_GetError());
        SDL_DestroyWindow(WIN);
        SDL_Quit();
        return false;
    }

    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF 초기화 오류: %s\n", TTF_GetError());
        return false;
    }

    if (IMG_Init(IMG_INIT_PNG) == 0) {
        fprintf(stderr, "IMG 초기화 오류: %s\n", IMG_GetError());
        return false;
    }

    // src matching
    strcpy(client_msg, "DEBUG");

    FONT_DEFAULT = TTF_OpenFont("./src/Pretendard.ttf", 24);
    if (FONT_DEFAULT == NULL) {
        fprintf(stderr, "폰트 로드 실패: %s\n", TTF_GetError());
        cleanup();
        return false;
    }

    TEX_BOARD = load_texture("./src/board.png");
    if (TEX_BOARD == NULL) {
        fprintf(stderr, "./src/board.png 로드 실패: %s\n", IMG_GetError());
        cleanup();
    }

    TEX_GUARD = load_texture("./src/guard.png");
    if (TEX_GUARD == NULL) {
        fprintf(stderr, "./src/guard.png 로드 실패: %s\n", IMG_GetError());
        cleanup();
    }

    TEX_P1_LEFT = load_texture("./src/p1-left.png");
    if (TEX_P1_LEFT == NULL) {
        fprintf(stderr, "./src/p1-left.png 로드 실패: %s\n", IMG_GetError());
        cleanup();
    }

    TEX_P1_RIGHT = load_texture("./src/p1-right.png");
    if (TEX_P1_RIGHT == NULL) {
        fprintf(stderr, "./src/p1-right.png 로드 실패: %s\n", IMG_GetError());
        cleanup();
    }

    TEX_P2_LEFT = load_texture("./src/p2-left.png");
    if (TEX_P2_LEFT == NULL) {
        fprintf(stderr, "./src/p2-left.png 로드 실패: %s\n", IMG_GetError());
        cleanup();
    }

    TEX_P2_RIGHT = load_texture("./src/p2-right.png");
    if (TEX_P2_RIGHT == NULL) {
        fprintf(stderr, "./src/p2-right.png 로드 실패: %s\n", IMG_GetError());
        cleanup();
    }
    //미니게임 로드
    Minigame_begin_texture = load_texture("./src/begin1.png");
    if (TEX_P2_RIGHT == NULL) {
        fprintf(stderr, "./src/begin1.png 로드 실패: %s\n", IMG_GetError());
        cleanup();
    }

    for (int i = 0; i < sizeof(Quiz_imagePaths) / sizeof(Quiz_imagePaths[0]); i++) {
        Quiz_textures[i] = load_texture(Quiz_imagePaths[i]);
        if (Quiz_textures[i] == NULL) {
            fprintf(stderr, "Quiz_texture 로드 실패: %s\n", IMG_GetError());
            cleanup();
        }
    }
    for (int i = 0; i < sizeof(typing_imagePaths) / sizeof(typing_imagePaths[0]); i++) {
        typing_textures[i] = load_texture(typing_imagePaths[i]);
        if (typing_textures[i] == NULL) {
            fprintf(stderr, "typing_textures 로드 실패: %s\n", IMG_GetError());
            cleanup();
        }
    }
    for (int i = 0; i < sizeof(updown_imagePaths) / sizeof(updown_imagePaths[0]); i++) {
        updown_textures[i] = load_texture(updown_imagePaths[i]);
        if (updown_textures[i] == NULL) {
            fprintf(stderr, "updown_textures 로드 실패: %s\n", IMG_GetError());
            cleanup();
        }
    }
    for (int i = 0; i < sizeof(winlose_imagePaths) / sizeof(winlose_imagePaths[0]); i++) {
        winlose_textures[i] = load_texture(winlose_imagePaths[i]);
        if (winlose_textures[i] == NULL) {
            fprintf(stderr, "winlose_textures 로드 실패: %s\n", IMG_GetError());
            cleanup();
        }
    }
    return true;
}

void assign_stage_position() {
    const int start_y = 660;
    const int dy = 170;
    bool is_reversed = false; // false: left -> right

    for (int line = 0; line < 4; line++) {
        int start_x;
        if (is_reversed) start_x = 1015;
        else start_x = 265;

        for (int column = 0; column < 6; column++) {
            int dx = 150;
            if (is_reversed) dx *= -1;

            stage_position[line * 7 + column].x = start_x + column * dx;
            stage_position[line * 7 + column].y = start_y - line * dy;
        }

        if (is_reversed)
            stage_position[line * 7 - 1].x = 1060;
        else
            stage_position[line * 7 - 1].x = 205;
        stage_position[line * 7 - 1].y = start_y - 85 - (line - 1) * dy;
        is_reversed = is_reversed ? false : true;
    }

    for (int line = 0; line < 4; line++) {
        for (int column = 0; column <= 6; column++) {
            printf("[SDL] stage %d => %d, %d\n", line * 7 + column, stage_position[line * 7 + column].x,
                   stage_position[line * 7 + column].y);
        }
    }
}

void run_sdl() {
    // 로케일 설정, UTF-8 인코딩 사용
    setlocale(LC_ALL, "");
    printf("[SDL] 프로세스 시작\n");

    if (init_sdl("Octopus Game") == false) {
        perror("SDL 초기화 오류");
        exit(EXIT_FAILURE);
    }

    assign_stage_position();
    printf("[SDL] 준비 완료\n");

    // 입력 받기 전 화면 갱신
    SDL_RenderClear(REN);
    render_texture(TEX_BOARD, 0, 0);
    render_texture(TEX_P1_RIGHT, 100, 100);
    render_texture(TEX_P2_RIGHT, 300, 100);
    render_text("게임 준비 중...", 300, 360, COLOR_BLACK);
    SDL_RenderPresent(REN);

    sleep(5);
    SDL_Event e;
    bool quit = false;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                quit = true;
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                // 클릭 시 파이프 입력
                printf("[SDL] Clicked\n");
                write_to_client("[MOUSE]");
            }
        }

        if (dataptr->minigame_time) {
            render_mini_game();
        } else {
            update();
        }

        if (dataptr->game_running == false)
            break;
        // 100ms 단위로 Update
        SDL_Delay(100);
    }
    render_game_over(); // 3초간 승패 이미지 출력
    SDL_Delay(3000);

    cleanup();
}

void render_game_over() {
    SDL_RenderClear(REN);

    if (dataptr->pid[0] == getppid()) {
        //부모프로세스가 플레이어 1이면서
        if (dataptr->pid[0] == dataptr->winner) {
            //우승자일때
            render_texture(winlose_textures[0], 100, 0);
        } else {
            //우승자가 아닐때
            render_texture(winlose_textures[1], 100, 0);
        }
    }
    //부모 프로세스가 플레이어2이면서
    else {
        if (dataptr->pid[1] == dataptr->winner) {
            // 우승자 일때
            render_texture(winlose_textures[2], 100, 0);
        } else {
            //우승자가 아닐때
            render_texture(winlose_textures[3], 100, 0);
        }
    }
    SDL_RenderPresent(REN);
}

void render_text(const char *message, int x, int y, SDL_Color color) {
    // SDL_ttf에서 UTF-8 문자열 렌더링
    SDL_Surface *surface = TTF_RenderUTF8_Solid(FONT_DEFAULT, message, color);
    if (!surface) {
        fprintf(stderr, "TTF_RenderUTF8_Solid 오류: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(REN, surface);
    if (!texture) {
        fprintf(stderr, "SDL_CreateTextureFromSurface 오류: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }
    SDL_Rect dest = {x, y, surface->w, surface->h};

    SDL_FreeSurface(surface);

    SDL_RenderCopy(REN, texture, NULL, &dest);
    SDL_DestroyTexture(texture);
}

SDL_Texture *load_texture(const char *file) {
    SDL_Texture *texture = IMG_LoadTexture(REN, file);
    if (texture == NULL) {
        fprintf(stderr, "이미지 로드 실패: %s\n", IMG_GetError());
    }
    return texture;
}

void render_texture(SDL_Texture *tex, int x, int y) {
    SDL_Rect dst;
    dst.x = x;
    dst.y = y;
    SDL_QueryTexture(tex, NULL, NULL, &dst.w, &dst.h);
    SDL_RenderCopy(REN, tex, NULL, &dst);
}

void cleanup() {
    TTF_CloseFont(FONT_DEFAULT);
    TTF_Quit();

    SDL_DestroyTexture(TEX_BOARD);
    SDL_DestroyTexture(TEX_GUARD);
    SDL_DestroyTexture(TEX_P1_LEFT);
    SDL_DestroyTexture(TEX_P2_LEFT);
    SDL_DestroyTexture(TEX_P1_RIGHT);
    SDL_DestroyTexture(TEX_P2_RIGHT);
    IMG_Quit();

    SDL_DestroyRenderer(REN);
    SDL_DestroyWindow(WIN);
    SDL_Quit();
}

void update() {
    SDL_RenderClear(REN); // 이전 렌더링 내용 지우기
    render_texture(TEX_BOARD, 0, 0);

    // 플레이어 인덱스 + 위치 고려한 렌더링
    if (player_index == 0) {
        render_player(1);
        render_player(0);
    } else {
        render_player(0);
        render_player(1);
    }

    char *buffer;
    // 두 글자 이하는 출력 무시
    if (read_from_client(buffer) > 0 && strlen(buffer) > 1)
        strcpy(client_msg, buffer);
    render_text(client_msg, 450, 50, COLOR_BLACK);
    SDL_RenderPresent(REN); // 새 내용 화면에 표시
}

void render_mini_game() {
    char *buffer;
    if (read_from_client(buffer) > 0 && strlen(buffer) > 1) {
        //quiz
        if (strcmp(buffer, "quiz1") == 0) {
            macro_show_game(Quiz_textures[0]);
            macro_show_game(Minigame_begin_texture);
            macro_show_game(Quiz_textures[1]);
        } else if (strcmp(buffer, "quiz2") == 0) {
            macro_show_game(Quiz_textures[0]);
            macro_show_game(Minigame_begin_texture);
            macro_show_game(Quiz_textures[2]);
        } else if (strcmp(buffer, "quiz3") == 0) {
            macro_show_game(Quiz_textures[0]);
            macro_show_game(Minigame_begin_texture);
            macro_show_game(Quiz_textures[3]);
        } else if (strcmp(buffer, "quiz4") == 0) {
            macro_show_game(Quiz_textures[0]);
            macro_show_game(Minigame_begin_texture);
            macro_show_game(Quiz_textures[4]);
        } else if (strcmp(buffer, "quiz5") == 0) {
            macro_show_game(Quiz_textures[0]);
            macro_show_game(Minigame_begin_texture);
            macro_show_game(Quiz_textures[5]);
        } else if (strcmp(buffer, "quiz_success") == 0) {
            macro_show_game(Quiz_textures[6]);
        } else if (strcmp(buffer, "quiz_fail") == 0) {
            macro_show_game(Quiz_textures[7]);
        } //typing
        else if (strcmp(buffer, "typing1") == 0) {
            macro_show_game(typing_textures[0]);
            macro_show_game(typing_textures[1]);
            macro_show_game(typing_textures[2]);
        } else if (strcmp(buffer, "typing2") == 0) {
            macro_show_game(typing_textures[0]);
            macro_show_game(typing_textures[1]);
            macro_show_game(typing_textures[3]);
        } else if (strcmp(buffer, "typing3") == 0) {
            macro_show_game(typing_textures[0]);
            macro_show_game(typing_textures[1]);
            macro_show_game(typing_textures[4]);
        } else if (strcmp(buffer, "typing4") == 0) {
            macro_show_game(typing_textures[0]);
            macro_show_game(typing_textures[1]);
            macro_show_game(typing_textures[5]);
        } else if (strcmp(buffer, "typing5") == 0) {
            macro_show_game(typing_textures[0]);
            macro_show_game(typing_textures[1]);
            macro_show_game(typing_textures[6]);
        } else if (strcmp(buffer, "octotyping_corr") == 0) {
            macro_show_game(typing_textures[7]);
        } else if (strcmp(buffer, "octotyping_fail") == 0) {
            macro_show_game(typing_textures[8]);
        } else if (strcmp(buffer, "octotyping_timeout") == 0) {
            macro_show_game(typing_textures[9]);
        } //updown
        else if (strcmp(buffer, "octoupdown") == 0) {
            macro_show_game(updown_textures[0]);
            macro_show_game(updown_textures[1]);
            macro_show_game(updown_textures[2]);
            macro_show_game(updown_textures[3]);
            macro_show_game(updown_textures[4]);
        } else if (strcmp(buffer, "octoupdown_up") == 0) {
            macro_show_game(updown_textures[8]);
        } else if (strcmp(buffer, "octoupdown_down") == 0) {
            macro_show_game(updown_textures[7]);
        } else if (strcmp(buffer, "octoupdown_corr1") == 0) {
            macro_show_game(updown_textures[5]);
        } else if (strcmp(buffer, "octoupdown_final") == 0) {
            macro_show_game(updown_textures[10]);
        } else if (strcmp(buffer, "octoupdown_corr2") == 0) {
            macro_show_game(updown_textures[6]);
        } else if (strcmp(buffer, "octoupdown_fail") == 0) {
            macro_show_game(updown_textures[9]);
        }
    }
}

void macro_show_game(SDL_Texture *texture) {
    if (texture != NULL) {
        SDL_RenderClear(REN);
        render_texture(texture, 100, 0);
        SDL_RenderPresent(REN);
        usleep(1500000); // 1.5초 대기
    } else {
        fprintf(stderr, "texture 로드 실패: %s\n", IMG_GetError());
        cleanup();
    }
}

void render_player(int idx) {
    int p_pos = dataptr->player_position[idx];
    int p_x = stage_position[p_pos].x;
    int p_y = stage_position[p_pos].y;

    bool is_reversed;

    if ((p_pos / 7) % 2 == 0) is_reversed = false;
    else is_reversed = true;

    if (idx == 0 && is_reversed)
        render_texture(TEX_P1_LEFT, p_x - TEX_PLAYER_WIDTH / 2,
                       p_y - TEX_PLAYER_HEIGHT);
    else if (idx == 0 && !is_reversed)
        render_texture(TEX_P1_RIGHT, p_x - TEX_PLAYER_WIDTH / 2,
                       p_y - TEX_PLAYER_HEIGHT);
    else if (idx == 1 && is_reversed)
        render_texture(TEX_P2_LEFT, p_x - TEX_PLAYER_WIDTH / 2,
                       p_y - TEX_PLAYER_HEIGHT);
    else
        render_texture(TEX_P2_RIGHT, p_x - TEX_PLAYER_WIDTH / 2, p_y - TEX_PLAYER_HEIGHT);
}

void write_to_client(char *message) {
    if (write(pipe_sdl_to_client[1], message, MSG_SIZE) == -1) {
        perror("[SDL] pipe write failed");
        exit(EXIT_FAILURE);
    }
}

int read_from_client(char *buffer) {
    ssize_t count = read(pipe_client_to_sdl[0], buffer, MSG_SIZE);
    if (count == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        // 진짜 오류일 경우
        perror("[SDL] pipe read failed");
        cleanup(); // 리소스 정리 후 종료
        exit(EXIT_FAILURE);
    }

    return count;
}
