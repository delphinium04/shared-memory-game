#include "client_sdl.h"

int main(void)
{
    pid_t sdl_pid = fork();
    if (sdl_pid == 0)
    {
        run_sdl_client_loop();
        exit(EXIT_SUCCESS);
    }

    wait(NULL);
    printf("Succeed!");
    return 0;
}
