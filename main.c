// #include <stdio.h>
// #include <stdbool.h>
// #include <assert.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl\GL.h>

// Embedded data
#include "resources.h"
// A define for me to know what things are win specific and what things are not
#define win32_ 
#define dontcare_ 
typedef int bool;
#define true (1)
#define false (0)
typedef long long int64;
typedef unsigned long long uint64;

int abs(int value) {
    return (value < 0) ? -value : value;
}

void win32_clearConsole() {
    static int initialize = 0;
    static win32_ HANDLE win32_console_stdout = 0;
    // To also clear the scroll back, emit L"\x1b[3J" as well.
    // 2J only clears the visible window and 3J only clears the scroll back.
    static win32_ PCWSTR win32_clearConsoleSequence = L"\x1b[2J";
    static win32_ DWORD win32_originalMode = 0;
    if (initialize == 0) {
        win32_console_stdout = win32_ GetStdHandle(STD_OUTPUT_HANDLE);
        // First we have to activate the virtual terminal processing for this to work
        DWORD mode = 0;
        win32_ GetConsoleMode(win32_console_stdout, &mode);
        // We might want to keep the original mode to restore it if necessary. For example when using with other command lines utilities...
        win32_originalMode = mode;
        // restore original mode
        // win32_ SetConsoleMode(hStdOut, originalMode);
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        win32_ SetConsoleMode(win32_console_stdout, mode);
        initialize++;
    }
    win32_ WriteConsoleW(win32_console_stdout, win32_clearConsoleSequence, sizeof(win32_clearConsoleSequence)/sizeof((win32_clearConsoleSequence)[0]), NULL, NULL);
}

void win32_print(const char* string) {
    static int initialize = 0;
    static win32_ HANDLE win32_console_stdout = 0;
    if (initialize == 0) {
        win32_console_stdout = win32_ GetStdHandle(STD_OUTPUT_HANDLE);
        initialize++;
    }
    win32_ WriteConsole(win32_console_stdout, (const void*) string, win32_ lstrlen((LPCSTR) string), NULL, NULL);
    // TODO: make win32_ OutputDebugString() output to my STD_OUTPUT_HANDLE
}

void win32_printf(const char* format, ...) {
    static char buffer[1024];
    va_list args;
    va_start(args, format);
    win32_ wvsprintf((LPSTR) &buffer, format, args);
    va_end(args);
    win32_print(&buffer[0]);
}

void opengl_getErrors(void) {
    GLenum opengl_error = 0;
    while ((opengl_error = glGetError()) != GL_NO_ERROR) {
        win32_print("Error gl: ");
        switch (opengl_error) {
            case GL_NO_ERROR:          { win32_print ("No error\n"); } break;
            case GL_INVALID_ENUM:      { win32_print ("Invalid enum\n"); } break;
            case GL_INVALID_VALUE:     { win32_print ("Invalid value\n"); } break;
            case GL_INVALID_OPERATION: { win32_print ("Invalid operation\n"); } break;
            case GL_STACK_OVERFLOW:    { win32_print ("Stack overflow\n"); } break;
            case GL_STACK_UNDERFLOW:   { win32_print ("Stack underflow\n"); } break;
            case GL_OUT_OF_MEMORY:     { win32_print ("Out of memory\n"); } break;
            default:                   { win32_print ("Unknown error\n"); } break;
        }
    }
}
#define opengl_getErrorsAt(call) win32_printf("%s\n",call); opengl_getErrors();

void win32_getWindowSizeAndPosition(HWND win32_windowHandle, int* width, int* height, int* x, int* y, bool printDebug) {
    RECT win32_rect = (RECT) {0};
    win32_ GetWindowRect(win32_windowHandle, &win32_rect);
    *x = win32_rect.left;
    *y = win32_rect.top;
    *width = win32_rect.right - win32_rect.left;
    *height = win32_rect.bottom - win32_rect.top;
    if (printDebug) {
        win32_printf("win32_getWindowSizeAndPosition: at %d,%d with size {%d} {%d}\n", *x, *y, *width, *height);
    }
}

void win32_getClientSize(HWND win32_windowHandle, int* width, int* height, bool printDebug) {
    RECT win32_rect = (RECT) {0};
    // This just gives us the "drawable" part of the window
    win32_ GetClientRect(win32_windowHandle, &win32_rect);
    *width = win32_rect.right - win32_rect.left;
    *height = win32_rect.bottom - win32_rect.top;
    if (printDebug) {
        win32_printf("win32_getClientSize: {%d} {%d}\n", *width, *height);
    }
}

