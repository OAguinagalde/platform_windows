/*
{
    "HasMain": false,
    "Includes": [ "Hola", "Que", "Tal" ]
}
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma comment(lib, "User32")
#include <cassert>
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
    void FormattedPrint(const char* format, ...) {
        static char buffer[1024];
        va_list args;
        va_start(args, format);
        wvsprintf((LPSTR) buffer, format, args);
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
        RECT rect = {};
        GetWindowRect(windowHandle, &rect);
        if (x) *x = rect.left;
        if (y) *y = rect.top;
        if (width) *width = rect.right - rect.left;
        if (height) *height = rect.bottom - rect.top;
        if (printDebug) {
            FormattedPrint("Windows at %d,%d with size %dx%d\n", *x, *y, *width, *height);
        }
    }

    // Given a windowHandle, queries the width and height of the client size (The drawable area)
    void GetClientSize(HWND windowHandle, int* width, int* height, bool printDebug) {
        RECT rect = {};
        // This just gives us the "drawable" part of the window
        GetClientRect(windowHandle, &rect);
        *width = rect.right - rect.left;
        *height = rect.bottom - rect.top;
        if (printDebug) {
            FormattedPrint("Client size %dx%d\n", *width, *height);
        }
    }

    // Try to get a console, for situations where there might not be one
    // Return true on success or false if it wasn't possible to get a console
    bool GetConsole() {
        bool consoleFound = false;
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
            }
            else {
                // AllocConsole function fails if the calling process already has a console
                FormattedPrint("Unreachable at %s, %s\n", __FUNCTIONW__, __FILE__);
            }
        }
        
        return consoleFound;
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
        int windowPositonX = 100;
        int windowPositonY = 100;
        int windowWidth = 0;
        int windowHeight = 0;
        int clientWidth = 0;
        int clientHeight = 0;
        // The size for the window will be the whole window and not the drawing area, so we will have to adjust it later on and it doesn't matter much here
        HWND windowHandle = CreateWindowEx(0, windowClassName, title, WS_POPUP | WS_OVERLAPPED | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU  | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
            windowPositonX, windowPositonY, 10, 10,
            NULL, NULL, hInstance, NULL
        );
        Print("Window created\n");
        GetWindowSizeAndPosition(windowHandle,&windowWidth,&windowHeight,&windowPositonX,&windowPositonY,true);
        
        // let's figure out the real size of the client area (drawable part of window) and adjust it
        int desiredClientWidth = 500;
        int desiredClientHeight = 600;
        // Get client size
        GetClientSize(windowHandle, &clientWidth, &clientHeight, true);
        // Calculate difference between initial size of window and current size of drawable area, that should be the difference to make the window big enough to have our desired drawable area
        int difference_w = abs(clientWidth - desiredClientWidth);
        int difference_h = abs(clientHeight - desiredClientHeight);
        // Set the initially desired position and size now
        MoveWindow(windowHandle, windowPositonX, windowPositonY, clientWidth + difference_w, clientHeight + difference_h, 0);
        // It should have the right size about now
        GetClientSize(windowHandle, &clientWidth, &clientHeight, true);
        Print("Window adjusted\n");
        GetWindowSizeAndPosition(windowHandle, &windowWidth, &windowHeight, &windowPositonX, &windowPositonY, true);

        ShowWindow(windowHandle, nCmdShow);
        return windowHandle;
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


// int WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
void main(int argc, char** argv) {
    Win32::Test1();
    Sleep(1000);
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
#pragma comment(lib, "Opengl32")
namespace Win32 {
    namespace GL {
        // Loops through and print all the errors related to OpenGL
        void GetErrors() {
            GLenum opengl_error = 0;
            while ((opengl_error = glGetError()) != GL_NO_ERROR) {
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
        }

        // Loops through and print (Labeled) all the errors related to OpenGL
        void GetErrors(const char* label) {
            Print(label);
            GetErrors();
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
        void InitializeGlContext(HDC DeviceContextHandle) {    
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
#pragma comment(lib, "Gdi32.lib")
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
    }
}



namespace Buffers {
    struct BufferHeader {
        unsigned long long SizeInBytes;
        unsigned long long SizeInTs;
    };
    template <typename T>
    struct Buffer {
        BufferHeader header;
        T* data;
        void Empty() {
            header.SizeInBytes = 0;
            header.SizeInTs = 0;
        }
    };
}

namespace Vertices {
    static constexpr int Components = 8;
    struct Vertex {
        union {
            float data[Components];
            // Definition
            struct {
                // 2 for position
                float x;
                float y;
                // 2 for texture uv
                float u;
                float v;
                // 4 for rgba
                float r;
                float g;
                float b;
                float a;
            };
        };
        void Red() {
            r = 1; g = 0; b = 0; a = 1;
        }
        void Green() {
            r = 0; g = 1; b = 0; a = 1;
        }
        void Blue() {
            r = 0; g = 0; b = 1; a = 1;
        }
        void Yellow() {
            r = 0; g = 1; b = 1; a = 1;
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
        void Empty() {
            for(int i = 0; i < Components; i++) {
                data[i] = 0.0f;
            }
        }
    };
    static Vertex Zero() {
        Vertex v;
        v.Empty();
        return v;
    }
}