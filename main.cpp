#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <strsafe.h>
#pragma comment(lib, "User32")
#include <cassert>
#include <cstdlib>
namespace Win32 {
    // Clears the console associated with the stdout
    void ClearConsole() {
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
        WriteConsoleW(consoleStdOut, clearConsoleSequence, sizeof(clearConsoleSequence)/sizeof((clearConsoleSequence)[0]), NULL, NULL);
        // Restore original mode
        SetConsoleMode(consoleStdOut, originalMode);
    }

    // Prints a string to stdout
    void Print(const char* str) {
        WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), (const void*) str, lstrlen((LPCSTR) str), NULL, NULL);
    }

    // printf but wrong lol
    // Max output is 1024 bytes long!
    void FormattedPrint(const char* format, ...) {
        static const size_t buffer_size = 1024;
        static char buffer[buffer_size];
        va_list args;
        va_start(args, format);
        // https://docs.microsoft.com/en-us/windows/win32/menurc/strsafe-ovw
        // https://docs.microsoft.com/en-us/windows/win32/api/strsafe/nf-strsafe-stringcbvprintfa
        StringCbVPrintfA(buffer, buffer_size, format, args);
        va_end(args);
        Print(buffer);
    }

    // Given a char buffer, and a format str, puts the resulting str in the buffer
    void FormatBuffer(char* buffer, const char* format, ...) {
        va_list args;
        va_start(args, format);
        wvsprintf((LPSTR) buffer, format, args);
        va_end(args);
    }

    // Given a windowHandle, queries the width, height and position (x, y) of the window
    void GetWindowSizeAndPosition(HWND windowHandle, int* width, int* height, int* x, int* y, bool printDebug) {
        RECT rect;
        GetWindowRect(windowHandle, &rect);
        *x = rect.left;
        *y = rect.top;
        *width = rect.right - rect.left;
        *height = rect.bottom - rect.top;
        if (printDebug) {
            FormattedPrint("Window at %d,%d with size %dx%d\n", *x, *y, *width, *height);
        }
    }

    // Given a windowHandle, queries the width and height of the client size (The drawable area)
    void GetClientSize(HWND windowHandle, int* width, int* height, bool printDebug) {
        RECT rect;
        // This just gives us the "drawable" part of the window
        GetClientRect(windowHandle, &rect);
        *width = rect.right - rect.left;
        *height = rect.bottom - rect.top;
        if (printDebug) {
            FormattedPrint("Client size %dx%d\n", *width, *height);
        }
    }

    // Try to get a console, for situations where there might not be one
    // Return true when an external consolle (new window) has been allocated
    bool GetConsole() {
        bool consoleFound = false;
        bool consoleIsExternal = false;
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            // Situation example:
            // Opening a Windows Subsystem process from a cmd.exe process. It the process will attach to cmd.exe's console
            consoleFound = true;
            Print("Console hijacked!\n");
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
                    FormattedPrint("Unreachable at %s, %s\n", __FUNCTIONW__, __FILE__);
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
                FormattedPrint("Unreachable at %s, %s\n", __FUNCTIONW__, __FILE__);
            }
        }
        
        return consoleIsExternal;
    }

    // A very basic, default, WindowProc.
    // This is a mess, and basically it just handles some quitting messages such as x and ESC
    LRESULT CALLBACK BasicWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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
                Print("WM_DESTROY\n");
            } // break;
            Print("and\n");
            case WM_CLOSE: {
                // TODO: turn win32_running to false
                Print("WM_CLOSE\n");
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

    WNDCLASSA MakeWindowClass(const char* windowClassName, WNDPROC pfnWindowProc, HINSTANCE hInstance) {
        WNDCLASSA windowClass = {};
        windowClass.lpfnWndProc = pfnWindowProc;
        windowClass.hInstance = hInstance;
        windowClass.lpszClassName = windowClassName;
        windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClass(&windowClass);
        return windowClass;
    }

    HWND MakeWindow(const char* windowClassName, const char* title, HINSTANCE hInstance, int nCmdShow) {
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
        Print("Window created\n");
        GetWindowSizeAndPosition(windowHandle,&windowWidth,&windowHeight,&windowPositionX,&windowPositionY,false);
        
        // let's figure out the real size of the client area (drawable part of window) and adjust it
        int desiredClientWidth = 500;
        int desiredClientHeight = 600;
        // Get client size
        GetClientSize(windowHandle, &clientWidth, &clientHeight, false);
        // Calculate difference between initial size of window and current size of drawable area, that should be the difference to make the window big enough to have our desired drawable area
        int difference_w = abs(clientWidth - desiredClientWidth);
        int difference_h = abs(clientHeight - desiredClientHeight);
        // Set the initially desired position and size now
        MoveWindow(windowHandle, windowPositionX, windowPositionY, windowWidth + difference_w, windowHeight + difference_h, 0);
        // It should have the right size about now
        Print("Window adjusted\n");
        GetWindowSizeAndPosition(windowHandle, &windowWidth, &windowHeight, &windowPositionX, &windowPositionY, true);
        GetClientSize(windowHandle, &clientWidth, &clientHeight, true);
        ShowWindow(windowHandle, nCmdShow);
        return windowHandle;
    }

    void MoveAWindow(HWND windowHandle, int x, int y, int w, int h) {
        // Moving the console doesn't redraw it, so parts of the window that were originally hidden won't be rendered.
        MoveWindow(windowHandle, x, y, w, h, 0);
        // So after moving the window, redraw it.
        // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-redrawwindow
        // "If both the hrgnUpdate and lprcUpdate parameters are NULL, the entire client area is added to the update region."
        RedrawWindow(windowHandle, NULL, NULL, RDW_INVALIDATE);
    }

    void AllocateOnWindowsStuff() {
        // Heap allocation and free on win32...
        // unsigned char* data = NULL;
        // data = win32_ HeapAlloc(win32_ GetProcessHeap(), 0, sizeof(unsigned char)*texture_data_size);
        // win32_ memcpy(data, &texture_data[0], sizeof(unsigned char)*texture_data_size);
        // win32_ HeapFree(win32_ GetProcessHeap(), 0, (LPVOID) data);
    }

    // uint64 cpuFrequencySeconds;
    // uint64 cpuCounter;
    // GetCpuTimeAndFrequency(&cpuCounter, &cpuFrequencySeconds);
    void GetCpuCounterAndFrequencySeconds(unsigned long long* cpuCounter, unsigned long long* cpuFrequencySeconds) {
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
    unsigned long long GetTimeDifferenceMsAndFPS(unsigned long long cpuPreviousCounter, unsigned long long cpuFrequencySeconds, unsigned long long* timeDifferenceMs, unsigned long long* fps) {
        // Internal Counter at this point
        LARGE_INTEGER cpuCounter;
        QueryPerformanceCounter(&cpuCounter);
        // Difference since last update to this new update
        unsigned long long counterDifference = cpuCounter.QuadPart - cpuPreviousCounter;
        // Since we know the frequency we can calculate some times
        *timeDifferenceMs = 1000 * counterDifference / cpuFrequencySeconds;
        *fps = cpuFrequencySeconds / counterDifference;
        return cpuCounter.QuadPart;
    }

    // This is not a function, it's just a reference for me for when I want to do a message loop and I dont remember...
    void LoopWindowsMessages() {
        // GetMessage blocks until a message is found.
        // Instead, PeekMessage can be used.
        MSG msg = {0};
        // Look if there is a message and if so remove it
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg); 
            DispatchMessage(&msg);

            switch (msg.message) {
                case WM_QUIT: {
                } break;
                case WM_SIZE: {
                } break;
            }
        }
    }

    HDC GetDeviceContextHandle(HWND windowHandle) {
        HDC hdc = GetDC(windowHandle);
        return hdc;
    }

    void SwapPixelBuffers(HDC deviceContextHandle) {
        // The SwapBuffers function exchanges the front and back buffers if the
        // current pixel format for the window referenced by the specified device context includes a back buffer.
        SwapBuffers(deviceContextHandle);
    }

    void Test1() {
        GetConsole();
        // ClearConsole();
        Print("This is a test.\n");
        FormattedPrint("This is 42: %d\n", 42);
        char formattedText[1024];
        FormatBuffer(formattedText, "This text has been formatted! %d", 42);
        FormattedPrint("Formatting some text: \"%s\"\n", formattedText);
        int w,h,x,y;
        GetWindowSizeAndPosition(GetConsoleWindow(), &w,&h,&x,&y, true);
    }
}

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl\GL.h>
// https://www.khronos.org/registry/OpenGL/api/GL/wglext.h
#include "wglext.h"
// https://www.khronos.org/registry/OpenGL/api/GL/glext.h
// . depends on KHR/khrplatform.h
// . https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h
#include "glext.h"
#pragma comment(lib, "gdi32")
#pragma comment(lib, "Opengl32")
namespace Win32 {
    namespace GL {