// https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions
void* opengl_getFunctionAddress(const char* functionName) {
    void* function = (void*) win32_ wglGetProcAddress(functionName);
    // "While the MSDN documentation says that wglGetProcAddress returns NULL on failure,
    // some implementations will return other values. 1, 2, and 3 are used, as well as -1."
    if (function == 0 || function == (void*) 0x1 || function == (void*) 0x2 || function == (void*) 0x3 || function == (void*)-1) {
        // Some functions can only be found with GetProceAddress aparently...
        HMODULE module = win32_ LoadLibraryA("opengl32.dll");
        function = (void*) win32_ GetProcAddress(module, functionName);
    }
    return function;
}

void opengl_initialize(HDC win32_DeviceContextHandle) {    
    // First we need to get a pixel format that we want OpenGL to use
    // But we don't know what we can use, so we first describe our ideal pixel format
    win32_ PIXELFORMATDESCRIPTOR win32_desiredPixelFormat = (PIXELFORMATDESCRIPTOR) {0};
    win32_desiredPixelFormat.nSize = sizeof(win32_desiredPixelFormat);
    win32_desiredPixelFormat.nVersion = 1;
    win32_desiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
    win32_desiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
    // Color + Alpha = 32 bits color (RGBA, 1 byte per channel)
    // FOR SOME REASON it's supposed to be like this but aparently the chosen suggesting colorbits is 32.....
    win32_desiredPixelFormat.cColorBits = 32;
    win32_desiredPixelFormat.cAlphaBits = 8; 
    win32_desiredPixelFormat.iLayerType = PFD_MAIN_PLANE;
    
    // Then windows will suggest what is the best approximation to that pixel format by giving us an index
    int win32_suggestedPixelFormatIndex = win32_ ChoosePixelFormat(win32_DeviceContextHandle, &win32_desiredPixelFormat);
    win32_ PIXELFORMATDESCRIPTOR win32_chosenPixelFormat;
    // But we need to know the complete specification of that suggested pixel format, so also ask windows
    // for that specification fo that one pixel format
    win32_ DescribePixelFormat(win32_DeviceContextHandle, win32_suggestedPixelFormatIndex, sizeof(win32_chosenPixelFormat), &win32_chosenPixelFormat);
    // Finally, set that pixel format that was suggested as the one being used
    win32_ SetPixelFormat(win32_DeviceContextHandle, win32_suggestedPixelFormatIndex, &win32_chosenPixelFormat);

    HGLRC win32_GLRenderingContextHandle = wglCreateContext(win32_DeviceContextHandle);
    if (win32_ wglMakeCurrent(win32_DeviceContextHandle, win32_GLRenderingContextHandle)) {
        // Good
    } else {
        win32_ HANDLE win32_console_stdout = win32_ GetStdHandle(STD_OUTPUT_HANDLE);
        win32_print("Error on wglMakeCurrent");
        win32_ OutputDebugString("Error on wglMakeCurrent");
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
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
        // case WM_DESTROY: {
        //     win32_print("WM_DESTROY\n");
        // } // break;
        // win32_print("and\n");
        // case WM_CLOSE: {
        //     win32_print("WM_CLOSE\n");
        //     // Basically makes the application post a WM_QUIT message
        //     win32_ DestroyWindow(hwnd);
        //     // win32_ PostQuitMessage(0);
        // } break;
        // case WM_PAINT: {
        //     win32_print("WM_PAINT\n");
        // } break;
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                // TODO: There is too many points where I want to quite the application... Which one does what?
                win32_ PostQuitMessage(0);
            }
            return 0;
        } break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

