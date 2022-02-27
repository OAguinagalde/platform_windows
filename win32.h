#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <strsafe.h>
#include <stdbool.h>
#include <math.h>
#pragma comment(lib, "User32")

// Clears the console associated with the stdout
void win32_clear_console();

// printf but wrong lol
// Max output is 1024 bytes long!
#define WIN32_PRINTF_MAX_BUFFER_SIZE 1024
void win32_printf(const char* format, ...);

// Given a char buffer, and a format str, puts the resulting str in the buffer
void win32_format_buffer(char* buffer, const char* format, ...);

// Given a windowHandle, queries the width, height and position (x, y) of the window
void win32_get_window_size_and_position(HWND windowHandle, int* width, int* height, int* x, int* y, bool printDebug);

// Given a windowHandle, queries the width and height of the client size (The drawable area)
void win32_get_client_size(HWND windowHandle, int* width, int* height, bool printDebug);

// Try to get a console, for situations where there might not be one
// Return true when an external consolle (new window) has been allocated
bool win32_get_console();

WNDCLASS win32_make_window_class(const char* windowClassName, WNDPROC pfnWindowProc, HINSTANCE hInstance);

HWND win32_make_window(const char* windowClassName, const char* title, HINSTANCE hInstance, int nCmdShow);

void win32_move_window(HWND windowHandle, int x, int y, int w, int h);

void win32_allocate();

// unsigned long long cpuFrequencySeconds;
// unsigned long long cpuCounter;
// GetCpuCounterAndFrequencySeconds(&cpuCounter, &cpuFrequencySeconds);
void win32_get_cpu_counter_and_frequency(unsigned long long* cpuCounter, unsigned long long* cpuFrequencySeconds);

// Given the previous cpu counter to compare with, and the cpu frequency (Use GetCpuCounterAndFrequencySeconds)
// Calculate timeDifferenceMs and fps. Returns the current value of cpuCounter.
unsigned long long win32_calculate_ms_and_fps(unsigned long long cpuPreviousCounter, unsigned long long cpuFrequencySeconds, double* timeDifferenceMs, unsigned long long* fps);

bool win32_get_console_cursor_positon(short *cursorX, short *cursorY);
// The handle must have the GENERIC_READ access right
bool win32_set_console_cursor_positon(short posX, short posY);

HDC win32_get_device_context_handle(HWND windowHandle);
