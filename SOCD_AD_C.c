#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h> 
#include <stdbool.h>
#include <stdint.h>  
#include <stdlib.h>

#ifdef __GNUC__
    #define HOT_PATH __attribute__((hot))
    #define LIKELY(x) __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define FORCE_INLINE __attribute__((always_inline)) inline
#else
    #define HOT_PATH
    #define LIKELY(x) (x)
    #define UNLIKELY(x) (x)
    #define FORCE_INLINE __forceinline
#endif

#define VK_A 'A'
#define VK_D 'D'
#define SIMULATED_INPUT_MARKER 0xFF515700
#define RAWINPUT_BUFFER_SIZE 128

#define A_DOWN_BIT     0x01  // Bit 0: A key physical state
#define D_DOWN_BIT     0x02  // Bit 1: D key physical state
#define LAST_KEY_BIT   0x04  // Bit 2: Last pressed (0=A, 1=D)
#define DIR_NEUTRAL    0x00  // Direction in bits 3-4: neutral
#define DIR_LEFT       0x08  // Direction in bits 3-4: left (A)
#define DIR_RIGHT      0x10  // Direction in bits 3-4: right (D)
#define DIR_MASK       0x18  // Mask for direction bits
#define SIM_BIT        0x20  // Bit 5: Simulating state

typedef uint8_t SOCDState;

static SOCDState g_state = 0;

static const INPUT press_a_input = {
    .type = INPUT_KEYBOARD,
    .ki = {.wVk = VK_A, .dwFlags = 0, .dwExtraInfo = SIMULATED_INPUT_MARKER}
};

static const INPUT release_a_input = {
    .type = INPUT_KEYBOARD,
    .ki = {.wVk = VK_A, .dwFlags = KEYEVENTF_KEYUP, .dwExtraInfo = SIMULATED_INPUT_MARKER}
};

static const INPUT press_d_input = {
    .type = INPUT_KEYBOARD,
    .ki = {.wVk = VK_D, .dwFlags = 0, .dwExtraInfo = SIMULATED_INPUT_MARKER}
};

static const INPUT release_d_input = {
    .type = INPUT_KEYBOARD,
    .ki = {.wVk = VK_D, .dwFlags = KEYEVENTF_KEYUP, .dwExtraInfo = SIMULATED_INPUT_MARKER}
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static HOT_PATH FORCE_INLINE void process_key_event(UINT vkey, bool is_down);
static FORCE_INLINE void cleanup_resources(HWND hwnd, const wchar_t* class_name, HINSTANCE hinstance);

int main(void) {
    const wchar_t CLASS_NAME[] = L"RawInputInjectorSilentClass";
    const HINSTANCE hinstance = GetModuleHandle(NULL);
    HWND hwnd = NULL;

    const WNDCLASSW wc = {
        .lpfnWndProc = WindowProc,
        .hInstance = hinstance,
        .lpszClassName = CLASS_NAME
    };

    if (!RegisterClassW(&wc)) {
        fprintf(stderr, "Error: Failed to register window class. Code: %lu\n", GetLastError());
        return 1;
    }

    hwnd = CreateWindowExW(0, CLASS_NAME, L"SOCD Injector", 0, 0, 0, 0, 0,
                          HWND_MESSAGE, NULL, hinstance, NULL);

    if (hwnd == NULL) {
        fprintf(stderr, "Error: Failed to create message-only window. Code: %lu\n", GetLastError());
        cleanup_resources(NULL, CLASS_NAME, hinstance);
        return 1;
    }

    const RAWINPUTDEVICE rid = {
        .usUsagePage = 0x01, // Generic Desktop
        .usUsage = 0x06, // Keyboard
        .dwFlags = RIDEV_INPUTSINK | RIDEV_NOLEGACY,
        .hwndTarget = hwnd
    };

    if (!RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE))) {
        fprintf(stderr, "Error: Failed to register raw input device. Code: %lu\n", GetLastError());
        cleanup_resources(hwnd, CLASS_NAME, hinstance);
        return 1;
    }

    system("title SOCD software resolver");
    system("mode con: cols=70 lines=3");
    printf("SOCD resolution is currently active. Keep this process active.\n");
    fflush(stdout);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_state & SIM_BIT) {
        const uint8_t dir = g_state & DIR_MASK;
        if (dir == DIR_LEFT) {
            SendInput(1, (INPUT*)&release_a_input, sizeof(INPUT));
        } else if (dir == DIR_RIGHT) {
            SendInput(1, (INPUT*)&release_d_input, sizeof(INPUT));
        }
    }

    cleanup_resources(hwnd, CLASS_NAME, hinstance);
    return (int)msg.wParam;
}

