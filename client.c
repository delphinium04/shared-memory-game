#include "client.h"
#include "client_sdl.h"

#pragma region HEADER_GLOBAL_VARIABLES
int pipe_client_to_sdl[2];
int pipe_sdl_to_client[2];
int player_index;
GameData* dataptr;
Quiz quizzes[MAX_QUIZZES];
int quiz_count;
#pragma  endregion  HEADER_GLOBAL_VARIABLES

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

    // Set SDL
    if (pipe(pipe_client_to_sdl) == -1 || pipe(pipe_sdl_to_client) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // 비블로킹 설정
    fcntl(pipe_client_to_sdl[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_client_to_sdl[1], F_SETFL, O_NONBLOCK);
    fcntl(pipe_client_to_sdl[0], F_SETFL, O_NONBLOCK);
    fcntl(pipe_sdl_to_client[1], F_SETFL, O_NONBLOCK);

    pid_t sdl_pid = fork();
    if (sdl_pid == -1)
    {
        perror("fork failed???????????WTF");
        exit(EXIT_FAILURE);
    }

    if (sdl_pid == 0)
    {
        close(pipe_client_to_sdl[1]); // 읽기 전용
        close(pipe_sdl_to_client[0]); // 쓰기 전용

        run_sdl();
        exit(EXIT_SUCCESS);
    }

    close(pipe_client_to_sdl[0]); // 쓰기 전용
    close(pipe_sdl_to_client[1]); // 읽기 전용

    run_client();
    printf("run_client() end");

    wait(NULL);
    close(pipe_client_to_sdl[1]); // 쓰기 해제
    close(pipe_sdl_to_client[0]); // 읽기 해제

    terminate("client succeed");
}

// 게임 로직
void client_game()
{
    char buffer[MSG_SIZE];
    _Bool pass;
    while (dataptr->game_running)
    {
        if (is_turn)
        {
            is_turn = false; // 턴 진행 플래그 초기화
            write_to_sdl("당신 차례입니다, 화면을 클릭해서 주사위를 굴리세요!");

            read_from_sdl(buffer); // 대기

            int dice = get_random_int(6);
            int current_position = dataptr->player_position[player_index] += dice;
            if (current_position > 27) current_position = 26;

  
            if (dataptr->map_snake[current_position] != NOT_EXIST_WAY)
            {
               sleep(2);
               snprintf(buffer, MSG_SIZE, "문어 다리를 탔습니다! %d->%d", current_position,
               dataptr->map_snake[current_position]);
               current_position = dataptr->map_snake[current_position];
               dataptr->player_position[player_index] = current_position;
               write_to_sdl(buffer);  
            }

            if (dataptr->map_minigame[current_position] != NOT_MINI_GAME_ZONE) {
                sleep(2);
                dataptr->minigame_time = true; //미니게임 시작
                pass = start_mini_game(); //미니 게임 결과
                dataptr->minigame_time = false; //미니 게임 종료
                int random_poition = get_random_int(2);
                if (pass) { //성공 했을 경우
                    snprintf(buffer, MSG_SIZE, "미니게임에 성공하여 앞으로 %d칸 전진!", random_poition);
                    current_position += random_poition;
                    if (current_position > 27) current_position = 26;
                    dataptr->player_position[player_index] = current_position;
                    write_to_sdl(buffer);
                }
                else { // 실패 했을 경우
                    snprintf(buffer, MSG_SIZE, "미니게임 실패 ㅠㅠ(뒤로 %d칸 후진..)", random_poition);
                    current_position -= random_poition;
                    dataptr->player_position[player_index] = current_position;
                    write_to_sdl(buffer);
                }
            }

            sleep(2);
            snprintf(buffer, MSG_SIZE, "주사위 결과: %d | 현재 위치: %d", dice, current_position);
            write_to_sdl(buffer);
        
            kill(dataptr->server_pid, SIGTURNEND);
        }

        // 시그널 대기
        pause();
    }
}

void run_client()
{
    is_turn = false;
    signal(SIGTURNSTART, turn_start);
    signal(SIGGAMEOVER, game_end);

    printf("[Server:%d] 게임 준비 중\n", dataptr->server_pid);


    if (dataptr->game_running == false) //?
    {
        while (dataptr->game_running == false)
        {
            printf("waiting...\n");
            usleep(1000 * 100); // 100ms
        }
    }

    client_game();
}

void read_from_sdl(char* buffer)
{
    int read_bytes = 1;

    printf("[Client] 입력 대기 시작:read_from_sdl()\n");
    while (true)
    {
        read_bytes = read(pipe_sdl_to_client[0], buffer, MSG_SIZE);
        if (read_bytes > 0)
        {
            buffer[read_bytes] = '\0';
            break;
        }
        if (read_bytes == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            perror("파이프 읽기 오류");
            close(pipe_client_to_sdl[1]);
            close(pipe_sdl_to_client[0]);
            exit(EXIT_FAILURE);
        }
        usleep(1000 * 100); // 100ms sleep
    }
    printf("[SDL->Client] %s\n", buffer);
}

void write_to_sdl(char* message)
{
    char buffer[MSG_SIZE];
    strcpy(buffer, message);
    printf("[Client->SDL] %s\n", message);

    if (write(pipe_client_to_sdl[1], buffer, MSG_SIZE) == -1)
    {
        perror("write failed");
        exit(EXIT_FAILURE);
    };
}

// return 1~max(inclusive)
int get_random_int(int max)
{
    srand(getpid() + time(NULL));
    return rand() % max + 1;
}

// SIGTURNSTART Handler
void turn_start(int sig)
{
    is_turn = true; // 현재 턴 플래그 설정
}
// SIGGAMEEND Handler
void game_end(int sig) {}

_Bool start_mini_game() {
    _Bool player_won;
    switch(rand() % 3){
        case 2:
            player_won = random_quiz();
            break;
        case 1:
            player_won = typing();
            break;
        case 0:
            player_won = updown();
            break;
    }
    return player_won;
}

_Bool load_quiz_from_file(const char* filename, Quiz quizzes[]) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("파일을 열 수 없습니다");
        return 0;
    }

    char line[MAX_LINE_LENGTH];
    int count = 0;

    while (fgets(line, sizeof(line), file) && count < MAX_QUIZZES) {
        Quiz quiz;
        char* token = strtok(line, ",");

        // 문제 파싱
        if (token == NULL) continue;
        strncpy(quiz.problem, token, sizeof(quiz.problem) - 1);

        // 예시 답변 파싱
        for (int i = 0; i < 4; i++) {
            token = strtok(NULL, ",");
            if (token == NULL) continue;
            strncpy(quiz.answer_list[i], token, sizeof(quiz.answer_list[i]) - 1);
        }

        // 정답 인덱스 파싱
        token = strtok(NULL, ",");
        if (token == NULL) continue;
        quiz.answer_index = atoi(token);

        quizzes[count++] = quiz;
    }

    fclose(file);
    quiz_count = count;
    return 1;
}

