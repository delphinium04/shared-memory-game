#include "client_common.h"

GameData *dataptr;
char server_message[MSG_SIZE] = {'\0'};

void run_manager();

void set_game_running(GameData *dataptr, bool is_running);

void terminate(int shmid, char *message) {
    printf("terminate: %s\n", message);

    if (shmdt(dataptr) == -1) {
        perror("shmdt failed");
        exit(EXIT_FAILURE);
    }

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl failed");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

void initialize_data(void) {
    dataptr->pid[0] = -1;
    dataptr->pid[1] = -1;
    dataptr->pid_count = 0;
    dataptr->server_pid = getpid();

    dataptr->game_running = false;

    for (int i = 0; i < MAP_SIZE; i++) {
        dataptr->map_snake[i] = NOT_EXIST_WAY;
    }

    dataptr->minigame_time = false; //미니 게임 장소 세팅
    for (int i = 0; i < MAP_SIZE; i++) {
        dataptr->map_minigame[i] = NOT_MINI_GAME_ZONE;
    }
    
    dataptr->player_position[0] = 0;
    dataptr->player_position[1] = 0;
    dataptr->winner = -1;
    dataptr->current_turn = -1;
}

bool wait_players(int seconds) {
    int timer = 0;
    while (timer++ < seconds) {
        if (dataptr->pid_count == 2) {
            printf("waiting...\n");
            return true;
        }
        sleep(1);
    }
    return false;
}

int main(void) {
    int room_number = 0;
    printf("Create Room by ID: ");
    scanf("%d", &room_number);

    key_t key = room_number;
    int shmid = shmget(key, sizeof(GameData), 0666 | IPC_CREAT | IPC_EXCL);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    dataptr = shmat(shmid, NULL, 0); //공유 메모리 데이터 불러오기
    if (dataptr == (void *) -1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    initialize_data();

    bool is_game_valid = wait_players(30);
    if (is_game_valid == false) {
        terminate(shmid, "No Player Connected");
    }

    run_manager();

    printf("run_manager() end");

    // 서버는 클라이언트 대기 후 공유 메모리 제거
    waitpid(dataptr->pid[0], NULL, 0);
    waitpid(dataptr->pid[1], NULL, 0);

    terminate(shmid, "manager succeed");
}

int get_random_int(int max) {
    srand(getpid() + time(NULL));
    return rand() % max + 1;
}

void set_snake_ladder() {
    srand(time(NULL));
    Vector2 snake[] ={
        {3,8},
        {6, 15},
        {11,13},
        {18, 7},
        {22, 20},
        {24, 9}
    };

    for (int i=0; i<sizeof(snake) / sizeof(Vector2); i++)
    {
        dataptr->map_snake[snake[i].x] = snake[i].y;
    }
}
void set_mini_game_zone() {
    dataptr->map_minigame[2] = 1;
    dataptr->map_minigame[8] = 1;
    dataptr->map_minigame[14] = 1;
    dataptr->map_minigame[20] = 1;
    dataptr->map_minigame[25] = 1;
}

void set_turn() {
    // 게임 끝 확인
    for (int i = 0; i < 2; i++) {
        if (dataptr->player_position[i] >= MAP_SIZE-1) {
            dataptr->winner = dataptr->pid[i];
            for (int idx = 0; idx < dataptr->pid_count; idx++) {
                kill(dataptr->pid[idx], SIGGAMEOVER);
            }
            kill(getpid(), SIGGAMEOVER);
            break;
        }
    }

    if (dataptr->current_turn == dataptr->pid[0]) dataptr->current_turn = dataptr->pid[1];
    else dataptr->current_turn = dataptr->pid[0];

    printf("[%d]의 차례\n", dataptr->current_turn);
    kill(dataptr->current_turn, SIGTURNSTART);
    printf("kill SIGTURNSTART\n");
}

// SIGTURNEND Handler
void turn_end(int sig) {
    printf("[%d] 턴 종료\n", dataptr->current_turn);
    set_turn();
}

// SIGGAMEOVER Handler
void game_end(int sig) {
    dataptr->game_running = false;
    printf("게임 종료, [%d] 승리\n", dataptr->winner);
}

void run_manager() {
    signal(SIGTURNEND, turn_end);
    signal(SIGGAMEOVER, game_end);

    printf("게임 준비 중: [Server] %d / [Client] %d %d\n", dataptr->server_pid, dataptr->pid[0], dataptr->pid[1]);
    set_snake_ladder();
    set_mini_game_zone();
    sleep(2);

    dataptr->game_running = true;

    printf("Game started\n");
    sleep(1);
    set_turn(); // 턴 시작

    while (dataptr->game_running) {
        pause(); // 시그널 대기
    }
}
