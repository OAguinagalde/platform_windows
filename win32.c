#include "win32.h"

// Clears the console associated with the stdout
void win32_clear_console() {
    HANDLE consoleStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    // First we have to activate the virtual terminal processing for this to work
    // We might want to keep the original mode to restore it if necessary. For example when using with other command lines utilities...
    DWORD mode = 0;
    DWORD originalMode = 0;
    GetConsoleMode(consoleStdOut, &mode);
    originalMode = mode;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(consoleStdOut, mode);
    // 2J only clears the visible window and 3J only clears the scroll back.
    PCWSTR clearConsoleSequence = L"\x1b[2J";
    WriteConsole(consoleStdOut, clearConsoleSequence, sizeof(clearConsoleSequence)/sizeof((clearConsoleSequence)[0]), NULL, NULL);
    // Restore original mode
    SetConsoleMode(consoleStdOut, originalMode);
}

// Prints a string to stdout
void win32_print(const char* str) {
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), (const void*) str, lstrlen((LPCSTR) str), NULL, NULL);
}

// printf but wrong lol
// Max output is 1024 bytes long!
#define WIN32_PRINTF_MAX_BUFFER_SIZE 1024
void win32_printf(const char* format, ...) {
    // static const size_t buffer_size = 1024;
    static char buffer[WIN32_PRINTF_MAX_BUFFER_SIZE];
    va_list args;
    va_start(args, format);
    // https://docs.microsoft.com/en-us/windows/win32/menurc/strsafe-ovw
    // https://docs.microsoft.com/en-us/windows/win32/api/strsafe/nf-strsafe-stringcbvprintfa
    StringCbVPrintfA(buffer, WIN32_PRINTF_MAX_BUFFER_SIZE, format, args);
    va_end(args);
    win32_print(buffer);
}

// Given a char buffer, and a format str, puts the resulting str in the buffer
void win32_format_buffer(char* buffer, const char* format, ...) {
    va_list args;
    va_start(args, format);
    wvsprintf((LPSTR) buffer, format, args);
    va_end(args);
}

// Given a windowHandle, queries the width, height and position (x, y) of the window
void win32_get_window_size_and_position(HWND windowHandle, int* width, int* height, int* x, int* y, bool printDebug) {
    RECT rect;
    GetWindowRect(windowHandle, &rect);
    *x = rect.left;
    *y = rect.top;
    *width = rect.right - rect.left;
    *height = rect.bottom - rect.top;
    if (printDebug) {
        win32_printf("Window at %d,%d with size %dx%d\n", *x, *y, *width, *height);
    }
}

// Given a windowHandle, queries the width and height of the client size (The drawable area)
void win32_get_client_size(HWND windowHandle, int* width, int* height, bool printDebug) {
    RECT rect;
    // This just gives us the "drawable" part of the window
    GetClientRect(windowHandle, &rect);
    *width = rect.right - rect.left;
    *height = rect.bottom - rect.top;
    if (printDebug) {
        win32_printf("Client size %dx%d\n", *width, *height);
    }
}

// Try to get a console, for situations where there might not be one
// Return true when an external consolle (new window) has been allocated
bool win32_get_console() {
    bool consoleFound = false;
    bool consoleIsExternal = false;
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        // Situation example:
        // Opening a Windows Subsystem process from a cmd.exe process. It the process will attach to cmd.exe's console
        consoleFound = true;
        win32_printf("Console hijacked!\n");
    }
    else {
        DWORD error = GetLastError();
        switch (error) {
            // If the calling process is already attached to a console, the error code returned is ERROR_ACCESS_DENIED.
            case ERROR_ACCESS_DENIED: {
                // Already attached to a console so that's it
                consoleFound = true;
            } break;
            // If the specified process does not have a console, the error code returned is ERROR_INVALID_HANDLE.
            case ERROR_INVALID_HANDLE: {
                
            } break;
            // If the specified process does not exist, the error code returned is ERROR_INVALID_PARAMETER. 
            case ERROR_INVALID_PARAMETER: {
                win32_printf("Unreachable at %s, %s\n", __FUNCTIONW__, __FILE__);
            } break;
        }

    }

    if (!consoleFound) {
        // If we still don't have a console then create a new one
        if (AllocConsole()) {
            // Creates a new console
            consoleFound = true;
            consoleIsExternal = true;
        }
        else {
            // AllocConsole function fails if the calling process already has a console
            win32_printf("Unreachable at %s, %s\n", __FUNCTIONW__, __FILE__);
        }
    }
    
    return consoleIsExternal;
}

WNDCLASS win32_make_window_class(const char* windowClassName, WNDPROC pfnWindowProc, HINSTANCE hInstance) {
    WNDCLASS windowClass = {0};
    windowClass.lpfnWndProc = pfnWindowProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = windowClassName;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&windowClass);
    return windowClass;
}