_Bool random_quiz() {
    load_quiz_from_file("./src/problem.csv", quizzes);
    int random_quiz_index = rand() % quiz_count;
    Quiz quiz = quizzes[random_quiz_index];
    if (random_quiz_index == 0) {
        write_to_sdl("quiz1");
    }
    else if (random_quiz_index == 1) {
        write_to_sdl("quiz2");
    }
    else if (random_quiz_index == 2) {
        write_to_sdl("quiz3");
    }
    else if (random_quiz_index == 3) {
        write_to_sdl("quiz4");
    }
    else if (random_quiz_index == 4) {
        write_to_sdl("quiz5");
    }
    sleep(5);
    printf("숫자를 입력해주세요! : ");

    int user_input = 0;
    scanf("%d", &user_input);
    while (getchar() != '\n'); //입력 버퍼 지우기
    if (user_input == quiz.answer_index) {
        write_to_sdl("quiz_success"); //sdl한테 성공 했다고 보여달라고 하기
        return true;
    }

    write_to_sdl("quiz_fail");  //sdl한테 실패 했다고 보여달라 하기
    return false;
}

_Bool typing(){
    int qnum; // 문제 결정 변수
    char ans[100]; // 문제 답변 및 정답 여부 확인용 배열
    const char* questions[5] = {
        "공부를 플레이하는 게임전공", //octotype_1
        "폭력은 협상과 꾀를 대체할 수 없다",//octotype_2
        "영부터 시작하는 정수 자료형", //octotype_3
        "문어 제 다리 뜯어먹는 격", //octotype_4
        "대답은 짧아야 덜 성가신 법이다" //octotype_5
    };
    
    srand(time(0));
    qnum = rand() % 5; // 랜덤 문제 결정
    const char* qstr = questions[qnum];
    if (qnum == 0) {
        write_to_sdl("typing1");
    }
    else if (qnum == 1) {
        write_to_sdl("typing2");
    }
    else if (qnum == 2) {
        write_to_sdl("typing3");
    }
    else if (qnum == 3) {
        write_to_sdl("typing4");
    }
    else if (qnum == 4) {
        write_to_sdl("typing5");
    }
    sleep(4);
    printf("타이핑을 시작하세요!\n");
    while (getchar() != '\n');
    // 10초 제한 타이머 설정
    time_t start = time(NULL);
    fgets(ans, sizeof(ans), stdin); // 안전한 입력을 위해 fgets 사용
    time_t end = time(NULL);

    // fgets로 인해 입력에 붙는 개행 문자를 제거
    ans[strcspn(ans, "\n")] = '\0';

    if (difftime(end, start) > 10.0) {
        write_to_sdl("octotype_timeout");
        return false;
    }

    if (!strcmp(qstr, ans)) {
        write_to_sdl("octotyping_corr");
        printf("octotyping_corr\n");
        return true;
    }
    else {
        write_to_sdl("octotyping_fail"); //타이핑 실패 보여달라고 하기
        printf("octotyping_fail\n");
        return false;
    }
}

_Bool updown() {
    int question_number;
    int user_input; // 문제 답변 및 정답 여부 확인용 변수
    srand(time(0));
    question_number = rand() % 50 + 1; //랜덤 문제 결정
    write_to_sdl("octoupdown");
    sleep(5);
    for (int i = 0; i < 5; i++) {
        printf("숫자를 입력하세요! : ");
        scanf("%d", &user_input);
        while (getchar() != '\n');
        if (user_input > question_number) {
            write_to_sdl("octoupdown_down");
        }
        else if (user_input < question_number) {
            write_to_sdl("octoupdown_up");
        }
        else {
            break;
        }
    }

    if (user_input == question_number) {
        write_to_sdl("octoupdown_corr1");
        return true;
    }

    write_to_sdl("octoupdown_final");
    sleep(4);
    scanf("%d", &user_input);
    printf("숫자를 입력하세요! : ");
    while (getchar() != '\n');
    if ((question_number - 5 <= user_input) && (user_input <= question_number + 5)) {
        write_to_sdl("octoupdown_corr2");
        return true;
    }
    write_to_sdl("octoupdown_fail");
    return false;
}
