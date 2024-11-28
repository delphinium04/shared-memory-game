#include "client_sdl.h"

int pipe_client_to_sdl[2];
int pipe_sdl_to_client[2];

void clear_status_buffer() {
    while (read(pipe_sdl_to_client[0], NULL, MSG_SIZE) > 1);
}

int main(void)
{
    if (pipe(pipe_client_to_sdl) == -1 || pipe(pipe_sdl_to_client) == -1)
    exit(EXIT_FAILURE);

    // 비블로킹 설정 (비동기라 필수)
    fcntl(pipe_client_to_sdl[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_client_to_sdl[1], F_SETFL, O_NONBLOCK);
    fcntl(pipe_sdl_to_client[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_sdl_to_client[1], F_SETFL, O_NONBLOCK);

    pid_t sdl_pid = fork();
    if (sdl_pid == 0)
    {
        close(pipe_client_to_sdl[1]); // 읽기 전용
        close(pipe_sdl_to_client[0]); // 쓰기 전용

        run_sdl();
        exit(EXIT_SUCCESS);
    }

    close(pipe_client_to_sdl[0]); // 쓰기 전용
    close(pipe_sdl_to_client[1]); // 읽기 전용

    printf("3초 대기중\n");
    sleep(3);

    char buffer[MSG_SIZE];

    for (int i = 0; i < 3; i++)
    {
        printf("%d) 1초 후 클릭 대기\n", i+1);
        sleep(1);

        clear_status_buffer();
        write(pipe_client_to_sdl[1], "[CLIENT] 입력 대기 시작", MSG_SIZE);


        int count = 0;
        while (read(pipe_sdl_to_client[0], buffer, MSG_SIZE) <= 0);

        printf("입력 확인 %s\n", buffer);
        write(pipe_client_to_sdl[1], buffer, MSG_SIZE);
    }

    printf("성공\n");

    wait(NULL);

    close(pipe_client_to_sdl[1]);
    close(pipe_sdl_to_client[0]);

    printf("Succeed!");
    return 0;
}