HWND win32_make_window(const char* windowClassName, const char* title, HINSTANCE hInstance, int nCmdShow) {
    int windowPositionX = 100;
    int windowPositionY = 100;
    int windowWidth = 0;
    int windowHeight = 0;
    int clientWidth = 0;
    int clientHeight = 0;
    // The size for the window will be the whole window and not the drawing area, so we will have to adjust it later on and it doesn't matter much here
    HWND windowHandle = CreateWindowEx(0, windowClassName, title, WS_POPUP | WS_OVERLAPPED | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU  | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        windowPositionX, windowPositionY, 10, 10,
        NULL, NULL, hInstance, NULL
    );
    win32_printf("Window created\n");
    win32_get_window_size_and_position(windowHandle,&windowWidth,&windowHeight,&windowPositionX,&windowPositionY,false);
    
    // let's figure out the real size of the client area (drawable part of window) and adjust it
    int desiredClientWidth = 500;
    int desiredClientHeight = 600;
    // Get client size
    win32_get_client_size(windowHandle, &clientWidth, &clientHeight, false);
    // Calculate difference between initial size of window and current size of drawable area, that should be the difference to make the window big enough to have our desired drawable area
    int difference_w = abs(clientWidth - desiredClientWidth);
    int difference_h = abs(clientHeight - desiredClientHeight);
    // Set the initially desired position and size now
    MoveWindow(windowHandle, windowPositionX, windowPositionY, windowWidth + difference_w, windowHeight + difference_h, 0);
    // It should have the right size about now
    win32_printf("Window adjusted\n");
    win32_get_window_size_and_position(windowHandle, &windowWidth, &windowHeight, &windowPositionX, &windowPositionY, true);
    win32_get_client_size(windowHandle, &clientWidth, &clientHeight, true);
    ShowWindow(windowHandle, nCmdShow);
    return windowHandle;
}

void win32_move_window(HWND windowHandle, int x, int y, int w, int h) {
    // Moving the console doesn't redraw it, so parts of the window that were originally hidden won't be rendered.
    MoveWindow(windowHandle, x, y, w, h, 0);
    // So after moving the window, redraw it.
    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-redrawwindow
    // "If both the hrgnUpdate and lprcUpdate parameters are NULL, the entire client area is added to the update region."
    RedrawWindow(windowHandle, NULL, NULL, RDW_INVALIDATE);
}

void win32_allocate() {
    // Heap allocation and free on win32...
    // unsigned char* data = NULL;
    // data = win32_ HeapAlloc(win32_ GetProcessHeap(), 0, sizeof(unsigned char)*texture_data_size);
    // win32_ memcpy(data, &texture_data[0], sizeof(unsigned char)*texture_data_size);
    // win32_ HeapFree(win32_ GetProcessHeap(), 0, (LPVOID) data);
}

// unsigned long long cpuFrequencySeconds;
// unsigned long long cpuCounter;
// GetCpuCounterAndFrequencySeconds(&cpuCounter, &cpuFrequencySeconds);
void win32_get_cpu_counter_and_frequency(unsigned long long* cpuCounter, unsigned long long* cpuFrequencySeconds) {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    // Internal Counter at this point
    *cpuCounter = counter.QuadPart;
    // The internal counter alone is not enough to know how much time has passed.
    // However, we can query the system for the performance of the cpu, which tells us how many cycles happen per second
    // and with that calculate the time.
    LARGE_INTEGER performanceFrequency;
    QueryPerformanceFrequency(&performanceFrequency);
    // Cycles per second
    *cpuFrequencySeconds = performanceFrequency.QuadPart;

    // TODO: There is some other ways of getting performance information such as __rdtsc()...
    // I should try it since (I think) might be more precise, since it is an intrinsic function from the compiler?
    // uint64 cyclecount = __rdtsc();
}

// Given the previous cpu counter to compare with, and the cpu frequency (Use GetCpuCounterAndFrequencySeconds)
// Calculate timeDifferenceMs and fps. Returns the current value of cpuCounter.
unsigned long long win32_calculate_ms_and_fps(unsigned long long cpuPreviousCounter, unsigned long long cpuFrequencySeconds, double* timeDifferenceMs, unsigned long long* fps) {
    // Internal Counter at this point
    LARGE_INTEGER cpuCounter;
    QueryPerformanceCounter(&cpuCounter);
    // Difference since last update to this new update
    unsigned long long counterDifference = cpuCounter.QuadPart - cpuPreviousCounter;
    // Since we know the frequency we can calculate some times
    *timeDifferenceMs = 1000.0 * (double)counterDifference / (double)cpuFrequencySeconds;
    *fps = cpuFrequencySeconds / counterDifference;
    return cpuCounter.QuadPart;
}

