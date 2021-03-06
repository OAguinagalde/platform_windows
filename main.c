// #include <stdio.h>
// #include <stdbool.h>
// #include <assert.h>

// TODO: If I define WIN32_LEAN_AND_MEAN then I lose the contents of mmeapi.h
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
// #define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl\GL.h>
// https://www.khronos.org/registry/OpenGL/api/GL/wglext.h
#include "wglext.h"
// https://www.khronos.org/registry/OpenGL/api/GL/glext.h
// . depends on KHR/khrplatform.h
// . https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h
#include "glext.h"
#include <DSound.h>

// Embedded data
#include "resources.h"
// A define for me to know what things are win specific and what things are not
#define win32_ 
#define dontcare_ 

typedef int bool;
#define true 1
#define false 0

typedef  signed char        int8;
typedef  unsigned char      uint8;
typedef  signed short       int16;
typedef  unsigned short     uint16;
typedef  signed long        int32;
typedef  unsigned long      uint32;
typedef  signed long long   int64;
typedef  unsigned long long uint64;

#define debug 1
#if debug
    #define assert_printf_function win32_printf
    #define assert(expression) { if (!(expression)) {assert_printf_function("Assertion failed!\n"); assert_printf_function("\t%s\n", #expression); *(int*)0 = 0;} }
#else
    #define assert(expression)
#endif

// int abs(int value) {
//     return (value < 0) ? -value : value;
// }

// TODO: if parent process is in use for attached console (powershell or cmd) I'm not sure if changing the ConsoleMode might
// break something
void win32_clearConsole() {
    static bool initialized = false;
    static win32_ HANDLE win32_console_stdout = 0;
    // To also clear the scroll back, emit L"\x1b[3J" as well.
    // 2J only clears the visible window and 3J only clears the scroll back.
    static win32_ PCWSTR win32_clearConsoleSequence = L"\x1b[2J";
    static win32_ DWORD win32_originalMode = 0;
    if (!initialized) {
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
        initialized = true;
    }
    win32_ WriteConsoleW(win32_console_stdout, win32_clearConsoleSequence, sizeof(win32_clearConsoleSequence)/sizeof((win32_clearConsoleSequence)[0]), NULL, NULL);
}

// WARNING: Not ideal but right now the first call to win32_print will set the stdout.
void win32_print(const char* string) {
    static bool initialized = false;
    static win32_ HANDLE win32_console_stdout = 0;
    if (!initialized) {
        win32_console_stdout = win32_ GetStdHandle(STD_OUTPUT_HANDLE);
        initialized = true;
    }
    win32_ WriteConsole(win32_console_stdout, (const void*) string, win32_ lstrlen((LPCSTR) string), NULL, NULL);
    // TODO: make win32_ OutputDebugString() output to my STD_OUTPUT_HANDLE
}

void win32_printf(const char* format, ...) {
    static char buffer[1024];
    va_list args;
    va_start(args, format);
    // WARNING. There is a bunch of restrictions with wvsprintf.
    win32_ wvsprintf((LPSTR) &buffer, format, args);
    va_end(args);
    win32_print(&buffer[0]);
}

