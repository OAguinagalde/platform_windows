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
typedef long long int64;
typedef unsigned long long uint64;

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
#define opengl_getErrorsAt(call) win32_print(call);win32_print("\n");opengl_getErrors();

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
        case WM_SIZE: {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
        } break;
        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                DestroyWindow(hwnd);
            }
            return 0;
        } break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

win32_ int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* win32_cmdLine, int nCmdShow) {
    win32_ AllocConsole();
    win32_print("Command line:\n");
    win32_print(win32_cmdLine);
    win32_print("\n");
    const char win32_windowClassName[]  = "Window Class Text";
    win32_ WNDCLASSA win32_windowClass = (win32_ WNDCLASSA) {0};
    win32_windowClass.lpfnWndProc = WindowProc;
    win32_windowClass.hInstance = hInstance;
    win32_windowClass.lpszClassName = win32_windowClassName;
    win32_ RegisterClass(&win32_windowClass);

    // Create the window
    const int win32_windowWidth = 500;
    const int win32_windowHeight = 2*300;
    // (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
    win32_ HWND win32_windowHandle = win32_ CreateWindowEx(0, win32_windowClassName, "Window Text", WS_POPUP,
        100, 100, win32_windowWidth, win32_windowHeight,
        NULL, NULL, hInstance, NULL
    );

    win32_ HDC win32_DeviceContextHandle = win32_ GetDC(win32_windowHandle);
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
        /*Positions...*/ 0*win32_windowWidth + win32_windowWidth*0.1f, 1*win32_windowHeight - win32_windowHeight*0.1f, 0.0f, /*Colors...*/ 1, 1, 1, 1, /*Textures...*/ 0, 1,
        /*Positions...*/ 1*win32_windowWidth - win32_windowWidth*0.1f, 1*win32_windowHeight - win32_windowHeight*0.1f, 0.0f, /*Colors...*/ 1, 1, 1, 1, /*Textures...*/ 1, 1,
        /*Positions...*/ 1*win32_windowWidth - win32_windowWidth*0.1f, 0*win32_windowHeight + win32_windowHeight*0.1f, 0.0f, /*Colors...*/ 1, 1, 1, 1, /*Textures...*/ 1, 0,
        /*Positions...*/ 0*win32_windowWidth + win32_windowWidth*0.1f, 0*win32_windowHeight + win32_windowHeight*0.1f, 0.0f, /*Colors...*/ 1, 1, 1, 1, /*Textures...*/ 0, 0,
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

        glViewport(0, 0, win32_windowWidth, win32_windowHeight);
        glClearColor(1.0f, 0.5f, 0.0f, 0.5f);

    }
    
    win32_ ShowWindow(win32_windowHandle, nCmdShow);

    win32_ MSG win32_msg = (MSG) {0};
    win32_ BOOL win32_returnValue;

    while((win32_returnValue = win32_ GetMessage(&win32_msg, NULL, 0, 0)) != 0)
    { 
        if (win32_returnValue == -1)
        {
            win32_ LPTSTR win32_errorMessage = NULL;
            // handle the error and possibly exit
            win32_ DWORD win32_lastError = win32_ GetLastError();
            win32_ FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                win32_lastError,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                win32_errorMessage,
                0, NULL );
            
            if (win32_errorMessage) {
                char buffer[1024];
                win32_ wsprintf((LPSTR) &buffer,"Error on GetMessage:\n%s", win32_errorMessage);
                win32_ OutputDebugString((LPCSTR) &buffer);
                win32_ LocalFree(win32_errorMessage);
            }
            else {
                win32_ OutputDebugString("Error on GetMessage. FormatMessage failed.");
            }
            

        }
        else
        {
            TranslateMessage(&win32_msg); 
            DispatchMessage(&win32_msg);

            // Rendering stuff
            {
                glViewport(0, 0, win32_windowWidth, win32_windowHeight);
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
                GLfloat w = 2.0f/(float)win32_windowWidth;
                GLfloat h = 2.0f/(float)win32_windowHeight;
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
                glVertex3f(0*win32_windowWidth,1*win32_windowHeight,0);
                glColor3f(0,1,0);
                glVertex3f(1*win32_windowWidth,1*win32_windowHeight,0);
                glColor3f(0,0,1);
                glVertex3f(0,0,0);

                // glColor3f(0,1,0);
                // glVertex3f(0,200,0);
                // glVertex3f(100,200,0);
                // glVertex3f(50,0,0);
                glEnd();
                glBindTexture(GL_TEXTURE_2D, 0);

                SwapBuffers(win32_DeviceContextHandle);
                opengl_getErrorsAt("[glerrors] After frame...");
            }
        }
    }

    // OpenGL cleanup
    {
        glDeleteTextures(1, &opengl_texture);
    }

    win32_ FreeConsole();
    return 0;
}