win32_ int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* win32_cmdLine, int nCmdShow) {
    // Set up console
    {
        win32_ AllocConsole();
    }

    const char win32_windowClassName[]  = "Window Class Text";
    WNDCLASSA win32_windowClass = (WNDCLASSA) {0};
    win32_windowClass.lpfnWndProc = WindowProc;
    win32_windowClass.hInstance = hInstance;
    win32_windowClass.lpszClassName = win32_windowClassName;
    win32_ RegisterClass(&win32_windowClass);

    // Create the window
    int win32_windowPositonX = 100;
    int win32_windowPositonY = 100;
    int win32_windowWidth = 0;
    int win32_windowHeight = 0;
    int win32_clientWidth = 0;
    int win32_clientHeight = 0;
    // The size for the window will be the whole window and not the drawing area, so we will have to adjust it later on and it doesn't matter much here
    HWND win32_windowHandle = win32_ CreateWindowEx(0, win32_windowClassName, "Window Text", WS_POPUP | WS_OVERLAPPED | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU  | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        win32_windowPositonX, win32_windowPositonY, dontcare_ 10, dontcare_ 10,
        NULL, NULL, hInstance, NULL
    );
    win32_getWindowSizeAndPosition(win32_windowHandle,&win32_windowWidth,&win32_windowHeight,&win32_windowPositonX,&win32_windowPositonY,true);

    // Before starting with GL stuff, let's figure out the real size of the client area (drawable part of window) and adjust it
    int win32_desiredClientWidth = 500;
    int win32_desiredClientHeight = 600;
    {
        // Get client size
        win32_getClientSize(win32_windowHandle, &win32_clientWidth, &win32_clientHeight, true);
        // Calculate difference between initial size of window and current size of drawable area, that should be the difference to make the window big enough to have our desired drawable area
        int difference_w = abs(win32_clientWidth - win32_desiredClientWidth);
        int difference_h = abs(win32_clientHeight - win32_desiredClientHeight);
        // Set the initially desired position and size now
        win32_ MoveWindow(win32_windowHandle, win32_windowPositonX, win32_windowPositonY, win32_clientWidth + difference_w, win32_clientHeight + difference_h, 0);
        // It should have the right size about now
        win32_getClientSize(win32_windowHandle, &win32_clientWidth, &win32_clientHeight, true);
        win32_print("Window adjusted\n");
    }
    win32_getWindowSizeAndPosition(win32_windowHandle,&win32_windowWidth,&win32_windowHeight,&win32_windowPositonX,&win32_windowPositonY,true);

    HDC win32_DeviceContextHandle = win32_ GetDC(win32_windowHandle);
    win32_ ShowWindow(win32_windowHandle, nCmdShow);
    opengl_initialize(win32_DeviceContextHandle);
    
    // Some gl code
    const int quads = 1;
    GLuint opengl_texture;
    GLuint opengl_vertexBuffer;
    GLuint opengl_indexBuffer;
    // What OpenGL works with...
    // * Screen:
    //     (-1,1)_________(1,1)
    //     |                 |
    //     |                 |
    //     (-1,-1)_________(1,-1)
    // * Textures:
    //     (0,1)_________(1,1)
    //     |                 |
    //     |                 |
    //     (0,0)_________(1,0)
    // What I want to work with...
    // * Indexes:
    //     3---2
    //     | / |
    //     0___1
    // * Screen:
    //     (0,0)_________(1,0)
    //     |                 |
    //     |                 |
    //     (0,1)_________(1,1)
    // * Textures:
    //     (0,0)_________(1,0)
    //     |                 |
    //     |                 |
    //     (0,1)_________(1,1)
    const int indices[6] = { 0, 1, 2, 0, 2, 3 };
    float vertices [4*(3+4+2)] = {
        // Positionx3, Colorx4 (RGBA), Texturex2 (UV)
        /*Positions...*/ 0*win32_clientWidth + win32_clientWidth*0.1f, 1*win32_clientHeight - win32_clientHeight*0.1f, 0.0f, /*Colors...*/ 1, 1, 1, 1, /*Textures...*/ 0, 1,
        /*Positions...*/ 1*win32_clientWidth - win32_clientWidth*0.1f, 1*win32_clientHeight - win32_clientHeight*0.1f, 0.0f, /*Colors...*/ 1, 1, 1, 1, /*Textures...*/ 1, 1,
        /*Positions...*/ 1*win32_clientWidth - win32_clientWidth*0.1f, 0*win32_clientHeight + win32_clientHeight*0.1f, 0.0f, /*Colors...*/ 1, 1, 1, 1, /*Textures...*/ 1, 0,
        /*Positions...*/ 0*win32_clientWidth + win32_clientWidth*0.1f, 0*win32_clientHeight + win32_clientHeight*0.1f, 0.0f, /*Colors...*/ 1, 1, 1, 1, /*Textures...*/ 0, 0,
    };
    {
        glEnable(GL_TEXTURE_2D);
        glGenTextures(1, &opengl_texture);
        glBindTexture(GL_TEXTURE_2D, opengl_texture);
        // Heap allocation and free on win32...
        // unsigned char* data = NULL;
        // data = win32_ HeapAlloc(win32_ GetProcessHeap(), 0, sizeof(unsigned char)*texture_data_size);
        // win32_ memcpy(data, &texture_data[0], sizeof(unsigned char)*texture_data_size);
        // win32_ HeapFree(win32_ GetProcessHeap(), 0, (LPVOID) data);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, (GLsizei)texture_width, (GLsizei)texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &texture_data[0]);
        opengl_getErrorsAt("[glerrors] texture load...");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        glViewport(0, 0, win32_clientWidth, win32_clientHeight);
        glClearColor(1.0f, 0.5f, 0.0f, 0.5f);

    }

    // win32_ Start timers/counters
    uint64 win32_frequency_seconds;
    uint64 win32_counter_start;
    uint64 win32_counter_lastFrame;
    uint64 win32_counter_lastUpdate;
    {
        LARGE_INTEGER win32_counter;
        win32_ QueryPerformanceCounter(&win32_counter);
        // Internal Counter at this point
        win32_counter_start = win32_counter.QuadPart;
        // The internal counter alone is not enough to know how much time has passed.
        // However, we can query the system for the performance of the cpu, which tells us how many cycles happen per second
        // and with that calculate the time.
        LARGE_INTEGER win32_performanceFrequency;
        win32_ QueryPerformanceFrequency(&win32_performanceFrequency);
        // Cycles per second
        win32_frequency_seconds = win32_performanceFrequency.QuadPart;

        // TODO: There is some other ways of getting performance information such as __rdtsc()...
        // I should try it since (I think) might be more precise, since it is an intrinsic function from the compiler?
        // uint64 cyclecount = __rdtsc();
    }
    win32_counter_lastFrame = win32_counter_start;
    win32_counter_lastUpdate = win32_counter_start;

    int win32_running = 1;
    while (win32_running) {
        // GetMessage blocks until a message is found.
        // Instead, PeekMessage can be used.
        win32_ MSG win32_msg = (MSG) {0};
        while (win32_ PeekMessage(&win32_msg, NULL, 0, 0, PM_REMOVE)) {
            win32_ TranslateMessage(&win32_msg); 
            win32_ DispatchMessage(&win32_msg);
            switch (win32_msg.message) {
                case WM_QUIT: {
                    win32_print("WM_QUIT\n");
                    win32_running = 0;
                } break;
                case WM_SIZE: {
                    
                } break;
            }
        }

        // logic or something
        {
            uint64 ms_sinceLastUpdate = 0;
            // Get time since last update
            {
                // Internal Counter at this point
                LARGE_INTEGER win32_counter;
                win32_ QueryPerformanceCounter(&win32_counter);
                // Difference since last update to this new update
                uint64 win32_counterDifference_lastUpdate = win32_counter.QuadPart - win32_counter_lastUpdate;
                // Since we know the frequency we can calculate some times
                ms_sinceLastUpdate = 1000 * win32_counterDifference_lastUpdate / win32_frequency_seconds;
                win32_counter_lastUpdate = win32_counter.QuadPart;
            }
            // win32_clearConsole();
            // win32_print("__ frame __\n");

            RECT win32_clientRectangle = (RECT) {0};
            win32_ GetClientRect(win32_windowHandle, &win32_clientRectangle);
            int win32_clientRectangleWidth = win32_clientRectangle.right;
            int win32_clientRectangleHeight = win32_clientRectangle.bottom;
            // win32_printf("win32_clientRectangleWidth: %d\n", win32_clientRectangleWidth);
            // win32_printf("win32_clientRectangleHeight: %d\n", win32_clientRectangleHeight);
        }
        // Rendering stuff
        {
            glViewport(0, 0, win32_clientWidth, win32_clientHeight);
            glClearColor(1.0f, 0.5f, 0.0f, 0.5f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            // AAAAAAAAAAAAHHHHH GL is column major TT
            // RowMajor:
            // v0,  v1,  v2,  v3,
            // v4,  v5,  v6,  v7,
            // v8,  v9,  v10, v11,
            // v12, v13, v14, v15
            // ColumnMajor:
            // v0,  v4,  v8,  v12,
            // v1,  v5,  v9,  v13,
            // v2,  v6,  v10, v14,
            // v3,  v7,  v11, v15
            #define ToColumnMajor(v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15) v0,v4,v8,v12,v1,v5,v9,v13,v2,v6,v10,v14,v3,v7,v11,v15

            // Invert textures vertically so that I can work with the top left corner as the origin
            GLfloat matrix_invertVertically[] = { ToColumnMajor(
                1, 0, 0, 0,
                0,-1, 0, 1,
                0, 0, 1, 0,
                0, 0, 0, 1
            )};
            glMatrixMode(GL_TEXTURE);
            glLoadIdentity();
            // glLoadMatrixf(matrix_invertVertically);

            // Nothing to do here
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            // http://www.songho.ca/opengl/gl_projectionmatrix.html
            // In order to have the origin (0, 0) on the top left corner
            // and have Right = 1.0f and Bottom = 1.0f :
            // Taking into account that the default fixed pipeline for OpenGL uses a
            // projection of type Left = -1, Right = 1, Top = 1 and Bottom = -1
            // First: Translate every point -1 units on X and Y. That way the projection will change like this:
            // ==> Left = 0, Right = 2, Top = 2 and Bottom = 0
            // Next multiply every (X, Y) by 2, that way we achieve something similar to having a projection like this:
            // ==> Left = 0, Right = 1, Top = 1 and Bottom = 0
            // Finally inverse the Y coordinate to have something like this:
            // ==> Left = 0, Right = 1, Top = 0 and Bottom = 1
            GLfloat w = 2.0f/(float)win32_clientWidth;
            GLfloat h = 2.0f/(float)win32_clientHeight;
            GLfloat matrix_projection[] = { ToColumnMajor(
                w, 0, 0, -1,
                0, -h, 0, 1,
                0, 0, 1, 0,
                0, 0, 0, 1
            )};
            glMatrixMode(GL_PROJECTION);
            glLoadMatrixf(matrix_projection);

            glBindTexture(GL_TEXTURE_2D, opengl_texture);
            glBegin(GL_TRIANGLES);
            glColor3f(1,1,1);
            for(int index = 0; index < quads*6; index++) {
                int current_vertex = indices[index];
                glTexCoord2f(vertices[(current_vertex*9)+7],vertices[(current_vertex*9)+8]);
                glVertex3f(vertices[(current_vertex*9)+0],vertices[(current_vertex*9)+1],vertices[(current_vertex*9)+2]);
            }
            glColor3f(1,0,0);
            glVertex3f(0*win32_clientWidth/10.0f,0*win32_clientHeight/10.0f,0);
            glColor3f(0,1,0);
            glVertex3f(2*win32_clientWidth/10.0f,1*win32_clientHeight/10.0f,0);
            glColor3f(0,0,1);
            glVertex3f(1*win32_clientWidth/10.0f,4*win32_clientHeight/10.0f,0);

            glEnd();
            glBindTexture(GL_TEXTURE_2D, 0);

            win32_ SwapBuffers(win32_DeviceContextHandle);
            // opengl_getErrorsAt("[glerrors] After frame...");
        }
        // Times and stuff
        {
            LARGE_INTEGER win32_counter;
            win32_ QueryPerformanceCounter(&win32_counter);
            // Internal Counter at this point
            // The difference in the counter at the start of the program and right now.
            uint64 win32_counterDifference_start = win32_counter.QuadPart - win32_counter_start;
            uint64 win32_counterDifference_lastFrame = win32_counter.QuadPart - win32_counter_lastFrame;

            // Since we know the frequency we can calculate some times
            uint64 ms_sinceLastFrame = 1000 * win32_counterDifference_lastFrame / win32_frequency_seconds;
            uint64 s_sinceProgramStart = win32_counterDifference_start / win32_frequency_seconds;
            int fps = win32_frequency_seconds / win32_counterDifference_lastFrame;

            // win32_printf("ms_sinceLastFrame:   %lu\n", ms_sinceLastFrame);
            // win32_printf("s_sinceProgramStart: %lu\n", s_sinceProgramStart);
            // win32_printf("FPS:                 %d\n", fps);

            win32_counter_lastFrame = win32_counter.QuadPart;
        }
    }

    // OpenGL cleanup
    {
        glDeleteTextures(1, &opengl_texture);
    }
    // Technically windows will get rid of everything anyway so maybe closing stuff is not really necessary and might just slow down the process of finising our application?
    // https://hero.handmade.network/episode/code/day003/
    // win32_ Sleep(1500);
    win32_ FreeConsole();
    return 0;
}