void formattedString(char* buffer, const char* format, ...) {
    va_list args;
    va_start(args, format);
    // WARNING. There is a bunch of restrictions with wvsprintf.
    win32_ wvsprintf((LPSTR) buffer, format, args);
    va_end(args);
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
    // both GetProcAddress and wglGetProcAddress return NULL on failure, so just check for return value to see if function was found
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
    if (! win32_ wglMakeCurrent(win32_DeviceContextHandle, win32_GLRenderingContextHandle)) {
        win32_print("Error on wglMakeCurrent");
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
        case WM_DESTROY: {
            // TODO: turn win32_running to false
            win32_print("WM_DESTROY\n");
        } // break;
        win32_print("and\n");
        case WM_CLOSE: {
            // TODO: turn win32_running to false
            win32_print("WM_CLOSE\n");
            // Basically makes the application post a WM_QUIT message
            // win32_ DestroyWindow(hwnd); // This only closes the main window
            win32_ PostQuitMessage(0); // This sends the quit message which the main loop will read and end the loop
            return 0;
        } break;
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
    // Events that I'm consciously not capturing:
    // . WM_SETCURSOR Sent to a window if the mouse causes the cursor to move within a window and mouse input is not captured
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


// Direct Sound stuff
// https://docs.microsoft.com/en-us/previous-versions/windows/desktop/mt708921(v=vs.85)
typedef HRESULT WINAPI win32_directSound_directSoundCreate_t(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
// TODO: For now this is global
static LPDIRECTSOUNDBUFFER win32_directSound_globalSecondaryBuffer;

win32_ void win32_directSound_initialize(HWND win32_windowHandle, int SamplesPerSecond, int BufferSize) {
    // Load the library dinamically, allowing to deal with the library not existing if that's the case
    HMODULE win32_directSound_library = win32_ LoadLibraryA("dsound.dll");
    if(win32_directSound_library) {
        win32_printf("DirectSound: Library loaded.\n");
        // To avoid linking a whole library for a single function that I need (DirectSoundCreate). Instead get the address of that function in the library itself.
        // This also allows to load the sound library dinamically, so that if the library doesn't exist then I can just keep running without using sound.
        win32_directSound_directSoundCreate_t *DirectSoundCreate = (win32_directSound_directSoundCreate_t *) win32_ GetProcAddress(win32_directSound_library, "DirectSoundCreate");

        // The Direct Sound API seems to be made in an object oriented kind of way. They basically hardcode the vtable that would be automatically created in c++
        // and in order to use I just have to explicitly invoke the operations of the "object" through the lpVtbl member and pass the address of the object as the
        // first parameter to every operation. An example is:
        // win32_directSound_object->lpVtbl->SetCooperativeLevel(&win32_directSound_object, win32_windowHandle, DSSCL_PRIORITY)
        struct IDirectSound* win32_directSound_object;
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &win32_directSound_object, 0))) {
            win32_printf("DirectSound: Interface created.\n");
            // For some reason the WAVEFORMAT is not defined in DirectSound header... So I just copied instead to my source... Not sure I should be doing this, but
            // as long as it works......
            WAVEFORMATEX win32_directSound_waveFormat = (WAVEFORMATEX){0};
            win32_directSound_waveFormat.wFormatTag = WAVE_FORMAT_PCM;
            // Stereo Sound
            win32_directSound_waveFormat.nChannels = 2;
            win32_directSound_waveFormat.nSamplesPerSec = SamplesPerSecond;
            // 16 bits per sample because... why not?
            win32_directSound_waveFormat.wBitsPerSample = 16;
            // nBlockAlign:
            // . https://docs.microsoft.com/en-us/previous-versions/windows/desktop/bb280529(v=vs.85)
            // . https://docs.microsoft.com/en-us/windows/win32/api/mmeapi/ns-mmeapi-waveformatex
            // . "Block alignment, in bytes.
            // . The value of the BlockAlign property must be equal to the product of Channels and BitsPerSample divided by 8 (bits per byte).
            // . Software must process a multiple of nBlockAlign bytes of data at a time."
            win32_directSound_waveFormat.nBlockAlign = (win32_directSound_waveFormat.nChannels*win32_directSound_waveFormat.wBitsPerSample) / 8;
            // nAvgBytesPerSec:
            // . "Should be equal to the product of nSamplesPerSec and nBlockAlign"
            win32_directSound_waveFormat.nAvgBytesPerSec = win32_directSound_waveFormat.nSamplesPerSec*win32_directSound_waveFormat.nBlockAlign;
            win32_directSound_waveFormat.cbSize = 0;

            // Cooperation level let's windows know how the application sound should interact with the rest of applications. Kind of
            if(SUCCEEDED(win32_directSound_object->lpVtbl->SetCooperativeLevel(win32_directSound_object, win32_windowHandle, DSSCL_PRIORITY))) {
                win32_printf("DirectSound: CooperationLevel changed.\n");

                // As far as I know, the reason we have to create a main buffer (which we won't use) and then later on a "secondary buffer"
                // which we will actually work with is because nowadays windows (?) doesn't allow to write to the sound hardware directly (the main buffer)
                // and instead every application uses a secondary buffer, and later on windows mix every secondary buffer and does the necessary work with
                // that main buffer. Something like that...
                DSBUFFERDESC win32_directSound_bufferDescription = (DSBUFFERDESC){0};
                win32_directSound_bufferDescription.dwSize = sizeof(win32_directSound_bufferDescription);
                win32_directSound_bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                struct IDirectSoundBuffer* win32_directSound_primaryBuffer;
                if(SUCCEEDED(win32_directSound_object->lpVtbl->CreateSoundBuffer(win32_directSound_object, &win32_directSound_bufferDescription, &win32_directSound_primaryBuffer, 0))) {
                    win32_printf("DirectSound: Primary buffer created successfully.\n");
                    HRESULT ret = win32_directSound_primaryBuffer->lpVtbl->SetFormat(win32_directSound_primaryBuffer, (const WAVEFORMATEX*) &win32_directSound_waveFormat);
                    if(SUCCEEDED(ret)) {
                        win32_printf("DirectSound: Primary buffer format was set.\n");
                    }
                    else {
                        win32_printf("DirectSound: Primary buffer formatting failed.\n");
                    }
                }
                else {
                    win32_printf("DirectSound: Primary buffer creation failed.\n");
                } // create primary sound buffer
            }
            else {
                win32_printf("DirectSound: CooperationLevel change failed.\n");
            } // coop level

            DSBUFFERDESC win32_directSound_bufferDescription = (DSBUFFERDESC){0};
            win32_directSound_bufferDescription.dwSize = sizeof(win32_directSound_bufferDescription);
            win32_directSound_bufferDescription.dwFlags = 0;
            win32_directSound_bufferDescription.dwBufferBytes = BufferSize;
            win32_directSound_bufferDescription.lpwfxFormat = &win32_directSound_waveFormat;
            HRESULT ret = win32_directSound_object->lpVtbl->CreateSoundBuffer(win32_directSound_object, &win32_directSound_bufferDescription, &win32_directSound_globalSecondaryBuffer, 0);
            if(SUCCEEDED(ret)) {
                win32_printf("DirectSound: Secondary buffer created successfully.\n");
            }
            else {
                win32_printf("DirectSound: Secondary buffer creation failed.\n");
            }
        }
        else {
            win32_printf("DirectSound: Interface creation failed.\n");
        } // DirectSoundCreate
    }
    else {
        win32_printf("DirectSound: Library DSound.dll not found.\n");
    } // libary dsound.dll exists
}

