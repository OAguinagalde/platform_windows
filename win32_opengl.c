#include "win32_opengl.h"

// Loops through and print all the errors related to OpenGL
bool win32_gl_get_errors(char* label) {
    UNREFERENCED_PARAMETER(label);
    bool noErrors = true;
    GLenum opengl_error = 0;
    while ((opengl_error = glGetError()) != GL_NO_ERROR) {
        noErrors = false;
        win32_printf("Error gl: ");
        switch (opengl_error) {
            case GL_NO_ERROR:          { win32_printf("GL_NO_ERROR\n"); } break;
            case GL_INVALID_ENUM:      { win32_printf("GL_INVALID_ENUM\n"); } break;
            case GL_INVALID_VALUE:     { win32_printf("GL_INVALID_VALUE\n"); } break;
            case GL_INVALID_OPERATION: { win32_printf("GL_INVALID_OPERATION\n"); } break;
            case GL_STACK_OVERFLOW:    { win32_printf("GL_STACK_OVERFLOW\n"); } break;
            case GL_STACK_UNDERFLOW:   { win32_printf("GL_STACK_UNDERFLOW\n"); } break;
            case GL_OUT_OF_MEMORY:     { win32_printf("GL_OUT_OF_MEMORY\n"); } break;
            default:                   { win32_printf("Unknown Error\n"); } break;
        }
    }
    if (noErrors) {
        win32_printf("No errors\n");
    }
    return !noErrors;
}

// Returns the pointer to the function given or returns NULL on failure
void* win32_gl_get_function_address(const char* functionName) {
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
void win32_gl_initialize_context(HDC DeviceContextHandle) {    
    // First we need to get a pixel format that we want OpenGL to use
    // But we don't know what we can use, so we first describe our ideal pixel format
    PIXELFORMATDESCRIPTOR desiredPixelFormat;
    memset(&desiredPixelFormat, 0, sizeof(desiredPixelFormat));
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
        win32_printf("Error at %s: wglMakeCurrent, GetLastError() -> %d\n", __FUNCTIONW__, e);
    }
    else {
        win32_printf("Pixel Format selected and wglContext created\n");
    }
}

void win32_gl_initialize_extensions() {
    #define InitializeExtension(type, name) name = (type) win32_gl_get_function_address(#name); assert(name && #name);
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

void win32_gl_swap_pixel_buffer(HDC deviceContextHandle) {
    // The SwapBuffers function exchanges the front and back buffers if the
    // current pixel format for the window referenced by the specified device context includes a back buffer.
    SwapBuffers(deviceContextHandle);
}

GLuint win32_gl_load_shader(const char* shaderSource, int sourceSize, ShaderType type) {
    GLuint shaderObject = 0;
    if (type == FragmentShader) {
        shaderObject = glCreateShader(GL_FRAGMENT_SHADER);
    }
    else if (type == VertexShader) {
        shaderObject = glCreateShader(GL_VERTEX_SHADER);
    }
    glShaderSource(shaderObject, 1, &shaderSource, (const GLint *)&sourceSize);
    glCompileShader(shaderObject);
    int success;
    glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &success);
    if(success == 0) {
        char info[512];
        glGetShaderInfoLog(shaderObject, 512, NULL, info);
        win32_printf("Error compiling fragment shader:\n\t%s", info);
    }

    win32_gl_get_errors(__FUNCTION__);
    return shaderObject;
}

GLuint win32_gl_generate_shader_program_and_clean(GLuint vertexShaderObject, GLuint fragmentShaderObject) {
    GLuint shaderProgramObject = glCreateProgram();
    glAttachShader(shaderProgramObject, vertexShaderObject);
    glAttachShader(shaderProgramObject, fragmentShaderObject);
    glLinkProgram(shaderProgramObject);
    int success;
    glGetProgramiv(shaderProgramObject, GL_LINK_STATUS, &success);
    if(success == 0) {
        char info[512];
        glGetProgramInfoLog(shaderProgramObject, 512, NULL, info);
        win32_printf("Error linking shader program:\n\t%s", info);
    }
    glDeleteShader(vertexShaderObject);
    glDeleteShader(fragmentShaderObject);
    win32_gl_get_errors(__FUNCTION__);
    return shaderProgramObject;
}

GLuint win32_gl_load_texture_rgba_u32(void* data, GLsizei w, GLsizei h, int textureSlot) {
    GLuint textureObject;
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &textureObject);
    float textureDimensions[2];
    textureDimensions[0] = (float) w;
    textureDimensions[1] = (float) h;
    glActiveTexture(GL_TEXTURE0 + textureSlot);
    glBindTexture(GL_TEXTURE_2D, textureObject);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    // TODO: deactivate texture unit?
    win32_gl_get_errors(__FUNCTION__);
    return textureObject;
}

