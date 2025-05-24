#include <stdio.h>
#include <stdbool.h>
#include <windows.h>

// Key bit positions
enum {
    W_BIT = 0,
    A_BIT = 1,
    S_BIT = 2,
    D_BIT = 3,
    NUM_KEYS = 4
};

// SOCD pair indices
enum {
    W_TO_S = 0,
    S_TO_W = 1,
    A_TO_D = 2,
    D_TO_A = 3,
    NUM_PAIRS = 4
};

static const int SOCD_PAIRS[NUM_PAIRS][2] = {
    {W_BIT, S_BIT}, // W > S
    {S_BIT, W_BIT}, // S > W
    {A_BIT, D_BIT}, // A > D
    {D_BIT, A_BIT} // D > A
};

static const char *const KEY_NAMES[NUM_KEYS] = {"W", "A", "S", "D"};
static const char KEY_CODES[NUM_KEYS] = {'W', 'A', 'S', 'D'};

void gotoxy(const int x, const int y) {
    const COORD coord = {x, y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void clearLine(int y) {
    const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    const COORD coord = {0, y};
    DWORD written;
    FillConsoleOutputCharacter(hConsole, ' ', csbi.dwSize.X, coord, &written);
    SetConsoleCursorPosition(hConsole, coord);
}

double getHighResolutionTime(void) {
    static LARGE_INTEGER frequency = {0};
    static bool initialized = false;

    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        initialized = true;
    }

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double) now.QuadPart / (double) frequency.QuadPart;
}

void initializeConsole(void) {
    const HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    // Hide cursor
    const CONSOLE_CURSOR_INFO cursorInfo = {1, FALSE};
    SetConsoleCursorInfo(consoleHandle, &cursorInfo);

    // Set high priority for better timing accuracy
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    // Setup console
    system("cls");
    system("title SOCD tester");
    system("mode con: cols=61 lines=9");
}

void printInitialDisplay(void) {
    printf("WASD keystroke monitor:\n");
    printf("\n\n");
    printf("SOCD clearing delays (milliseconds):\n");
    printf(" [W] > [S]  0.000\n");
    printf(" [S] > [W]  0.000\n");
    printf(" [A] > [D]  0.000\n");
    printf(" [D] > [A]  0.000\n");
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

void updateKeyDisplay(unsigned char keys) {
    gotoxy(0, 1);
    clearLine(1);
    printf("> ");

    for (int i = 0; i < NUM_KEYS; i++) {
        if (keys & (1 << i)) {
            printf("[%s] ", KEY_NAMES[i]);
        }
    }
    fflush(stdout);
}

void updateDelayDisplay(const double delays[NUM_PAIRS]) {
    static const int delay_lines[] = {4, 5, 6, 7};
    static const char *const delay_labels[] = {
        " [W] > [S]  %.3f",
        " [S] > [W]  %.3f",
        " [A] > [D]  %.3f",
        " [D] > [A]  %.3f"
    };

    for (int i = 0; i < NUM_PAIRS; i++) {
        gotoxy(0, delay_lines[i]);
        printf(delay_labels[i], delays[i]);
    }
    fflush(stdout);
}

int main(void) {
    initializeConsole();
    printInitialDisplay();

    unsigned char last_keys = 0;
    double socd_start_time[NUM_PAIRS] = {0};
    double socd_delay[NUM_PAIRS] = {0};
    bool socd_measuring[NUM_PAIRS] = {false};

    while (true) {
        const unsigned char current_keys = getCurrentKeys();
        const double current_time = getHighResolutionTime();
        bool delays_updated = false;

        // Process SOCD measurements
        for (int pair = 0; pair < NUM_PAIRS; pair++) {
            const int first_key = SOCD_PAIRS[pair][0];
            const int second_key = SOCD_PAIRS[pair][1];
            const unsigned char first_mask = 1 << first_key;
            const unsigned char second_mask = 1 << second_key;

            // Start measuring when second key is pressed while first is held
            if ((current_keys & first_mask) &&
                (current_keys & second_mask) &&
                !(last_keys & second_mask) &&
                !socd_measuring[pair]) {
                socd_start_time[pair] = current_time;
                socd_measuring[pair] = true;
            }

            // Stop measuring when first key is released (SOCD cleared)
            if (socd_measuring[pair] && !(current_keys & first_mask)) {
                socd_delay[pair] = (current_time - socd_start_time[pair]) * 1000.0;
                socd_measuring[pair] = false;
                delays_updated = true;
            }

            // Cancel measurement if second key is released
            if (socd_measuring[pair] && !(current_keys & second_mask)) {
                socd_measuring[pair] = false;
            }
        }

        // Update displays only when necessary
        if (current_keys != last_keys) {
            updateKeyDisplay(current_keys);
        }

        if (delays_updated) {
            updateDelayDisplay(socd_delay);
        }

        last_keys = current_keys;
    }
}