static FORCE_INLINE void cleanup_resources(const HWND hwnd, const wchar_t* class_name, const HINSTANCE hinstance) {
    if (hwnd) DestroyWindow(hwnd);
    if (class_name && hinstance) UnregisterClassW(class_name, hinstance);
}

LRESULT CALLBACK WindowProc(const HWND hwnd, const UINT uMsg, const WPARAM wParam, const LPARAM lParam) {
    if (uMsg == WM_INPUT) {
        UINT dwSize = 0;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
        if (UNLIKELY(dwSize == 0)) return 0;

        BYTE stack_buffer[RAWINPUT_BUFFER_SIZE];
        BYTE* buffer = (dwSize <= RAWINPUT_BUFFER_SIZE) ? stack_buffer : malloc(dwSize);

        if (LIKELY(buffer && GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize)) {
            const RAWINPUT* raw = (RAWINPUT*)buffer;
          
            if (LIKELY(raw->header.dwType == RIM_TYPEKEYBOARD &&
                       raw->data.keyboard.ExtraInformation != SIMULATED_INPUT_MARKER)) {

                const UINT vkey = raw->data.keyboard.VKey;
                if (LIKELY(vkey == VK_A || vkey == VK_D)) {
                    const bool is_down = !(raw->data.keyboard.Flags & RI_KEY_BREAK);
                    process_key_event(vkey, is_down);
                }
            }
        }

        if (UNLIKELY(buffer != stack_buffer)) free(buffer);
        return 0;
    } else if (uMsg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static HOT_PATH FORCE_INLINE void process_key_event(const UINT vkey, const bool is_down) {
    const uint8_t old_dir = g_state & DIR_MASK;

    const bool is_a_key = (vkey == VK_A);
    const uint8_t key_bit = is_a_key ? A_DOWN_BIT : D_DOWN_BIT;

    if (is_down) {
        g_state |= key_bit;  // Set key bit
        g_state = (g_state & ~LAST_KEY_BIT) |  // Update last pressed key
                  (is_a_key ? 0 : LAST_KEY_BIT);
    } else {
        g_state &= ~key_bit;  // Clear key bit
    }

    // Calculate new direction 
    const uint8_t physical_state = g_state & (A_DOWN_BIT | D_DOWN_BIT);
    uint8_t new_dir;

    if (physical_state == 0) {
        new_dir = DIR_NEUTRAL;
    } else if (physical_state == (A_DOWN_BIT | D_DOWN_BIT)) {
        new_dir = (g_state & LAST_KEY_BIT) ? DIR_RIGHT : DIR_LEFT;
    } else {
        // Only one key pressed
        new_dir = (physical_state & A_DOWN_BIT) ? DIR_LEFT : DIR_RIGHT;
    }

    if ((g_state & DIR_MASK) == new_dir) return;

    const bool need_simulate = physical_state == (A_DOWN_BIT | D_DOWN_BIT) || g_state & SIM_BIT;

    if (need_simulate) {
        if (old_dir == DIR_LEFT && new_dir != DIR_LEFT) {
            SendInput(1, (INPUT*)&release_a_input, sizeof(INPUT));
        } else if (old_dir == DIR_RIGHT && new_dir != DIR_RIGHT) {
            SendInput(1, (INPUT*)&release_d_input, sizeof(INPUT));
        }

        if (new_dir == DIR_LEFT && old_dir != DIR_LEFT) {
            SendInput(1, (INPUT*)&press_a_input, sizeof(INPUT));
        } else if (new_dir == DIR_RIGHT && old_dir != DIR_RIGHT) {
            SendInput(1, (INPUT*)&press_d_input, sizeof(INPUT));
        }

        g_state = g_state & ~SIM_BIT | (new_dir != DIR_NEUTRAL ? SIM_BIT : 0);
    } else {
        g_state &= ~SIM_BIT;
    }
    g_state = g_state & ~DIR_MASK | new_dir;
}