GLuint win32_gl_create_texture(int textureSlot) {
    GLuint textureObject;
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0 + textureSlot);
    glGenTextures(1, &textureObject);
    glBindTexture(GL_TEXTURE_2D, textureObject);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    // TODO: deactivate texture unit?
    win32_gl_get_errors(__FUNCTION__);
    return textureObject;
}

void win32_gl_enable_blending() {
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
}

GLuint win32_gl_start_vertex_array_object_configuration() {
    GLuint vertexArrayObject;
    glGenVertexArrays(1, &vertexArrayObject);
    glBindVertexArray(vertexArrayObject);
    return vertexArrayObject;
}
void win32_gl_finish_vertex_array_object_configuration() {
    // Finish, unbind everything
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLuint win32_gl_generate_and_initialize_element_buffer_012023(unsigned long long maxQuads) {
    GLuint elementBufferObject;
    glGenBuffers(1, &elementBufferObject);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferObject);
    // It's an static buffer so we can just load it now on initialization and forget about it
    unsigned long long maxIndices = maxQuads * 6;
    unsigned long *indices = (unsigned long *)calloc(1, maxIndices * sizeof(unsigned long));
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
    free(indices);
    return elementBufferObject;
}

GLuint win32_gl_generate_and_zero_initialize_vertex_buffer(unsigned long long maxQuads, unsigned long long sizeOfVertex) {
    GLuint vertexBufferObject;
    glGenBuffers(1, &vertexBufferObject);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    unsigned long long maxVertices = maxQuads * 4;
    unsigned long long requiredData = sizeOfVertex * maxVertices;
    // pretty sure there is better ways of doing this lol
    void *emptyData = (void*)calloc(1, requiredData);
    glBufferData(GL_ARRAY_BUFFER, requiredData, emptyData, GL_DYNAMIC_DRAW);
    free(emptyData);
    return vertexBufferObject;
}

void win32_gl_initialize_renderer(unsigned long long maxQuads) {
    
    // Something about windows and framerates, dont remember, probably related to vertical sync
    wglSwapIntervalEXT(1);

    GLuint textureObject = win32_gl_create_texture(0);
    DBG_UNREFERENCED_PARAMETER(textureObject);
    win32_gl_enable_blending();

    GLuint vertexArrayObject = win32_gl_start_vertex_array_object_configuration();
    GLuint elementBufferObject = win32_gl_generate_and_initialize_element_buffer_012023(maxQuads);
    GLuint vertexBufferObject = win32_gl_generate_and_zero_initialize_vertex_buffer(maxQuads, sizeof(float) * 8);
    DBG_UNREFERENCED_PARAMETER(vertexArrayObject);
    DBG_UNREFERENCED_PARAMETER(elementBufferObject);
    DBG_UNREFERENCED_PARAMETER(vertexBufferObject);
    {
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(0));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
    }
    win32_gl_finish_vertex_array_object_configuration();
}

void win32_gl_render(unsigned long clientWidth, unsigned long clientHeight, HDC deviceContextHandle, int textureSlot, GLuint vertexArrayObject, GLuint shaderProgramObject, GLuint vertexBufferObject, GLuint textureObject, int vertexSize, unsigned long quadsToRender, float* vertexBuffer, int textureHeight, int textureWidth) {
    glViewport(0, 0, clientWidth, clientHeight);
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    #define TO_COLUMN_MAJOR(v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15) v0,v4,v8,v12,v1,v5,v9,v13,v2,v6,v10,v14,v3,v7,v11,v15
    GLfloat w = 2.0f/(float)clientWidth;
    GLfloat h = 2.0f/(float)clientHeight;
    GLfloat projectionMatrix[] = { TO_COLUMN_MAJOR(
        w,  0,  0, -1,
        0, -h,  0,  1,
        0,  0,  1,  0,
        0,  0,  0,  1
    )};
    #undef TO_COLUMN_MAJOR

    glBindVertexArray(vertexArrayObject);
    glActiveTexture(GL_TEXTURE0 + textureSlot);
    glBindTexture(GL_TEXTURE_2D, textureObject);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, vertexSize * 4 * quadsToRender, vertexBuffer, GL_DYNAMIC_DRAW);
    glUseProgram(shaderProgramObject);
    GLint mvpUniformPosition = glGetUniformLocation(shaderProgramObject, "mvp");
    GLint textureDimensionsUniformPosition = glGetUniformLocation(shaderProgramObject, "texture_dimensions");
    glUniformMatrix4fv(mvpUniformPosition, 1, GL_FALSE, projectionMatrix);
    float textureDimensions[2];
    textureDimensions[0] = (float) textureWidth;
    textureDimensions[1] = (float) textureHeight;
    glUniform2fv(textureDimensionsUniformPosition, 1, textureDimensions);
    glDrawElements(GL_TRIANGLES, quadsToRender * 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    //glBindTexture(GL_TEXTURE_2D, 0);
    //glActiveTexture(GL_TEXTURE0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    win32_gl_swap_pixel_buffer(deviceContextHandle);
}
