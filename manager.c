#include "client_common.h"

GameData *dataptr;
char server_message[MSG_SIZE] = {'\0'};
int fd1, fd2; // Manager -> Client pipe

void run_manager();

void set_game_running(GameData *dataptr, bool is_running);

void terminate(int shmid, char *message) {
    printf("terminate: %s\n", message);

    if (strcmp(dataptr->fifo_p0_path, '\0') == 0) {
        unlink(dataptr->fifo_p0_path);
    }
    if (strcmp(dataptr->fifo_p1_path, '\0') == 0) {
        unlink(dataptr->fifo_p1_path);
    }

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

    dataptr->fifo_p0_path[0] = '\0';
    dataptr->fifo_p1_path[0] = '\0';

    dataptr->game_running = false;

    for (int i = 0; i < MAP_SIZE; i++) {
        dataptr->map_snake[i] = NOT_EXIST_WAY;
    }

    dataptr->player_position[0] = 0;
    dataptr->player_position[1] = 0;
    dataptr->winner = -1;
    dataptr->current_turn = -1;

    pthread_mutex_init(&dataptr->lock, NULL);
    pthread_cond_init(&dataptr->cond, NULL);
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

    // 명명 파이프 초기 세팅 [/tmp/방번호_fifo_p{1,2}]
    char *fifo_pipe_name;
    sprintf(fifo_pipe_name, "/tmp/%d_fifo_p1", room_number);
    strcpy(dataptr->fifo_p0_path, fifo_pipe_name);
    if (mkfifo(fifo_pipe_name, 0666) == -1) {
        perror("mkfifo failed");
        terminate(shmid, "mkfifo_p1 failed");
    }

    sprintf(fifo_pipe_name, "/tmp/%d_fifo_p2", room_number);
    strcpy(dataptr->fifo_p1_path, fifo_pipe_name);
    if (mkfifo(fifo_pipe_name, 0666) == -1) {
        perror("mkfifo failed");
        terminate(shmid, "mkfifo_p2 failed");
    }

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
    set_game_running(dataptr, false);
    printf("게임 종료, [%d] 승리\n", dataptr->winner);

    // fifo pipe 닫기
    close(fd1);
    close(fd2);
}

// AI 사용, mutex와 cond에 대한 지식 필요
void set_game_running(GameData *dataptr, bool is_running) {
    pthread_mutex_lock(&dataptr->lock);
    dataptr->game_running = is_running;
    pthread_cond_signal(&dataptr->cond);
    pthread_mutex_unlock(&dataptr->lock);
}

void run_manager() {
    signal(SIGTURNEND, turn_end);
    signal(SIGGAMEOVER, game_end);

    fd1 = open(dataptr->fifo_p0_path, O_WRONLY);
    if (fd1 == -1) {
        perror("open fifo failed");
        return;
    }

    fd2 = open(dataptr->fifo_p1_path, O_WRONLY);
    if (fd2 == -1) {
        perror("open fifo failed");
        return;
    }

    printf("게임 준비 중: [Server] %d / [Client] %d %d\n", dataptr->server_pid, dataptr->pid[0], dataptr->pid[1]);
    set_snake_ladder();
    sleep(2);

    pthread_mutex_lock(&dataptr->lock);
    dataptr->game_running = true;
    pthread_cond_broadcast(&dataptr->cond); // 모든 클라이언트를 깨웁니다.
    pthread_mutex_unlock(&dataptr->lock);

    printf("Game started\n");
    sleep(1);
    set_turn(); // 턴 시작

    while (dataptr->game_running) {
        pause(); // 시그널 대기
    }
}