bool win32_get_console_cursor_positon(short *cursorX, short *cursorY) {
    CONSOLE_SCREEN_BUFFER_INFO cbsi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cbsi)) {
        *cursorX = cbsi.dwCursorPosition.X;
        *cursorY = cbsi.dwCursorPosition.Y;
        return true;
    }
    else {
        return false;
    }
}
// The handle must have the GENERIC_READ access right
bool win32_set_console_cursor_positon(short posX, short posY) {
    COORD position;
    position.X = posX;
    position.Y = posY;
    return SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), position);
}

HDC win32_get_device_context_handle(HWND windowHandle) {
    HDC hdc = GetDC(windowHandle);
    return hdc;
}

//////////////////////////////


// A very basic, default, WindowProc.
// This is a mess, and basically it just handles some quitting messages such as x and ESC
static LRESULT CALLBACK _window_procedure_example(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg)
    {
        // case WM_SIZE: {
        //     RECT win32_rect = (RECT) {0};
        //     // This just gives us the "drawable" part of the window
        //     win32_ GetClientRect(hwnd, &win32_rect);
        //     int width = win32_rect.right - win32_rect.left;
        //     int height = win32_rect.bottom - win32_rect.top;
        //     win32_printf("WIDTH: %d, height: %d\n", width, height);
            
        //     // UINT width = LOWORD(lParam);
        //     // UINT height = HIWORD(lParam);
        // } break;
        case WM_DESTROY: {
            // TODO: turn win32_running to false
            win32_printf("WM_DESTROY\n");
        } // break;
        win32_printf("and\n");
        case WM_CLOSE: {
            // TODO: turn win32_running to false
            win32_printf("WM_CLOSE\n");
            // Basically makes the application post a WM_QUIT message
            // win32_ DestroyWindow(hwnd); // This only closes the main window
            PostQuitMessage(0); // This sends the quit message which the main loop will read and end the loop
            return 0;
        } break;
        // case WM_PAINT: {
        //     win32_print("WM_PAINT\n");
        // } break;
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                // TODO: There is too many points where I want to quite the application... Which one does what?
                PostQuitMessage(0);
            }
            return 0;
        } break;
    }
    // Events that I'm consciously not capturing:
    // . WM_SETCURSOR Sent to a window if the mouse causes the cursor to move within a window and mouse input is not captured
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
    UNREFERENCED_PARAMETER(cmdline);
    UNREFERENCED_PARAMETER(hInstPrev);
    bool isExternalConsole = win32_get_console();
    win32_printf("\n\n");
    const char windowClassName[] = "windowClass";
    WNDCLASS windowClass = win32_make_window_class(windowClassName, _window_procedure_example, hInst);
    UNREFERENCED_PARAMETER(windowClass);
    HWND windowHandle = win32_make_window(windowClassName, "MyWindow!", hInst, cmdshow);
    HDC deviceContextHandle = win32_get_device_context_handle(windowHandle);
    UNREFERENCED_PARAMETER(deviceContextHandle);
    int windowW, windowH, windowX, windowY;
    win32_get_window_size_and_position(windowHandle, &windowW, &windowH, &windowX, &windowY, false);
    int clientW, clientH;
    win32_get_client_size(windowHandle, &clientW, &clientH, false);

    if (isExternalConsole) {
        HWND consoleWindowHandle = GetConsoleWindow();
        int consoleW, consoleH, consoleX, consoleY;
        win32_get_window_size_and_position(consoleWindowHandle, &consoleW, &consoleH, &consoleX, &consoleY, false);
        win32_move_window(consoleWindowHandle, windowX+windowW, windowY, consoleW, consoleH);
    }
    
    bool running = true;
    
    unsigned long long cpuFrequencySeconds;
    unsigned long long cpuCounter;
    win32_get_cpu_counter_and_frequency(&cpuCounter, &cpuFrequencySeconds);
    
    // Main loop
    while (running) {
        MSG msg;
        memset(&msg, 0, sizeof(MSG));
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            switch (msg.message) {
                case WM_QUIT: {
                    running = false;
                } break;
                case WM_SIZE: {
                } break;
            }
        }

        // Write ms and fps at the top of the console
        static double ms;
        static unsigned long long fps;
        static short cursorX;
        static short cursorY;

		cpuCounter = win32_calculate_ms_and_fps(cpuCounter, cpuFrequencySeconds, &ms, &fps);
        if (!win32_get_console_cursor_positon(&cursorX, &cursorY)) {
            win32_printf("Error getting the console cursor position!");
            running = false;
        }
        if (!win32_set_console_cursor_positon(0, 0)) {
            win32_printf("Error setting the console cursor position!");
            running = false;
        }
        win32_printf("ms:  %f         \nfps: %d         ", ms, fps);
        if (!win32_set_console_cursor_positon(cursorX, cursorY)) {
            win32_printf("Error setting the console cursor position!");
            running = false;
        }
    }
}