        #define DeclareExtension(type, name) type name = NULL;
        DeclareExtension(PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT);
        DeclareExtension(PFNGLGENBUFFERSPROC, glGenBuffers);
        DeclareExtension(PFNGLBINDBUFFERPROC, glBindBuffer);
        DeclareExtension(PFNGLBUFFERDATAPROC, glBufferData);
        DeclareExtension(PFNGLCREATESHADERPROC, glCreateShader);
        DeclareExtension(PFNGLSHADERSOURCEPROC, glShaderSource);
        DeclareExtension(PFNGLCOMPILESHADERPROC, glCompileShader);
        DeclareExtension(PFNGLGETSHADERIVPROC, glGetShaderiv);
        DeclareExtension(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
        DeclareExtension(PFNGLBLENDFUNCSEPARATEPROC, glBlendFuncSeparate);
        DeclareExtension(PFNGLCREATEPROGRAMPROC, glCreateProgram);
        DeclareExtension(PFNGLATTACHSHADERPROC, glAttachShader);
        DeclareExtension(PFNGLLINKPROGRAMPROC, glLinkProgram);
        DeclareExtension(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
        DeclareExtension(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
        DeclareExtension(PFNGLDELETESHADERPROC, glDeleteShader);
        DeclareExtension(PFNGLUSEPROGRAMPROC, glUseProgram);
        DeclareExtension(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
        DeclareExtension(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
        DeclareExtension(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
        DeclareExtension(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
        DeclareExtension(PFNGLACTIVETEXTUREPROC, glActiveTexture);
        DeclareExtension(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);
        DeclareExtension(PFNGLUNIFORM2FVPROC, glUniform2fv);
        DeclareExtension(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
        #undef DeclareExtension

        // Loops through and print all the errors related to OpenGL
        bool GetErrors() {
            bool noErrors = true;
            GLenum opengl_error = 0;
            while ((opengl_error = glGetError()) != GL_NO_ERROR) {
                noErrors = false;
                Print("Error gl: ");
                switch (opengl_error) {
                    case GL_NO_ERROR:          { Print("GL_NO_ERROR\n"); } break;
                    case GL_INVALID_ENUM:      { Print("GL_INVALID_ENUM\n"); } break;
                    case GL_INVALID_VALUE:     { Print("GL_INVALID_VALUE\n"); } break;
                    case GL_INVALID_OPERATION: { Print("GL_INVALID_OPERATION\n"); } break;
                    case GL_STACK_OVERFLOW:    { Print("GL_STACK_OVERFLOW\n"); } break;
                    case GL_STACK_UNDERFLOW:   { Print("GL_STACK_UNDERFLOW\n"); } break;
                    case GL_OUT_OF_MEMORY:     { Print("GL_OUT_OF_MEMORY\n"); } break;
                    default:                   { Print("Unknown Error\n"); } break;
                }
            }
            if (noErrors) {
                Print("No errors\n");
            }
            return !noErrors;
        }

        // Loops through and print (Labeled) all the errors related to OpenGL
        bool GetErrors(const char* label) {
            FormattedPrint("%s: ", label);
            return GetErrors();
        }

        // Returns the pointer to the function given or returns NULL on failure
        void* GetFunctionAddress(const char* functionName) {
            void* function = (void*) wglGetProcAddress(functionName);
            // https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions
            // "While the MSDN documentation says that wglGetProcAddress returns NULL on failure,
            // some implementations will return other values. 1, 2, and 3 are used, as well as -1."
            if (function == 0 || function == (void*) 0x1 || function == (void*) 0x2 || function == (void*) 0x3 || function == (void*)-1) {
                // Some functions can only be found with GetProceAddress aparently...
                HMODULE module = LoadLibraryA("opengl32.dll");
                function = (void*) GetProcAddress(module, functionName);
            }
            // both GetProcAddress and wglGetProcAddress return NULL on failure, so just check for return value to see if function was found
            return function;
        }

        // Given a devide context handle, initializes a ready to use OpenGl context with a (hopefully) RGBA32 pixel format
        void InitializeWGlContext(HDC DeviceContextHandle) {    
            // First we need to get a pixel format that we want OpenGL to use
            // But we don't know what we can use, so we first describe our ideal pixel format
            PIXELFORMATDESCRIPTOR desiredPixelFormat = {};
            desiredPixelFormat.nSize = sizeof(desiredPixelFormat);
            desiredPixelFormat.nVersion = 1;
            desiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
            desiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
            // Color + Alpha = 32 bits color (RGBA, 1 byte per channel)
            // FOR SOME REASON it's supposed to be like this but aparently the chosen suggesting colorbits is 32.....
            desiredPixelFormat.cColorBits = 32;
            desiredPixelFormat.cAlphaBits = 8; 
            desiredPixelFormat.iLayerType = PFD_MAIN_PLANE;
            
            // Then windows will suggest what is the best approximation to that pixel format by giving us an index
            int suggestedPixelFormatIndex = ChoosePixelFormat(DeviceContextHandle, &desiredPixelFormat);
            PIXELFORMATDESCRIPTOR chosenPixelFormat;
            // But we need to know the complete specification of that suggested pixel format, so also ask windows
            // for that specification for that one pixel format
            DescribePixelFormat(DeviceContextHandle, suggestedPixelFormatIndex, sizeof(chosenPixelFormat), &chosenPixelFormat);
            // Finally, set that pixel format that was suggested as the one being used
            SetPixelFormat(DeviceContextHandle, suggestedPixelFormatIndex, &chosenPixelFormat);

            HGLRC GLRenderingContextHandle = wglCreateContext(DeviceContextHandle);
            if (!wglMakeCurrent(DeviceContextHandle, GLRenderingContextHandle)) {
                DWORD e = GetLastError();
                FormattedPrint("Error at %s: wglMakeCurrent, GetLastError() -> %d\n", __FUNCTIONW__, e);
            }
            else {
                Print("Pixel Format selected and wglContext created\n");
            }
        }

        void GetGLExtensions() {
            #define InitializeExtension(type, name) name = (type) GetFunctionAddress(#name); assert(name && #name);
            InitializeExtension(PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT);
            InitializeExtension(PFNGLGENBUFFERSPROC, glGenBuffers);
            InitializeExtension(PFNGLBINDBUFFERPROC, glBindBuffer);
            InitializeExtension(PFNGLBUFFERDATAPROC, glBufferData);
            InitializeExtension(PFNGLCREATESHADERPROC, glCreateShader);
            InitializeExtension(PFNGLSHADERSOURCEPROC, glShaderSource);
            InitializeExtension(PFNGLCOMPILESHADERPROC, glCompileShader);
            InitializeExtension(PFNGLGETSHADERIVPROC, glGetShaderiv);
            InitializeExtension(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
            InitializeExtension(PFNGLBLENDFUNCSEPARATEPROC, glBlendFuncSeparate);
            InitializeExtension(PFNGLCREATEPROGRAMPROC, glCreateProgram);
            InitializeExtension(PFNGLATTACHSHADERPROC, glAttachShader);
            InitializeExtension(PFNGLLINKPROGRAMPROC, glLinkProgram);
            InitializeExtension(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
            InitializeExtension(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
            InitializeExtension(PFNGLDELETESHADERPROC, glDeleteShader);
            InitializeExtension(PFNGLUSEPROGRAMPROC, glUseProgram);
            InitializeExtension(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
            InitializeExtension(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
            InitializeExtension(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
            InitializeExtension(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
            InitializeExtension(PFNGLACTIVETEXTUREPROC, glActiveTexture);
            InitializeExtension(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);
            InitializeExtension(PFNGLUNIFORM2FVPROC, glUniform2fv);
            InitializeExtension(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
            #undef InitializeExtension
        }

        struct Color {
            float r, g, b, a;
            static Color Red() {
                Color c;
                c.r = 1; c.g = 0; c.b = 0; c.a = 1;
                return c;
            }
            static Color Green() {
                Color c;
                c.r = 0; c.g = 1; c.b = 0; c.a = 1;
                return c;
            }
            static Color Blue() {
                Color c;
                c.r = 0; c.g = 0; c.b = 1; c.a = 1;
                return c;
            }
            static Color Cyan() {
                Color c;
                c.r = 0; c.g = 1; c.b = 1; c.a = 1;
                return c;
            }
            static Color Yellow() {
                Color c;
                c.r = 1; c.g = 1; c.b = 0; c.a = 1;
                return c;
            }
            static Color White() {
                Color c;
                c.r = 1; c.g = 1; c.b = 1; c.a = 1;
                return c;
            }
        };
        struct Vertex {
            static constexpr int componentsNumber = 8;
            union {
                struct {
                    // x, y for position
                    // u, v for texture coordinates
                    // r, g, b, a for color
                    float x, y;
                    float u, v;
                    union {
                        float r, g, b, a;
                        Color color;
                    };
                };
                float data[componentsNumber];
            };

            // Returns an empty Vertex
            static Vertex Zero() {
                Vertex v;
                v.Empty();
                return v;
            }
            // Empties the vertex
            void Empty() {
                for(int i = 0; i < componentsNumber; i++) {
                    data[i] = 0.0f;
                }
            }
            void TopLeft() {
                x = 0; y = 0;
            }
            void TopRight() {
                x = 1; y = 0;
            }
            void BottomLeft() {
                x = 0; y = 1;
            }
            void BottomRight() {
                x = 1; y = 1;
            }
            void TextTopLeft() {
                u = 0; v = 0;
            }
            void TextTopRight() {
                u = 1; v = 0;
            }
            void TextBottomLeft() {
                u = 0; v = 1;
            }
            void TextBottomRight() {
                u = 1; v = 1;
            }
        };
        struct Quad {
            // d___c
            // | / |
            // a___b
            Vertex a, b, c, d;
        };
        struct Renderer {
            // What OpenGL works with...
            // Screen                 Textures
            // (-1, 1)_____( 1, 1)    ( 0, 1)_____( 1, 1)
            // |                 |    |                 |
            // |                 |    |                 |
            // (-1,-1)_____( 1,-1)    ( 0, 0)_____( 1, 0)
            
            // What I want to work with...
            // Screen                 Textures               Indexes
            // ( 0, 0)_____( 1, 0)    ( 0, 0)_____( 1, 0)    3---2
            // |                 |    |                 |    | / |
            // |                 |    |                 |    0___1
            // ( 0, 1)_____( 1, 1)    ( 0, 1)_____( 1, 1)

            static constexpr unsigned long maxQuads = 1000;
            static constexpr unsigned long maxVertices = maxQuads * 4;
            static constexpr unsigned long maxIndices = maxQuads * 6;
            GLfloat textureDimensions[2];
            unsigned long quadsToRender = 0;

            GLuint textureObject = 0;
            GLuint vertexArrayObject = 0;
            GLuint elementBufferObject = 0;
            GLuint vertexBufferObject = 0;
            GLuint vertexShaderObject = 0;
            GLuint fragmentShaderObject = 0;
            GLuint shaderProgramObject = 0;

            
            Vertex vertexBuffer[maxVertices];

            bool textureLoaded = false;

            enum shaderType { FragmentShader, VertexShader };
            void LoadShader(const char* shaderSource, unsigned long sourceSize, shaderType type) {
                if (type == shaderType::FragmentShader) {
                    fragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);
                    glShaderSource(fragmentShaderObject, 1, &shaderSource, NULL);
                    glCompileShader(fragmentShaderObject);
                    int success;
                    glGetShaderiv(fragmentShaderObject, GL_COMPILE_STATUS, &success);
                    if(success == 0) {
                        char info[512];
                        glGetShaderInfoLog(fragmentShaderObject, 512, NULL, info);
                        FormattedPrint("Error compiling fragment shader:\n\t%s", info);
                    }
                }
                else if (type == shaderType::VertexShader) {
                    vertexShaderObject = glCreateShader(GL_VERTEX_SHADER);
                    glShaderSource(vertexShaderObject, 1, &shaderSource, NULL);
                    glCompileShader(vertexShaderObject);
                    int success;
                    glGetShaderiv(vertexShaderObject, GL_COMPILE_STATUS, &success);
                    if(success == 0) {
                        char info[512];
                        glGetShaderInfoLog(vertexShaderObject, 512, NULL, info);
                        FormattedPrint("Error compiling vertex shader:\n\t%s", info);
                    }
                }
                GetErrors(__FUNCTION__);
            }

            void GenerateShaderProgram() {
                shaderProgramObject = glCreateProgram();
                glAttachShader(shaderProgramObject, vertexShaderObject);
                glAttachShader(shaderProgramObject, fragmentShaderObject);
                glLinkProgram(shaderProgramObject);
                int success;
                glGetProgramiv(shaderProgramObject, GL_LINK_STATUS, &success);
                if(success == 0) {
                    char info[512];
                    glGetProgramInfoLog(shaderProgramObject, 512, NULL, info);
                    FormattedPrint("Error linking shader program:\n\t%s", info);
                }
                glDeleteShader(vertexShaderObject);
                glDeleteShader(fragmentShaderObject);
                GetErrors(__FUNCTION__);
            }
            
            void LoadTexture(void* data, GLsizei w, GLsizei h) {
                textureDimensions[0] = w;
                textureDimensions[1] = h;
                glBindTexture(GL_TEXTURE_2D, textureObject);
                glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                glBindTexture(GL_TEXTURE_2D, 0);
                GetErrors(__FUNCTION__);
            }

            void Initialize() {
                // Something about windows and framerates, dont remember, probably vertical sync
                wglSwapIntervalEXT(1);
                
                // Configure textures
                // Load them later with LoadTexture()
                glEnable(GL_TEXTURE_2D);
                glGenTextures(1, &textureObject);
                glBindTexture(GL_TEXTURE_2D, textureObject);
                glActiveTexture(GL_TEXTURE0);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glBindTexture(GL_TEXTURE_2D, 0);
                
                // Configure Blending
                glEnable(GL_BLEND);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

                // Generate the vertex array object and configure it
                glGenVertexArrays(1, &vertexArrayObject);
                glBindVertexArray(vertexArrayObject);

                // Generate an element buffer object for the indices
                glGenBuffers(1, &elementBufferObject);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferObject);
                // It's an static buffer so we can just load it now on initialization and forget about it
                unsigned long indices[maxIndices];
                for (int i = 0; i < maxQuads; i++) {
                    int vertex = i * 4; // 4 vertices per quad
                    int index = i * 6; // 6 indices per quad
                    indices[index + 0] = vertex + 0;
                    indices[index + 1] = vertex + 1;
                    indices[index + 2] = vertex + 2;
                    indices[index + 3] = vertex + 0;
                    indices[index + 4] = vertex + 2;
                    indices[index + 5] = vertex + 3;
                }
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned long) * maxIndices, indices, GL_STATIC_DRAW);
                
                // vertex buffer object
                glGenBuffers(1, &vertexBufferObject);
                glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
                for (int i = 0; i < maxVertices; i++) {
                    vertexBuffer[i].Empty();
                }
                glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * maxVertices, vertexBuffer, GL_DYNAMIC_DRAW);

                // Configure the vertex layer
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(0));
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
                glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));
                glEnableVertexAttribArray(0);
                glEnableVertexAttribArray(1);
                glEnableVertexAttribArray(2);

                // Finish, unbind everything
                glBindVertexArray(0);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }

            void Render(unsigned long clientWidth, unsigned long clientHeight, Color clearColor, HDC deviceContextHandle) {
                glViewport(0, 0, clientWidth, clientHeight);
                glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
                glClear(GL_COLOR_BUFFER_BIT);

                #define ToColumnMajor(v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15) v0,v4,v8,v12,v1,v5,v9,v13,v2,v6,v10,v14,v3,v7,v11,v15
                GLfloat w = 2.0f/(float)clientWidth;
                GLfloat h = 2.0f/(float)clientHeight;
                GLfloat projectionMatrix[] = { ToColumnMajor(
                    w,  0,  0, -1,
                    0, -h,  0,  1,
                    0,  0,  1,  0,
                    0,  0,  0,  1
                )};
                #undef ToColumnMajor

                glBindVertexArray(vertexArrayObject);
                glBindTexture(GL_TEXTURE_2D, textureObject);
                glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
                glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 4 * quadsToRender, vertexBuffer, GL_DYNAMIC_DRAW);
                glUseProgram(shaderProgramObject);
                GLint mvpUniformPosition = glGetUniformLocation(shaderProgramObject, "mvp");
                GLint textureDimensionsUniformPosition = glGetUniformLocation(shaderProgramObject, "texture_dimensions");
                glUniformMatrix4fv(mvpUniformPosition, 1, GL_FALSE, projectionMatrix);
                glUniform2fv(textureDimensionsUniformPosition, 1, textureDimensions);
                glDrawElements(GL_TRIANGLES, quadsToRender * 6, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
                glBindTexture(GL_TEXTURE_2D, 0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                Win32::SwapPixelBuffers(deviceContextHandle);
                quadsToRender = 0;
            }
        };
    }
}

// Warning: If I define WIN32_LEAN_AND_MEAN then I lose the contents of mmeapi.h
// . Which contains WAVEFORMATEX and other things I need for DirectSound.
// . typedef struct {
// .     WORD    wFormatTag;        /* format type */
// .     WORD    nChannels;         /* number of channels (i.e. mono, stereo...) */
// .     DWORD   nSamplesPerSec;    /* sample rate */
// .     DWORD   nAvgBytesPerSec;   /* for buffer estimation */
// .     WORD    nBlockAlign;       /* block size of data */
// .     WORD    wBitsPerSample;    /* Number of bits per sample of mono data */
// .     WORD    cbSize;            /* The count in bytes of the size of extra information (after cbSize) */
// . } WAVEFORMATEX;
// . #define WAVE_FORMAT_PCM 1
// . #include <Mmreg.h>
#include <windows.h>
#include <mmsystem.h>
#include <DSound.h>
#pragma comment(lib, "gdi32.lib")
namespace Win32 {
    namespace DSOUND {
        // https://docs.microsoft.com/en-us/previous-versions/windows/desktop/mt708921(v=vs.85)
        typedef HRESULT WINAPI directSoundCreate_t(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
        // TODO: For now this is global
        static LPDIRECTSOUNDBUFFER globalSecondaryBuffer;
        
        void Initialize(HWND windowHandle, int SamplesPerSecond, int BufferSize) {
            // Load the library dinamically, allowing to deal with the library not existing if that's the case
            HMODULE library =  LoadLibraryA("dsound.dll");
            if(library) {
                Print("DirectSound: Library loaded.\n");
                // To avoid linking a whole library for a single function that I need (DirectSoundCreate). Instead get the address of that function in the library itself.
                // This also allows to load the sound library dinamically, so that if the library doesn't exist then I can just keep running without using sound.
                directSoundCreate_t *DirectSoundCreate = (directSoundCreate_t *)  GetProcAddress(library, "DirectSoundCreate");

                // The Direct Sound API seems to be made in an object oriented kind of way. They basically hardcode the vtable that would be automatically created in c++
                // and in order to use I just have to explicitly invoke the operations of the "object" through the lpVtbl member and pass the address of the object as the
                // first parameter to every operation. An example is:
                // object->lpVtbl->SetCooperativeLevel(&object, windowHandle, DSSCL_PRIORITY)
                IDirectSound* object;
                if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &object, 0))) {
                    Print("DirectSound: Interface created.\n");
                    // For some reason the WAVEFORMAT is not defined in DirectSound header... So I just copied instead to my source... Not sure I should be doing this, but
                    // as long as it works......
                    WAVEFORMATEX waveFormat = {};
                    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
                    // Stereo Sound
                    waveFormat.nChannels = 2;
                    waveFormat.nSamplesPerSec = SamplesPerSecond;
                    // 16 bits per sample because... why not?
                    waveFormat.wBitsPerSample = 16;
                    // nBlockAlign:
                    // . https://docs.microsoft.com/en-us/previous-versions/windows/desktop/bb280529(v=vs.85)
                    // . https://docs.microsoft.com/en-us/windows/win32/api/mmeapi/ns-mmeapi-waveformatex
                    // . "Block alignment, in bytes.
                    // . The value of the BlockAlign property must be equal to the product of Channels and BitsPerSample divided by 8 (bits per byte).
                    // . Software must process a multiple of nBlockAlign bytes of data at a time."
                    waveFormat.nBlockAlign = (waveFormat.nChannels*waveFormat.wBitsPerSample) / 8;
                    // nAvgBytesPerSec:
                    // . "Should be equal to the product of nSamplesPerSec and nBlockAlign"
                    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec*waveFormat.nBlockAlign;
                    waveFormat.cbSize = 0;

                    // Cooperation level let's windows know how the application sound should interact with the rest of applications. Kind of
                    // c calling:
                    // if(SUCCEEDED(object->lpVtbl->SetCooperativeLevel(object, windowHandle, DSSCL_PRIORITY))) {
                    // cpp calling:
                    if(SUCCEEDED(object->SetCooperativeLevel(windowHandle, DSSCL_PRIORITY))) {
                        Print("DirectSound: CooperationLevel changed.\n");

                        // As far as I know, the reason we have to create a main buffer (which we won't use) and then later on a "secondary buffer"
                        // which we will actually work with is because nowadays windows (?) doesn't allow to write to the sound hardware directly (the main buffer)
                        // and instead every application uses a secondary buffer, and later on windows mix every secondary buffer and does the necessary work with
                        // that main buffer. Something like that...
                        DSBUFFERDESC bufferDescription = {};
                        bufferDescription.dwSize = sizeof(bufferDescription);
                        bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                        IDirectSoundBuffer* primaryBuffer;
                        if(SUCCEEDED(object->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0))) {
                            Print("DirectSound: Primary buffer created successfully.\n");
                            HRESULT ret = primaryBuffer->SetFormat((const WAVEFORMATEX*) &waveFormat);
                            if(SUCCEEDED(ret)) {
                                Print("DirectSound: Primary buffer format was set.\n");
                            }
                            else {
                                Print("DirectSound: Primary buffer formatting failed.\n");
                            }
                        }
                        else {
                            Print("DirectSound: Primary buffer creation failed.\n");
                        } // create primary sound buffer
                    }
                    else {
                        Print("DirectSound: CooperationLevel change failed.\n");
                    } // coop level

                    DSBUFFERDESC bufferDescription = {};
                    bufferDescription.dwSize = sizeof(bufferDescription);
                    bufferDescription.dwFlags = 0;
                    bufferDescription.dwBufferBytes = BufferSize;
                    bufferDescription.lpwfxFormat = &waveFormat;
                    HRESULT ret = object->CreateSoundBuffer(&bufferDescription, &globalSecondaryBuffer, 0);
                    if(SUCCEEDED(ret)) {
                        Print("DirectSound: Secondary buffer created successfully.\n");
                    }
                    else {
                        Print("DirectSound: Secondary buffer creation failed.\n");
                    }
                }
                else {
                    Print("DirectSound: Interface creation failed.\n");
                } // DirectSoundCreate
            }
            else {
                Print("DirectSound: Library DSound.dll not found.\n");
            } // libary dsound.dll exists
        }

        void EasyInitialization(HWND windowHandle) {
            int samplesPerSecond = 48000;
            // int16 == signed short
            int bytesPerSample = sizeof(signed short) * 2;
            int bufferSize = bytesPerSample * samplesPerSecond;
            Initialize(windowHandle, samplesPerSecond, bufferSize);
        }
    
        void ProcessFrameSound(int samplesPerSecond, int bytesPerSample) {
            // Play sounds!
            // . https://hero.handmade.network/episode/code/day008/
            // . * A square wave oscillates between "full-positive" to "full-negative" every half period
            // . * A Stereo (2-channel) 16-bit PCM audio buffer is arranged as an array of signed int16 values in (left channel value, right channel value) pairs
            // . * A "sample" sometimes refers to the values for all channels in a sampling period, and sometimes a value for a single channel. Be careful.
            // . The procedure for writing sound data into a buffer is as follows:
            // . 1. Figure out where in the buffer you want to start writing, and how much data you want to write
            // .     Its useful to look at the play cursor - IDirectSoundBuffer8::GetCurrentPosition()
            // . 2. Acquire a lock on the buffer - IDirectSoundBuffer8::Lock()
            // .     Because we are working with a circular buffer, this call will return 1 or 2 writable regions
            // . 3. Write the samples to the buffer
            // . 4. Unlock the regions - IDirectSoundBuffer8::Unlock()
            {
                // Get DSound buffer cursors
                DWORD playCursor, writeCursor;
                {
                    // We have the position of the play cursor and the write cursor.
                    // https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee418062(v=vs.85)
                    // "The write cursor is the point in the buffer ahead of which it is safe to write data to the buffer.
                    // . Data should not be written to the part of the buffer after the play cursor and before the write cursor."
                    HRESULT result = globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor);
                    if (result != DS_OK) {
                        switch (result) {
                            case DSERR_INVALIDPARAM: {
                                assert(false && "DirectSound Buffer GetCurrentPosition DSERR_INVALIDPARAM");
                            } break;
                            case DSERR_PRIOLEVELNEEDED: {
                                assert(false && "DirectSound Buffer GetCurrentPosition DSERR_PRIOLEVELNEEDED");
                            } break;
                            default: {
                                assert(false && "Unreachable error code (DSOUND Buffer GetCurrentPosition)");
                            } break;
                        }
                    }
                }
                

                // Figure out how many bytes to write in the buffer
                int bytesToWrite;
                // FIXME: Just for now
                int framesPerSecond = 60;

                int samplesPerFrame = (int)((float)samplesPerSecond/(float)framesPerSecond);
                int bytesPerFrame = samplesPerFrame * bytesPerSample;

                bytesToWrite = bytesPerFrame;

                // Lock the audio buffer
                // We will receive up to 2 "buffers" to write, since it's a circular buffer, so we will have to check wether we got 1 or 2
                void* bufferPointer1 = NULL;
                unsigned long bufferSize1 = 0;
                void* bufferPointer2 = NULL;
                unsigned long bufferSize2 = 0;
                DWORD lockFlags = 0;
                // Possible flags:
                // . DSBLOCK_FROMWRITECURSOR Start the lock at the write cursor. The dwOffset parameter is ignored.
                // . DSBLOCK_ENTIREBUFFER Lock the entire buffer. The dwBytes parameter is ignored.
                bool usingTwoBuffers = false;
                {
                    HRESULT result = globalSecondaryBuffer->Lock(
                        writeCursor, bytesToWrite, &bufferPointer1, &bufferSize1, &bufferPointer2, &bufferSize2, lockFlags
                    );
                    if (result != DS_OK) {
                        switch (result) {
                            case DSERR_BUFFERLOST: {
                                assert(false && "DirectSound Buffer Lock DSERR_BUFFERLOST");
                            } break;
                            case DSERR_INVALIDCALL: {
                                assert(false && "DirectSound Buffer Lock DSERR_INVALIDCALL");
                            } break;
                            case DSERR_INVALIDPARAM: {
                                assert(false && "DirectSound Buffer Lock DSERR_INVALIDPARAM");
                            } break;
                            case DSERR_PRIOLEVELNEEDED: {
                                assert(false && "DirectSound Buffer Lock DSERR_PRIOLEVELNEEDED");
                            } break;
                            default: {
                                assert(false && "Unreachable error code (DSOUND Buffer Lock)");
                            } break;
                        }
                    }
                }
                if (bufferSize1 < bytesToWrite) {
                    usingTwoBuffers = true;
                }
                else {
                    assert(bufferSize1 == bytesToWrite);
                }

                // TODO: Write the data to the buffer(s) and keep track of how much data was written in each buffer
                // The buffers are arrays of signed int16
                // int16 = signed short
                signed short* buffer1 = (signed short*) bufferPointer1;
                signed short* buffer2 = (signed short*) bufferPointer2;
                int actualAmmountOfDataWrittenToBuffer1 = 0;
                int actualAmmountOfDataWrittenToBuffer2 = 0;

                // WARNING: I legit have no clue what I'm doing!!
                // I'm trying to make a square sound wave of 60 hz
                int hz = 60; // "oscillations"(no idea how to call this, gotta study sound basics lol) per second
                int oscillationsPerFrame = (int)((float)hz / (float)framesPerSecond);
                int volume = 16000;
                int maxVol = volume;
                int minVol = -volume;
                
                int ammountOfSamplesPerOscillation = (int)((float)samplesPerFrame / (float)oscillationsPerFrame);
                int ammountOfSamplesPerHalfOscillation = ammountOfSamplesPerOscillation / 2;

                int sampleCounter = 0;
                if (usingTwoBuffers) {
                    for(int i = 0; i < bufferSize1/sizeof(signed short); i++) {
                        if (sampleCounter%ammountOfSamplesPerOscillation < ammountOfSamplesPerHalfOscillation) {
                            buffer1[i] = (signed short)maxVol;
                        }
                        else {
                            buffer1[i] = (signed short)minVol;
                        }
                        sampleCounter++;
                    }
                    for(int i = 0; i < bufferSize2/sizeof(signed short); i++) {
                        if (sampleCounter%ammountOfSamplesPerOscillation < ammountOfSamplesPerHalfOscillation) {
                            buffer2[i] = (signed short)maxVol;
                        }
                        else {
                            buffer2[i] = (signed short)minVol;
                        }
                        sampleCounter++;
                    }
                    actualAmmountOfDataWrittenToBuffer1 = bufferSize1/sizeof(signed short) * sizeof(signed short);
                    actualAmmountOfDataWrittenToBuffer2 = bufferSize2/sizeof(signed short) * sizeof(signed short);
                }
                else {
                    for(int i = 0; i < bufferSize1/sizeof(signed short); i++) {
                        if (sampleCounter%ammountOfSamplesPerOscillation < ammountOfSamplesPerHalfOscillation) {
                            buffer1[i] = (signed short)maxVol;
                        }
                        else {
                            buffer1[i] = (signed short)minVol;
                        }
                        sampleCounter++;
                    }
                    actualAmmountOfDataWrittenToBuffer1 = bufferSize1/sizeof(signed short) * sizeof(signed short);
                }
                assert(actualAmmountOfDataWrittenToBuffer1 + actualAmmountOfDataWrittenToBuffer2 == bytesToWrite);

                // Unlock the buffers
                {
                    HRESULT result = globalSecondaryBuffer->Unlock(
                        bufferPointer1, actualAmmountOfDataWrittenToBuffer1, bufferPointer2, actualAmmountOfDataWrittenToBuffer2
                    );
                    if (result != DS_OK) {
                        switch (result) {
                            case DSERR_INVALIDCALL: {
                                assert(false && "DirectSound Buffer Unlock DSERR_INVALIDCALL");
                            } break;
                            case DSERR_INVALIDPARAM: {
                                assert(false && "DirectSound Buffer Unlock DSERR_INVALIDPARAM");
                            } break;
                            case DSERR_PRIOLEVELNEEDED: {
                                assert(false && "DirectSound Buffer Unlock DSERR_PRIOLEVELNEEDED");
                            } break;
                            default: {
                                assert(false && "Unreachable error code (DSOUND Buffer Unlock)");
                            } break;
                        }
                    }
                }

                // Finally, play the buffer!
                {
                    // https://docs.microsoft.com/en-us/previous-versions/windows/desktop/mt708933(v=vs.85)
                    // First 2 parameters are reserved and should always be 0
                    // Only flag available is DSBPLAY_LOOPING
                    HRESULT result = globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
                    if (result != DS_OK) {
                        switch (result) {
                            case DSERR_BUFFERLOST: {
                                assert(false && "DirectSound Buffer Play DSERR_BUFFERLOST");
                            } break;
                            case DSERR_INVALIDCALL: {
                                assert(false && "DirectSound Buffer Play DSERR_INVALIDCALL");
                            } break;
                            case DSERR_INVALIDPARAM: {
                                assert(false && "DirectSound Buffer Play DSERR_INVALIDPARAM");
                            } break;
                            case DSERR_PRIOLEVELNEEDED: {
                                assert(false && "DirectSound Buffer Play DSERR_PRIOLEVELNEEDED");
                            } break;
                            default: {
                                assert(false && "Unreachable error code (DSOUND Buffer Play)");
                            } break;
                        }
                    }
                }
            }
        }
    }
}

#include "resources.h"
int WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
    bool isExternalConsole = Win32::GetConsole();
    const char windowClassName[] = "windowClass";
    auto windowClass = Win32::MakeWindowClass(windowClassName, Win32::BasicWindowProc, hInst);
    auto windowHandle = Win32::MakeWindow(windowClassName, "MyWindow!", hInst, cmdshow);
    auto deviceContextHandle = Win32::GetDeviceContextHandle(windowHandle);
    
    int windowW, windowH, windowX, windowY;
    Win32::GetWindowSizeAndPosition(windowHandle, &windowW, &windowH, &windowX, &windowY, false);
    int clientW, clientH;
    Win32::GetClientSize(windowHandle, &clientW, &clientH, false);

    if (isExternalConsole) {
        HWND consoleWindowHandle = GetConsoleWindow();
        int consoleW, consoleH, consoleX, consoleY;
        Win32::GetWindowSizeAndPosition(consoleWindowHandle, &consoleW, &consoleH, &consoleX, &consoleY, false);
        // Moving the console doesn't redraw it, so parts of the window that were originally hidden won't be rendered.
        MoveWindow(consoleWindowHandle, windowX+windowW, windowY, consoleW, consoleH, 0);
        // So after moving the window, redraw it.
        // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-redrawwindow
        // "If both the hrgnUpdate and lprcUpdate parameters are NULL, the entire client area is added to the update region."
        RedrawWindow(consoleWindowHandle, NULL, NULL, RDW_INVALIDATE);
    }
    
    Win32::GL::InitializeWGlContext(deviceContextHandle);
    Win32::GL::GetGLExtensions();
    Win32::GL::Renderer r;
    r.Initialize();
    r.LoadTexture((void*)texture_data, texture_width, texture_height);
    r.LoadShader(fshader, fshader_size, Win32::GL::Renderer::shaderType::FragmentShader);
    r.LoadShader(vshader, vshader_size, Win32::GL::Renderer::shaderType::VertexShader);
    r.GenerateShaderProgram();
    bool running = true;
    // Main loop
    while (running) {
        MSG msg;
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
        // r.vertexBuffer[0].color.Red();
        r.vertexBuffer[0].color = Win32::GL::Color::White();
        r.vertexBuffer[0].x = 0;
        r.vertexBuffer[0].y = texture_height;
        r.vertexBuffer[0].u = 0;
        r.vertexBuffer[0].v = texture_height;
        // r.vertexBuffer[1].color.Green();
        r.vertexBuffer[1].color = Win32::GL::Color::White();
        r.vertexBuffer[1].x = texture_width;
        r.vertexBuffer[1].y = texture_height;
        r.vertexBuffer[1].u = texture_width;
        r.vertexBuffer[1].v = texture_height;
        // r.vertexBuffer[2].color.Blue();
        r.vertexBuffer[2].color = Win32::GL::Color::White();
        r.vertexBuffer[2].x = texture_width;
        r.vertexBuffer[2].y = 0;
        r.vertexBuffer[2].u = texture_width;
        r.vertexBuffer[2].v = 0;
        // r.vertexBuffer[3].color.Yellow();
        r.vertexBuffer[3].color = Win32::GL::Color::White();
        r.vertexBuffer[3].x = 0;
        r.vertexBuffer[3].y = 0;
        r.vertexBuffer[3].u = 0;
        r.vertexBuffer[3].v = 0;
        r.quadsToRender++;
        
        // TODO:
        // TextureDescription td;
        // td.pos = {0, 0};
        // td.size = {texture_width, texture_height};
        // QuadDescription desc;
        // desc.pos = {0, 0};
        // desc.size = {texture_width, texture_height};
        // desc.texture = td;
        // desc.color = Color.White
        // r.AddQuad(desc);

        // print the vertex buffer
        // for(int i = 0; i < r.quadsToRender * 4; i++) {
        //     Win32::FormattedPrint("Vertex %d: %04.4f, %04.4f, %04.4f, %04.4f, %04.4f, %04.4f, %04.4f, %04.4f\n", i,
        //         r.vertexBuffer[i].data[0], r.vertexBuffer[i].data[1], r.vertexBuffer[i].data[2], r.vertexBuffer[i].data[3],
        //         r.vertexBuffer[i].data[4], r.vertexBuffer[i].data[5], r.vertexBuffer[i].data[6], r.vertexBuffer[i].data[7]);
        // }

        r.Render(clientW, clientH, Win32::GL::Color::White(), deviceContextHandle);
        
        if (false && Win32::GL::GetErrors("Main Loop")) {
            Win32::Print("Exiting because there were gl errors!");
            running = false;
        }
    }
}