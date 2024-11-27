#include "client_sdl.h"

int sdl_pipe[2];
int status_pipe[2];

int main(void)
{
    pipe(sdl_pipe);
    pipe(status_pipe);

    pid_t sdl_pid = fork();
    if (sdl_pid == 0)
    {
        run_sdl_client_loop();
        exit(EXIT_SUCCESS);
    }


    close(sdl_pipe[0]); // 쓰기만
    close(status_pipe[1]); // 읽기만

    for (int i = 0; i < 3; i++)
    {
        sleep(3);
        char buffer[256];
        snprintf(buffer, 256, "OhMyGodWhatTheHellOhYeayea %d", i);
        write(sdl_pipe[1], buffer, 255);
    }

    close(sdl_pipe[1]);
    close(status_pipe[0]);

    wait(NULL);
    printf("Succeed!");
    return 0;
}