win32_ int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* win32_cmdLine, int nCmdShow) {
    // Set up console
    bool win32_console_attached = false;
    bool win32_console_allocated = false;
    HWND win32_console_handle;
    {
        // Attaches to the calling process console, so if called from cmd it starts showing output in there...
        // although you can still interact normally with that cmd... it's weird.
        if (win32_ AttachConsole(ATTACH_PARENT_PROCESS)) {
            // If it worked, then chances are that the console is being shared between the whatever process had it open and this application,
            // which can be weird. If calling process is something like cmd or powershell, then print a new line character to have a "clean start"
            win32_print("\n");
            win32_console_attached = true;
        }
        else {
            // if AttachConsole fails means that there is no parent process console open, so just allocate a new one
            if (win32_ AllocConsole()) {
                win32_console_attached = true;
                win32_console_allocated = true;
            }
            else {
                assert(false && "Not really Unreachable... But like what the heck? alloc console failed?");
            }
        }
        if (win32_console_attached) {
            win32_console_handle = win32_ GetConsoleWindow();
            if (win32_console_handle == NULL) {
                assert(false && "Unreachable! GetConsoleWindow should never fail if there is a console attached");
            }
        }
        win32_print("Console set.\n");
    }

    const char win32_windowClassName[]  = "Window Class Text";
    WNDCLASSA win32_windowClass = (WNDCLASSA) {0};
    win32_windowClass.lpfnWndProc = WindowProc;
    win32_windowClass.hInstance = hInstance;
    win32_windowClass.lpszClassName = win32_windowClassName;
    win32_windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
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

    // Init DirectSound
    int win32_directSound_samplesPerSecond = 48000;
    int win32_directSound_bytesPerSample = sizeof(int16) * 2;
    int win32_directSound_bufferSize = win32_directSound_bytesPerSample * win32_directSound_samplesPerSecond;
    win32_directSound_initialize(win32_windowHandle, win32_directSound_samplesPerSecond, win32_directSound_bufferSize);

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
    
    // If console is allocated (is not a parent console like powershell) then position it next to the main window side by side.
    if (win32_console_allocated){
        // Just always put the console right of main window
        int width, height, x, y;
        win32_getWindowSizeAndPosition(win32_console_handle, &width, &height, &x, &y, true);
        // Moving the console doesn't redraw it, so parts of the window that were originally hidden won't be rendered.
        win32_ MoveWindow(win32_console_handle, win32_windowPositonX+win32_windowWidth, win32_windowPositonY, width, height, 0);
        // So after moving the window, redraw it.
        // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-redrawwindow
        // "If both the hrgnUpdate and lprcUpdate parameters are NULL, the entire client area is added to the update region."
        win32_ RedrawWindow(win32_console_handle, NULL, NULL, RDW_INVALIDATE);
        
        win32_printf("Console final size and position\n");
        win32_getWindowSizeAndPosition(win32_console_handle, &width, &height, &x, &y, true);
        win32_getClientSize(win32_console_handle, &width, &height, true);
        win32_printf("Window final size and position\n");
        win32_getWindowSizeAndPosition(win32_windowHandle,&win32_windowWidth,&win32_windowHeight,&win32_windowPositonX,&win32_windowPositonY,true);
        win32_getClientSize(win32_windowHandle, &width, &height, true);
    }

    // Some gl code
    const int quads = 1;
    GLuint opengl_texture;
    // TODO: Clean this stuff
    GLuint opengl_vertexBuffer;
    GLuint opengl_indexBuffer;
    GLuint renderer_shaderProgramObject;
    GLuint renderer_vertexArrayObject;
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
    // TODO: This is just for testing, eventually get rid of this and use the already made vertex buffer for the renderer
    float vertices_testrenderer [4*(2+2+4)] = {
        // x, y, u, v, r, g, b, a
        // 0.5f,  0.5f, 1.0f, 1.0f,  1.0f, 0.0f, 0.0f, 1.0f,      // top right
        // 0.5f, -0.5f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f,      // bottom right
        // -0.5f, -0.5f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f,      // bottom left
        // -0.5f,  0.5f, 0.0f, 1.0f,  1.0f, 1.0f, 0.0f, 1.0f,      // top left 
        // /*Positions...*/ 0*win32_clientWidth + win32_clientWidth*0.1f, 1*win32_clientHeight - win32_clientHeight*0.1f, /*Textures...*/ 0, 1, /*Colors...*/ 1, 1, 1, 1,
        // /*Positions...*/ 1*win32_clientWidth - win32_clientWidth*0.1f, 1*win32_clientHeight - win32_clientHeight*0.1f, /*Textures...*/ 1, 1, /*Colors...*/ 1, 1, 1, 1,
        // /*Positions...*/ 1*win32_clientWidth - win32_clientWidth*0.1f, 0*win32_clientHeight + win32_clientHeight*0.1f, /*Textures...*/ 1, 0, /*Colors...*/ 1, 1, 1, 1,
        // /*Positions...*/ 0*win32_clientWidth + win32_clientWidth*0.1f, 0*win32_clientHeight + win32_clientHeight*0.1f, /*Textures...*/ 0, 0, /*Colors...*/ 1, 1, 1, 1,
        /*Positions...*/ 0*win32_clientWidth + win32_clientWidth*0.1f, 1*win32_clientHeight - win32_clientHeight*0.1f, /*Textures...*/ 0, constexpr_texture_height, /*Colors...*/ 1, 1, 1, 1,
        /*Positions...*/ 1*win32_clientWidth - win32_clientWidth*0.1f, 1*win32_clientHeight - win32_clientHeight*0.1f, /*Textures...*/ constexpr_texture_width, constexpr_texture_height, /*Colors...*/ 1, 1, 1, 1,
        /*Positions...*/ 1*win32_clientWidth - win32_clientWidth*0.1f, 0*win32_clientHeight + win32_clientHeight*0.1f, /*Textures...*/ constexpr_texture_width, 0, /*Colors...*/ 1, 1, 1, 1,
        /*Positions...*/ 0*win32_clientWidth + win32_clientWidth*0.1f, 0*win32_clientHeight + win32_clientHeight*0.1f, /*Textures...*/ 0, 0, /*Colors...*/ 1, 1, 1, 1,
        
    };
    #define opengl_loadExtension(type, name) type name = NULL; name = opengl_getFunctionAddress(#name); assert(name && #name);
    opengl_loadExtension(PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT);
    opengl_loadExtension(PFNGLGENBUFFERSPROC, glGenBuffers);
    opengl_loadExtension(PFNGLBINDBUFFERPROC, glBindBuffer);
    opengl_loadExtension(PFNGLBUFFERDATAPROC, glBufferData);
    opengl_loadExtension(PFNGLCREATESHADERPROC, glCreateShader);
    opengl_loadExtension(PFNGLSHADERSOURCEPROC, glShaderSource);
    opengl_loadExtension(PFNGLCOMPILESHADERPROC, glCompileShader);
    opengl_loadExtension(PFNGLGETSHADERIVPROC, glGetShaderiv);
    opengl_loadExtension(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
    opengl_loadExtension(PFNGLBLENDFUNCSEPARATEPROC, glBlendFuncSeparate);
    opengl_loadExtension(PFNGLCREATEPROGRAMPROC, glCreateProgram);
    opengl_loadExtension(PFNGLATTACHSHADERPROC, glAttachShader);
    opengl_loadExtension(PFNGLLINKPROGRAMPROC, glLinkProgram);
    opengl_loadExtension(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
    opengl_loadExtension(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
    opengl_loadExtension(PFNGLDELETESHADERPROC, glDeleteShader);
    opengl_loadExtension(PFNGLUSEPROGRAMPROC, glUseProgram);
    opengl_loadExtension(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
    opengl_loadExtension(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
    opengl_loadExtension(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
    opengl_loadExtension(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
    opengl_loadExtension(PFNGLACTIVETEXTUREPROC, glActiveTexture);
    opengl_loadExtension(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);
    opengl_loadExtension(PFNGLUNIFORM2FVPROC, glUniform2fv);
    opengl_loadExtension(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
    {
        wglSwapIntervalEXT(1);
        
        glEnable(GL_TEXTURE_2D);
        glGenTextures(1, &opengl_texture);
        // This is not really necessary because TEXTURE0 is always activated by default.
        // But I would require to do this with the other texture units
        // glActiveTexture(GL_TEXTURE0);
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

        // Sprite Batching Renderer
        typedef struct {
            union {
                union {
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
                float data[2+2+4];
            };
        } renderer_vertex;
        const int renderer_sizeOfVertex = sizeof(renderer_vertex);
        #define constexpr_renderer_maxQuads 1000
        #define constexpr_renderer_maxVertices (constexpr_renderer_maxQuads * 4)
        #define constexpr_renderer_maxIndices (constexpr_renderer_maxQuads * 6)
        const int renderer_maxQuads = constexpr_renderer_maxQuads;
        const int renderer_maxVertices = constexpr_renderer_maxVertices;
        const int renderer_maxIndices = constexpr_renderer_maxIndices;
        renderer_vertex renderer_vertices[constexpr_renderer_maxVertices];
        for (int i = 0; i < constexpr_renderer_maxVertices; i++) {
            renderer_vertices[i].data[0] = 0;
            renderer_vertices[i].data[1] = 0;
            renderer_vertices[i].data[2] = 0;
            renderer_vertices[i].data[3] = 0;
            renderer_vertices[i].data[4] = 0;
            renderer_vertices[i].data[5] = 0;
            renderer_vertices[i].data[6] = 0;
            renderer_vertices[i].data[7] = 0;
        }
        unsigned long renderer_indices[constexpr_renderer_maxIndices];
        // Initialize index buffer since it will be const
        for (int i = 0; i < renderer_maxQuads; i++) {
            int vertex = i * 4; // 4 vertices per quad
            int index = i * 6; // 6 indices per quad
            renderer_indices[index + 0] = vertex + 0;
            renderer_indices[index + 1] = vertex + 1;
            renderer_indices[index + 2] = vertex + 2;
            renderer_indices[index + 3] = vertex + 0;
            renderer_indices[index + 4] = vertex + 2;
            renderer_indices[index + 5] = vertex + 3;
        }
        // Shaders (taken from the embedded resources)
        const char* renderer_vertexShader = &vshader[0];
        const int renderer_vertexShaderSize = vshader_size;
        const char* renderer_fragmentShader = &fshader[0];
        const int renderer_fragmentShaderSize = fshader_size;

        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

        // VAO
        // GLuint renderer_vertexArrayObject;
        glGenVertexArrays(1, &renderer_vertexArrayObject);
        glBindVertexArray(renderer_vertexArrayObject);

        // Index Buffer
        GLuint renderer_elementBufferObject;
        glGenBuffers(1, &renderer_elementBufferObject);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer_elementBufferObject);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(renderer_indices), renderer_indices, GL_STATIC_DRAW);
        
        // VBO
        GLuint renderer_vertexBufferObject;
        glGenBuffers(1, &renderer_vertexBufferObject);
        glBindBuffer(GL_ARRAY_BUFFER, renderer_vertexBufferObject);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_testrenderer), vertices_testrenderer, GL_STATIC_DRAW);
        // TODO: setup the renderer to work with a dynamically vertex buffer
        // glBufferData(GL_ARRAY_BUFFER, sizeof(renderer_vertices), renderer_vertices, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(0));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Shader stuff
        GLuint renderer_vertexShaderObject;
        GLuint renderer_fragmentShaderObject;
        renderer_vertexShaderObject = glCreateShader(GL_VERTEX_SHADER);
        renderer_fragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(renderer_vertexShaderObject, 1, (const GLchar *const *) &renderer_vertexShader, NULL);
        glShaderSource(renderer_fragmentShaderObject, 1, (const GLchar *const *) &renderer_fragmentShader, NULL);
        glCompileShader(renderer_vertexShaderObject);
        {
            int success;
            char info[512];
            glGetShaderiv(renderer_vertexShaderObject, GL_COMPILE_STATUS, &success);
            if(!success) {
                glGetShaderInfoLog(renderer_vertexShaderObject, 512, NULL, info);
                win32_printf("Error compiling vertex shader:\n\t%s", &info[0]);
            }
        }
        glCompileShader(renderer_fragmentShaderObject);
        {
            int success;
            char info[512];
            glGetShaderiv(renderer_fragmentShaderObject, GL_COMPILE_STATUS, &success);
            if(!success) {
                glGetShaderInfoLog(renderer_fragmentShaderObject, 512, NULL, info);
                win32_printf("Error compiling fragment shader:\n\t%s", &info[0]);
            }
        }
        // GLuint renderer_shaderProgramObject;
        renderer_shaderProgramObject = glCreateProgram();

        glAttachShader(renderer_shaderProgramObject, renderer_vertexShaderObject);
        glAttachShader(renderer_shaderProgramObject, renderer_fragmentShaderObject);
        glLinkProgram(renderer_shaderProgramObject);
        {
            int success;
            char info[512];
            glGetProgramiv(renderer_shaderProgramObject, GL_LINK_STATUS, &success);
            if(!success) {
                glGetProgramInfoLog(renderer_shaderProgramObject, 512, NULL, info);
                win32_printf("Error linking shader program:\n\t%s", &info[0]);
            }
        }
        glDeleteShader(renderer_vertexShaderObject);
        glDeleteShader(renderer_fragmentShaderObject);
        opengl_getErrorsAt("[glerrors] After shader program creation...");
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

    // Main Loop
    bool win32_running = true;
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
                    win32_running = false;
                } break;
                case WM_SIZE: {
                    
                } break;
            }
        }

        static int fps = 0;
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
            // Change title to show fps
            {
                static char win32_window_text[1024];
                formattedString(&win32_window_text[0], "%d", fps);
                SetWindowText(win32_windowHandle, win32_window_text);
            }
            // win32_printf("win32_clientRectangleWidth: %d\n", win32_clientRectangleWidth);
            // win32_printf("win32_clientRectangleHeight: %d\n", win32_clientRectangleHeight);
        }
        
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
                HRESULT result = win32_directSound_globalSecondaryBuffer->lpVtbl->GetCurrentPosition(win32_directSound_globalSecondaryBuffer, &playCursor, &writeCursor);
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
            // {
                // FIXME: Just for now
                int framesPerSecond = 60;
                int samplesPerSecond = win32_directSound_samplesPerSecond;
                int bytesPerSample = win32_directSound_bytesPerSample;

                int samplesPerFrame = (int)((float)samplesPerSecond/(float)framesPerSecond);
                int bytesPerFrame = samplesPerFrame * bytesPerSample;

                bytesToWrite = bytesPerFrame;
            // }

            // Lock the audio buffer
            // We will receive up to 2 "buffers" to write, since it's a circular buffer, so we will have to check wether we got 1 or 2
            void* bufferPointer1 = NULL;
            int bufferSize1 = 0;
            void* bufferPointer2 = NULL;
            int bufferSize2 = 0;
            DWORD lockFlags = 0;
            // Possible flags:
            // . DSBLOCK_FROMWRITECURSOR Start the lock at the write cursor. The dwOffset parameter is ignored.
            // . DSBLOCK_ENTIREBUFFER Lock the entire buffer. The dwBytes parameter is ignored.
            bool usingTwoBuffers = false;
            {
                HRESULT result = win32_directSound_globalSecondaryBuffer->lpVtbl->Lock(
                    win32_directSound_globalSecondaryBuffer, writeCursor, bytesToWrite, &bufferPointer1, &bufferSize1, &bufferPointer2, &bufferSize2, lockFlags
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
            int16* buffer1 = (int16*) bufferPointer1;
            int16* buffer2 = (int16*) bufferPointer2;
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
                for(int i = 0; i < bufferSize1/sizeof(int16); i++) {
                    if (sampleCounter%ammountOfSamplesPerOscillation < ammountOfSamplesPerHalfOscillation) {
                        buffer1[i] = (int16)maxVol;
                    }
                    else {
                        buffer1[i] = (int16)minVol;
                    }
                    sampleCounter++;
                }
                for(int i = 0; i < bufferSize2/sizeof(int16); i++) {
                    if (sampleCounter%ammountOfSamplesPerOscillation < ammountOfSamplesPerHalfOscillation) {
                        buffer2[i] = (int16)maxVol;
                    }
                    else {
                        buffer2[i] = (int16)minVol;
                    }
                    sampleCounter++;
                }
                actualAmmountOfDataWrittenToBuffer1 = bufferSize1/sizeof(int16) * sizeof(int16);
                actualAmmountOfDataWrittenToBuffer2 = bufferSize2/sizeof(int16) * sizeof(int16);
            }
            else {
                for(int i = 0; i < bufferSize1/sizeof(int16); i++) {
                    if (sampleCounter%ammountOfSamplesPerOscillation < ammountOfSamplesPerHalfOscillation) {
                        buffer1[i] = (int16)maxVol;
                    }
                    else {
                        buffer1[i] = (int16)minVol;
                    }
                    sampleCounter++;
                }
                actualAmmountOfDataWrittenToBuffer1 = bufferSize1/sizeof(int16) * sizeof(int16);
            }
            assert(actualAmmountOfDataWrittenToBuffer1 + actualAmmountOfDataWrittenToBuffer2 == bytesToWrite);

            // Unlock the buffers
            {
                HRESULT result = win32_directSound_globalSecondaryBuffer->lpVtbl->Unlock(
                    win32_directSound_globalSecondaryBuffer, bufferPointer1, actualAmmountOfDataWrittenToBuffer1, bufferPointer2, actualAmmountOfDataWrittenToBuffer2
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
                HRESULT result = win32_directSound_globalSecondaryBuffer->lpVtbl->Play(
                    win32_directSound_globalSecondaryBuffer, 0, 0, DSBPLAY_LOOPING
                );
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
        // Rendering stuff
        {
            glViewport(0, 0, win32_clientWidth, win32_clientHeight);
            glClearColor(1.0f, 0.5f, 0.0f, 0.5f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            #define ToColumnMajor(v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15) v0,v4,v8,v12,v1,v5,v9,v13,v2,v6,v10,v14,v3,v7,v11,v15

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

            glBindTexture(GL_TEXTURE_2D, opengl_texture);
            
            // TODO: Each frame update the vertex buffer data
            
            GLfloat texture_dimensions[2];
            texture_dimensions[0] = texture_width;
            texture_dimensions[1] = texture_height;

            glUseProgram(renderer_shaderProgramObject);
            GLint shader_uniformPosition_mvp = glGetUniformLocation(renderer_shaderProgramObject, "mvp");
            GLint shader_uniformPosition_texture_dimensions = glGetUniformLocation(renderer_shaderProgramObject, "texture_dimensions");
            glUniformMatrix4fv(shader_uniformPosition_mvp, 1, GL_FALSE, matrix_projection);
            glUniform2fv(shader_uniformPosition_texture_dimensions, 1, &texture_dimensions[0]);
            glBindVertexArray(renderer_vertexArrayObject);
            static int quads_ready_to_render = 1;
            glDrawElements(GL_TRIANGLES, quads_ready_to_render * 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
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
            fps = win32_frequency_seconds / win32_counterDifference_lastFrame;

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
    if (win32_console_attached) {
        win32_ FreeConsole();
        win32_console_allocated = false;
        win32_console_attached = false;
    }
    return 0;
}
