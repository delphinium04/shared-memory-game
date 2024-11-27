#include "client.h"
#include "client_sdl.h"

int SDL_pipe[2]; // Client To SDL_Child
int SDL_status_pipe[2]; // SDL_child to Client
GameData* dataptr;
int player_index;
int fd;
bool is_turn;

// Exit game after shmat succeed (Not declared in header)
void terminate(char* message)
{
    printf("terminate: %s\n", message);
    if (shmdt(dataptr) == -1)
    {
        perror("shmdt failed");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

bool add_pid(pid_t pid)
{
    if (dataptr->pid_count < 2)
    {
        dataptr->pid[dataptr->pid_count] = pid;
        player_index = dataptr->pid_count;
        dataptr->pid_count++;
        return true;
    }

    return false;
}

bool wait_another_player(int seconds)
{
    printf("waiting...\n");
    int timer = 0;
    while (timer++ < seconds)
    {
        if (dataptr->pid_count == 2)
        {
            return true;
        }
        sleep(1);
    }
    return false;
}


int main(void)
{
    // 메모리 초기 세팅
    int room_number;
    printf("%d -> room ID: ", getpid());
    scanf("%d", &room_number);

    key_t key = room_number;
    int room_memory_id = shmget(key, sizeof(GameData), 0 | 0666);
    if (room_memory_id == -1)
    {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    printf("Get shared memory success\n");

    dataptr = shmat(room_memory_id, 0, 0);
    if (dataptr == (void*)-1)
    {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    printf("Join shared memory success\n");

    // 게임 초기 세팅
    bool try_add_pid = add_pid(getpid());
    if (try_add_pid == false)
    {
        terminate("최대 플레이어 접속");
    }

    bool wait_player = wait_another_player(30); // 상대할 플레이어 찾기

    if (wait_player == false)
    {
        printf("상대를 찾지 못해 클라이언트를 종료합니다.\n");
        shmdt(dataptr);
        exit(0);
    }

    // Set named pipe from manager.c

    // Set SDL
    if (pipe(SDL_pipe) == -1 || pipe(SDL_status_pipe) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t sdl_pid = fork();
    if (sdl_pid == 0)
    {
        run_sdl_client_loop();
        exit(EXIT_SUCCESS);
    }

    close(SDL_pipe[0]); // 쓰기만
    close(SDL_status_pipe[1]); // 읽기만

    run_client();
    printf("run_client() end");

    wait(NULL);

    close(SDL_pipe[1]);
    close(SDL_status_pipe[0]);
    terminate("client succeed");
}

int get_random_int(int max)
{
    return rand() % max + 1;
}

// SIGTURNSTART Handler
void turn_start(int sig)
{
    is_turn = true; // 현재 턴 플래그 설정
}


// SIGGAMEEND Handler
void game_end(int sig)
{
    if (dataptr->winner == getpid())
    {
        printf("게임이 끝났습니다! 축하드립니다, 승리하셨습니다!\n");
    }
    else
    {
        printf("게임이 끝났습니다. 승리자는 [%d]입니다!\n", dataptr->winner);
    }

    // fifo pipe 닫기
    close(fd);
}


// Client 로직
void client_game()
{
    char buffer[MSG_SIZE];
    while (dataptr->game_running)
    {
        if (is_turn)
        {
            is_turn = false; // 턴 진행 플래그 초기화

            write_to_sdl_child("당신 차례입니다, Y를 입력해서 주사위를 굴리세요!\n");
            int ch;
            while ((ch = getchar()) != 'Y') continue;

            int dice = get_random_int(6);

            int current_position = dataptr->player_position[player_index];
            current_position += dice;

            // do minigame (case: succeed)

            // 뱀, 사다리 array length 때문에 if 필수
            if (current_position <= 100)
            {
                if (dataptr->map_ladder[current_position] != NOT_EXIST_WAY)
                {
                    snprintf(buffer, MSG_SIZE - 1, "사다리를 탔습니다! %d->%d\n", current_position,
                             dataptr->map_ladder[current_position]);
                    current_position = dataptr->map_ladder[current_position];
                }
                else if (dataptr->map_snake[current_position] != NOT_EXIST_WAY)
                {
                    snprintf(buffer, MSG_SIZE - 1, "뱀을 탔습니다! %d->%d\n", current_position,
                             dataptr->map_ladder[current_position]);
                    current_position = dataptr->map_snake[current_position];
                }
                write_to_sdl_child(buffer);
            }

            dataptr->player_position[player_index] = current_position;

            snprintf(buffer, MSG_SIZE - 1, "주사위 결과: %d | 현재 위치: %d\n", dice, current_position);
            write_to_sdl_child(buffer);

            kill(dataptr->server_pid, SIGTURNEND);
        }

        // 시그널 대기
        pause();
    }
}

// AI 사용, mutex와 cond에 대한 지식 필요
void* wait_game_running(void* data)
{
    GameData* dataptr = (GameData*)data;
    pthread_mutex_lock(&dataptr->lock);
    while (!dataptr->game_running)
    {
        pthread_cond_wait(&dataptr->cond, &dataptr->lock);
    }
    printf("Game is now running!\n");
    pthread_mutex_unlock(&dataptr->lock);
    return NULL;
}

void run_client()
{
    is_turn = false;
    signal(SIGTURNSTART, turn_start);
    signal(SIGGAMEOVER, game_end);

    if (player_index == 0)
        fd = open(dataptr->fifo_p0_path, O_RDONLY);
    else
        fd = open(dataptr->fifo_p1_path, O_RDONLY);
    if (fd == -1)
    {
        perror("open fifo failed");
        return;
    }

    printf("[Server:%d] 게임 준비 중\n", dataptr->server_pid);

    // 쓰레드 처리로 바꿀 예정
    // pthread_t wait_thread;
    // int res = pthread_create(&wait_thread, NULL, wait_game_running, (void*)dataptr);
    // if (res != 0) {
    //     fprintf(stderr, "Error creating thread: %d\n", res);
    //     return;
    // }
    // res = pthread_join(wait_thread, NULL);
    // if (res != 0) {
    //     fprintf(stderr, "Error joining thread: %d\n", res);
    //     return;
    // }

    printf("waiting for game running thread to finish\n");
    if (dataptr->game_running == false)
    {
        while (dataptr->game_running == false)
        {
            printf("waiting...\n");
        }
        printf("game running thread finished\n");
    }

    client_game();
}

void write_to_sdl_child(char* message)
{
    char buffer[MSG_SIZE];
    snprintf(buffer, MSG_SIZE - 1, message);
    printf("%s", message);

    if (write(SDL_pipe[1], buffer, MSG_SIZE - 1) == -1)
    {
        perror("write failed");
        exit(EXIT_FAILURE);
    };
}
