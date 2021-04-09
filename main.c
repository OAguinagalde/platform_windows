#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include <windows.h>
#include <gl\GL.h>

void renderer_initialize() {
    int renderer_maxQuadCount = 10000;
    // 2position 4color 2textuv
    int vertex_elementCount = 8;
    // 8 floats per vertex * 4 vertex per quad * max ammount of quads
    float* renderer_vertexBuffer = malloc(sizeof(float)*vertex_elementCount*4*renderer_maxQuadCount);
    assert(renderer_vertexBuffer);
    // 6 indices per quad * max ammount of quads
    int *renderer_indexBuffer = malloc(1, sizeof(int)*6*renderer_maxQuadCount);
    assert(renderer_indexBuffer);
    
    // Populate the index buffer
    int vertices = 0;
    int indices = 0;
    for (int i = 0; i < renderer_maxQuadCount; i++) {
        vertices = i * 4;
        indices = i * 6;
        renderer_indexBuffer[indices + 0] = vertices + 0;
        renderer_indexBuffer[indices + 1] = vertices + 1;
        renderer_indexBuffer[indices + 2] = vertices + 2;
        renderer_indexBuffer[indices + 3] = vertices + 0;
        renderer_indexBuffer[indices + 4] = vertices + 2;
        renderer_indexBuffer[indices + 5] = vertices + 3;
    }

}

// https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions
void* opengl_getFunctionAddress(const char* functionName) {
    void* function = (void*) wglGetProcAddress(functionName);
    // "While the MSDN documentation says that wglGetProcAddress returns NULL on failure,
    // some implementations will return other values. 1, 2, and 3 are used, as well as -1."
    if (function == 0 || function == (void*) 0x1 || function == (void*) 0x2 || function == (void*) 0x3 || function == (void*)-1) {
        // Some functions can only be found with GetProceAddress aparently...
        HMODULE module = LoadLibraryA("opengl32.dll");
        function = (void*) GetProcAddress(module, functionName);
    }

    return function;
}

void opengl_initialize(HDC win32_DeviceContextHandle) {    
    // First we need to get a pixel format that we want OpenGL to use
    // But we don't know what we can use, so we first describe our ideal pixel format
    PIXELFORMATDESCRIPTOR win32_desiredPixelFormat = (PIXELFORMATDESCRIPTOR) {0};
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
    int win32_suggestedPixelFormatIndex = ChoosePixelFormat(win32_DeviceContextHandle, &win32_desiredPixelFormat);
    PIXELFORMATDESCRIPTOR win32_chosenPixelFormat;
    // But we need to know the complete specification of that suggested pixel format, so also ask windows
    // for that specification fo that one pixel format
    DescribePixelFormat(win32_DeviceContextHandle, win32_suggestedPixelFormatIndex, sizeof(win32_chosenPixelFormat), &win32_chosenPixelFormat);
    // Finally, set that pixel format that was suggested as the one being used
    SetPixelFormat(win32_DeviceContextHandle, win32_suggestedPixelFormatIndex, &win32_chosenPixelFormat);

    HGLRC win32_GLRenderingContextHandle = wglCreateContext(win32_DeviceContextHandle);
    if (wglMakeCurrent(win32_DeviceContextHandle, win32_GLRenderingContextHandle)) {
        // Good
    } else {
        printf("Error on wglMakeCurrent");
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
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

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* win32_cmdLine, int nCmdShow) {
    const char win32_windowClassName[]  = "Window Class Text";
    WNDCLASSA win32_windowClass = (WNDCLASSA) {0};
    win32_windowClass.lpfnWndProc = WindowProc;
    win32_windowClass.hInstance = hInstance;
    win32_windowClass.lpszClassName = win32_windowClassName;
    RegisterClassA(&win32_windowClass);

    // Create the window
    HWND win32_windowHandle = CreateWindowExA(0, win32_windowClassName, "Window Text", 0,
        100, 100, 300, 200,
        NULL, NULL, hInstance, NULL
    );

    HDC win32_DeviceContextHandle = GetDC(win32_windowHandle);
    opengl_initialize(win32_DeviceContextHandle);

    {
        // Some gl code
        glViewport(0,0, 300, 200);
        glClearColor(1.0f, 0.5f, 0.0f, 0.5f);
        glClear(GL_COLOR_BUFFER_BIT);
        SwapBuffers(win32_DeviceContextHandle);
    }
    ShowWindow(win32_windowHandle, nCmdShow);

    MSG win32_msg = (MSG) {0};
    while (GetMessageA(&win32_msg, NULL, 0, 0))
    {
        TranslateMessage(&win32_msg);
        DispatchMessageA(&win32_msg);
        
        {
            // Rendering stuff
            glClear(GL_COLOR_BUFFER_BIT);

            SwapBuffers(win32_DeviceContextHandle);
        }
    }
    return 0;
}