#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdbool.h>
#include <stdio.h>

#define SIM_MARKER 0xFF515700

enum { KEY_A, KEY_D, KEY_COUNT };

static const WORD VK[KEY_COUNT] = {'A', 'D'};

static bool real[KEY_COUNT]; // Physical key state
static bool simulated[KEY_COUNT]; // Key state applications see
static int last_pressed = KEY_A;

static void send_key(const int key, const bool down)
{
    INPUT input = {
        .type = INPUT_KEYBOARD,
        .ki = {
            .wVk = VK[key],
            .wScan = (WORD)MapVirtualKeyW(VK[key], MAPVK_VK_TO_VSC),
            .dwFlags = down ? 0 : KEYEVENTF_KEYUP,
            .dwExtraInfo = SIM_MARKER,
        },
    };
    SendInput(1, &input, sizeof input);
    simulated[key] = down;
}

// Injects whatever events are needed to make applications see `want`.
static void sync_visible(const bool want[KEY_COUNT])
{
    for (int k = 0; k < KEY_COUNT; k++)
    {
        if (simulated[k] != want[k]) send_key(k, want[k]);
    }
}

static void on_key_event(const int key, const bool down)
{
    real[key] = down;
    simulated[key] = down; // the physical event itself passes through to apps
    if (down) last_pressed = key;

    // A key should be visible while held, unless the opposite key is also held and was pressed more recently.
    bool want[KEY_COUNT];
    for (int k = 0; k < KEY_COUNT; k++)
    {
        want[k] = real[k] && (!real[1 - k] || last_pressed == k);
    }
    sync_visible(want);
}

static LRESULT CALLBACK window_proc(const HWND hwnd, const UINT msg, const WPARAM wparam, const LPARAM lparam)
{
    switch (msg)
    {
    case WM_INPUT:
        {
            RAWINPUT raw;
            UINT size = sizeof raw;
            if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, &raw, &size,
                                sizeof(RAWINPUTHEADER)) == (UINT)-1)
            {
                return 0;
            }
            const RAWKEYBOARD* kb = &raw.data.keyboard;
            if (raw.header.dwType == RIM_TYPEKEYBOARD &&
                kb->ExtraInformation != SIM_MARKER &&
                (kb->VKey == VK[KEY_A] || kb->VKey == VK[KEY_D]))
            {
                on_key_event(kb->VKey == VK[KEY_D] ? KEY_D : KEY_A,
                             !(kb->Flags & RI_KEY_BREAK));
            }
            return 0;
        }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}

int main(void)
{
    const wchar_t CLASS_NAME[] = L"SOCDResolverClass";
    const HINSTANCE hinstance = GetModuleHandleW(NULL);

    const WNDCLASSW wc = {
        .lpfnWndProc = window_proc,
        .hInstance = hinstance,
        .lpszClassName = CLASS_NAME,
    };
    if (!RegisterClassW(&wc))
    {
        fprintf(stderr, "Error: failed to register window class (code %lu)\n", GetLastError());
        return 1;
    }

    const HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"SOCD Resolver", 0, 0, 0, 0, 0,
                                      HWND_MESSAGE, NULL, hinstance, NULL);
    if (!hwnd)
    {
        fprintf(stderr, "Error: failed to create message-only window (code %lu)\n", GetLastError());
        return 1;
    }

    const RAWINPUTDEVICE rid = {
        .usUsagePage = 0x01, // Gen desktop
        .usUsage = 0x06, // Keyboard
        .dwFlags = RIDEV_INPUTSINK, // Receive input irregardless of focus
        .hwndTarget = hwnd,
    };
    if (!RegisterRawInputDevices(&rid, 1, sizeof rid))
    {
        fprintf(stderr, "Error: failed to register raw input device (code %lu)\n", GetLastError());
        return 1;
    }

    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    SetConsoleTitleW(L"SOCD software resolver");
    printf("SOCD resolution is currently active. Keep this process active.\n");

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    sync_visible(real);
    return 0;
}
