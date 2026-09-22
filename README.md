# 2024 서버프로그래밍입문

## 빌드

이 게임은 Linux의 System V 공유 메모리, `fork()`, POSIX 시그널을 사용합니다.
Windows용 MSVC로 직접 빌드할 수 없으며, Windows에서는 WSL의 Ubuntu에서 빌드하세요.

Ubuntu 터미널에서 빌드 도구와 SDL2 개발 패키지를 설치합니다.

```bash
sudo apt-get update
sudo apt-get install build-essential cmake pkg-config libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
```

프로젝트 폴더에서 빌드합니다. Windows의 `C:\project\shared-memory-game`은
WSL에서 `/mnt/c/project/shared-memory-game`입니다.

```bash
cd /mnt/c/project/shared-memory-game
cmake -S . -B build-linux
cmake --build build-linux -j
```

일반 Linux에서는 `cd` 경로를 실제 프로젝트 위치로 바꾸세요.
Windows에서 생성한 `out/build/x64-Debug` 캐시는 재사용하지 않습니다.

## 실행

클라이언트 빌드 시 이미지, 글꼴, 퀴즈 파일을 실행 파일 옆의 `src/`에 자동 복사합니다.
클라이언트는 실행 파일 위치를 기준으로 리소스를 읽으므로 어느 폴더에서 실행해도 됩니다.
실행 파일을 다른 위치로 옮길 때는 옆의 `src/` 폴더도 함께 옮기세요.
GUI를 표시할 수 있는 Linux 데스크톱 또는 WSLg 환경이 필요합니다.

**모두 빌드** 또는 `cmake --build build-linux -j`가 성공하면 매니저 1개와
클라이언트 2개를 자동 실행합니다. 방 번호는 자동으로 지정되며,
클라이언트 게임 창 2개가 열립니다. 빌드 자체는 게임 종료를 기다리지 않습니다.
`linux-debug` 프리셋으로 빌드할 때도 동일하게 동작합니다.

같은 빌드 폴더에서 실행한 게임이 아직 동작 중이면 중복 실행하지 않습니다.
자동 실행한 게임은 한쪽 창을 닫으면 두 클라이언트와 매니저를 함께 종료합니다.
종료 후 다시 빌드하면 새 게임이 실행됩니다.
매니저 출력과 각 클라이언트 출력은 빌드 폴더의 `game-session/` 아래
`manager.log`, `client-1.log`, `client-2.log`에 기록합니다.
GUI 환경이 없으면 빌드만 완료하고 실행을 건너뜁니다.

실행 중인 세 프로그램을 모두 종료하려면 Ubuntu 터미널에서 다음을 실행합니다.
프리셋 사용 시 `build-linux`를 `out/build/linux-debug`로 바꾸세요.

```bash
kill "$(cat build-linux/game-session/session.pid)"
```

자동 실행 없이 빌드하려면 다음과 같이 설정합니다. 다시 켜려면 `OFF`를 `ON`으로 바꾸세요.

```bash
cmake -S . -B build-linux -DGAME_AUTO_START=OFF
cmake --build build-linux -j
```

수동 실행 시 각각 별도의 터미널에서 매니저 하나와 클라이언트 두 개를 실행하고 같은 방 번호를 입력하세요.
매니저는 플레이어 접속을 30초 동안 기다립니다.

```bash
./build-linux/manager
# 두 번째와 세 번째 터미널에서 각각 실행
./build-linux/client
```

## Visual Studio의 CMake 오류

`FindPackageHandleStandardArgs.cmake`는 패키지를 찾지 못했을 때 오류를 표시하는
CMake 내부 파일입니다. 이 파일을 수정하지 마세요.
`Could NOT find PkgConfig` 또는 `PKG_CONFIG_EXECUTABLE-NOTFOUND`는
현재 빌드 환경에서 `pkg-config`를 찾지 못했다는 뜻입니다.

Visual Studio에서는 **Linux 및 임베디드 개발(C++)** 구성 요소를 설치하고
Windows `x64-Debug` 대신 Linux/WSL 대상을 사용하세요. 의존성도 해당 Ubuntu 안에
설치해야 합니다. 또는 위 Ubuntu 터미널 명령으로 직접 빌드할 수 있습니다.
Windows에 SDL2와 pkg-config만 설치하는 것으로는 Linux API 의존성이 해결되지 않습니다.

[Microsoft의 Visual Studio WSL 빌드 안내](https://learn.microsoft.com/en-us/cpp/build/walkthrough-build-debug-wsl2?view=msvc-170)

### WSL을 선택했는데도 Linux 전용 오류가 발생할 때

대상 시스템만 WSL로 바꾸고 `x64-Debug` 구성을 유지하면 Windows 빌드 캐시가
재사용될 수 있습니다. 프로젝트에 포함된 `CMakePresets.json`으로 빌드 폴더를 분리합니다.

1. Visual Studio에서 폴더를 닫았다가 다시 엽니다.
2. 대상 시스템은 `WSL: Ubuntu`, 구성은 `linux-debug` (`Linux Debug (WSL)`)를 선택합니다.
3. 프로젝트를 구성하고 모두 빌드합니다.

구성이 나타나지 않으면 도구 > 옵션 > CMake > 일반에서 `CMakePresets.json` 사용을
활성화하고 폴더를 다시 여세요. Linux 구성에서도 예전 오류가 남으면
해당 구성의 CMake 캐시를 삭제하고 다시 구성하세요.

같은 구성을 Ubuntu 터미널에서도 사용할 수 있습니다(CMake 3.21 이상).

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug -j
```

이 구성의 실행 파일은 `out/build/linux-debug/manager`와
`out/build/linux-debug/client`입니다. 프로젝트 루트에서 실행하세요.

## 메모리 오류 회귀 검사

Ubuntu의 프로젝트 폴더에서 `bash tests/run_memory_tests.sh`를 실행합니다.
보드 좌표와 양방향 파이프 메시지를 AddressSanitizer/UndefinedBehaviorSanitizer로
검사하며, SDL 창이나 게임 서버를 실행하지 않습니다.
