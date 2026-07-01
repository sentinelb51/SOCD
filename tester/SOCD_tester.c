#include <stdio.h>
#include <stdbool.h>
#include <windows.h>

// Key bit positions
enum { W_BIT, A_BIT, S_BIT, D_BIT, NUM_KEYS };

#define NUM_PAIRS 4

// {held key, opposing key} — measured from opposing press to held release
static const int SOCD_PAIRS[NUM_PAIRS][2] = {
    {W_BIT, S_BIT},
    {S_BIT, W_BIT},
    {A_BIT, D_BIT},
    {D_BIT, A_BIT}
};

static const char KEY_CODES[NUM_KEYS] = {'W', 'A', 'S', 'D'};

static LONGLONG qpc_frequency;

void gotoxy(const int x, const int y) {
    const COORD coord = {x, y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void clearLine(const int y) {
    const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    const COORD coord = {0, y};
    DWORD written;
    FillConsoleOutputCharacter(hConsole, ' ', csbi.dwSize.X, coord, &written);
    SetConsoleCursorPosition(hConsole, coord);
}

void initializeConsole(void) {
    const HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    // Hide cursor
    const CONSOLE_CURSOR_INFO cursorInfo = {1, FALSE};
    SetConsoleCursorInfo(consoleHandle, &cursorInfo);

    // Max out scheduling priority; REALTIME needs admin and silently
    // falls back to HIGH without it
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    // Pin to one core so the poll loop never migrates mid-measurement
    SetThreadAffinityMask(GetCurrentThread(), 1);

    // Setup console
    SetConsoleTitleA("SOCD tester");
    system("cls");
    system("mode con: cols=61 lines=9");
}

unsigned char getCurrentKeys(void) {
    unsigned char keys = 0;
    for (int i = 0; i < NUM_KEYS; i++) {
        if (GetAsyncKeyState(KEY_CODES[i]) & 0x8000) {
            keys |= (1 << i);
        }
    }
    return keys;
}

void updateKeyDisplay(const unsigned char keys) {
    clearLine(1);
    printf("> ");

    for (int i = 0; i < NUM_KEYS; i++) {
        if (keys & (1 << i)) {
            printf("[%c] ", KEY_CODES[i]);
        }
    }
    fflush(stdout);
}

void updateDelayDisplay(const double delays[NUM_PAIRS]) {
    for (int i = 0; i < NUM_PAIRS; i++) {
        gotoxy(0, 4 + i);
        printf(" [%c] > [%c]  %.3f", KEY_CODES[SOCD_PAIRS[i][0]],
               KEY_CODES[SOCD_PAIRS[i][1]], delays[i]);
    }
    fflush(stdout);
}

void printInitialDisplay(void) {
    printf("WASD keystroke monitor:\n\n\n");
    printf("SOCD clearing delays (milliseconds):\n");
    updateDelayDisplay((double[NUM_PAIRS]){0});
}

int main(void) {
    initializeConsole();
    printInitialDisplay();

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    qpc_frequency = freq.QuadPart;

    unsigned char last_keys = 0;
    LONGLONG socd_start[NUM_PAIRS] = {0};
    double socd_delay[NUM_PAIRS] = {0};
    bool socd_measuring[NUM_PAIRS] = {false};
    bool keys_dirty = false;
    bool delays_dirty = false;

    while (true) {
        const unsigned char keys = getCurrentKeys();
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        bool measuring_any = false;

        for (int pair = 0; pair < NUM_PAIRS; pair++) {
            const unsigned char first = 1 << SOCD_PAIRS[pair][0];
            const unsigned char second = 1 << SOCD_PAIRS[pair][1];

            if (!socd_measuring[pair]) {
                // Start when the second key lands while the first is held
                if ((keys & first) && (keys & second) && !(last_keys & second)) {
                    socd_start[pair] = now.QuadPart;
                    socd_measuring[pair] = true;
                }
            } else if (!(keys & first)) {
                // First key released: SOCD cleared
                socd_delay[pair] = (double) (now.QuadPart - socd_start[pair])
                                   * 1000.0 / (double) qpc_frequency;
                socd_measuring[pair] = false;
                delays_dirty = true;
            } else if (!(keys & second)) {
                // Second key released: measurement aborted
                socd_measuring[pair] = false;
            }

            measuring_any |= socd_measuring[pair];
        }

        keys_dirty |= keys != last_keys;
        last_keys = keys;

        // Console writes cost whole milliseconds; rendering mid-measurement
        // would pollute the reading, so defer until every pair is idle
        if (!measuring_any) {
            if (keys_dirty) {
                updateKeyDisplay(keys);
                keys_dirty = false;
            }
            if (delays_dirty) {
                updateDelayDisplay(socd_delay);
                delays_dirty = false;
            }
        }
    }
}
