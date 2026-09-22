#include "client_sdl.h"
#include "client.h"
#include <assert.h>

extern Vector2 stage_position[MAP_SIZE];

int main(int argc, char **argv)
{
    assert(argc == 2);
    if (strcmp(argv[1], "coordinates") == 0) {
        assign_stage_position();
        assert(stage_position[0].x == 265 && stage_position[0].y == 660);
        assert(stage_position[6].x == 1060 && stage_position[6].y == 575);
        assert(stage_position[13].x == 205 && stage_position[13].y == 405);
        assert(stage_position[20].x == 1060 && stage_position[20].y == 235);
        assert(stage_position[26].x == 265 && stage_position[26].y == 150);
    } else if (strcmp(argv[1], "pipes") == 0) {
        char buffer[MSG_SIZE];
        assert(pipe(pipe_sdl_to_client) == 0);
        write_to_client("[MOUSE]");
        read_from_sdl(buffer);
        assert(strcmp(buffer, "[MOUSE]") == 0);
        close(pipe_sdl_to_client[0]);
        close(pipe_sdl_to_client[1]);

        assert(pipe(pipe_client_to_sdl) == 0);
        write_to_sdl("quiz1");
        assert(read_from_client(buffer) == MSG_SIZE);
        assert(strcmp(buffer, "quiz1") == 0);
        char full_message[MSG_SIZE];
        memset(full_message, 'x', sizeof(full_message));
        assert(write(pipe_client_to_sdl[1], full_message, MSG_SIZE) == MSG_SIZE);
        assert(read_from_client(buffer) == MSG_SIZE);
        assert(buffer[MSG_SIZE - 1] == '\0');
        close(pipe_client_to_sdl[0]);
        close(pipe_client_to_sdl[1]);
    } else {
        return 2;
    }
    return 0;
}
