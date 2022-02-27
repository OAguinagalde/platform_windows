#pragma once
#include "win32.h"

#pragma comment(lib, "gdi32")
#pragma comment(lib, "Opengl32")

#include <gl\GL.h>

// https://www.khronos.org/registry/OpenGL/api/GL/wglext.h
#include "wglext.h"

// https://www.khronos.org/registry/OpenGL/api/GL/glext.h
//  depends on KHR/khrplatform.h
//  https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h
#include "glext.h"

#include <assert.h>
#include <stdlib.h>

#define DECLARE_EXTENSION(type, name) type name = NULL;
DECLARE_EXTENSION(PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT);
DECLARE_EXTENSION(PFNGLGENBUFFERSPROC, glGenBuffers);
DECLARE_EXTENSION(PFNGLBINDBUFFERPROC, glBindBuffer);
DECLARE_EXTENSION(PFNGLBUFFERDATAPROC, glBufferData);
DECLARE_EXTENSION(PFNGLCREATESHADERPROC, glCreateShader);
DECLARE_EXTENSION(PFNGLSHADERSOURCEPROC, glShaderSource);
DECLARE_EXTENSION(PFNGLCOMPILESHADERPROC, glCompileShader);
DECLARE_EXTENSION(PFNGLGETSHADERIVPROC, glGetShaderiv);
DECLARE_EXTENSION(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
DECLARE_EXTENSION(PFNGLBLENDFUNCSEPARATEPROC, glBlendFuncSeparate);
DECLARE_EXTENSION(PFNGLCREATEPROGRAMPROC, glCreateProgram);
DECLARE_EXTENSION(PFNGLATTACHSHADERPROC, glAttachShader);
DECLARE_EXTENSION(PFNGLLINKPROGRAMPROC, glLinkProgram);
DECLARE_EXTENSION(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
DECLARE_EXTENSION(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
DECLARE_EXTENSION(PFNGLDELETESHADERPROC, glDeleteShader);
DECLARE_EXTENSION(PFNGLUSEPROGRAMPROC, glUseProgram);
DECLARE_EXTENSION(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
DECLARE_EXTENSION(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
DECLARE_EXTENSION(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
DECLARE_EXTENSION(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
// Selects which texture unit subsequent texture state calls will affect. The number of texture units an implementation supports is implementation dependent, but must be at least 80.
// parameter texture - Specifies which texture unit to make active. The number of texture units is implementation dependent, but must be at least 80. texture must be one of GL_TEXTUREi, where i ranges from zero to the value of GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS minus one. The initial value is GL_TEXTURE0.
DECLARE_EXTENSION(PFNGLACTIVETEXTUREPROC, glActiveTexture);
DECLARE_EXTENSION(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);
DECLARE_EXTENSION(PFNGLUNIFORM2FVPROC, glUniform2fv);
DECLARE_EXTENSION(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
#undef DECLARE_EXTENSION

typedef struct Texture {
    int textureSlot;
    GLuint textureObject;
    bool dataLoaded;
    int width;
    int height;
} Texture;

typedef enum ShaderType {
    FragmentShader,
    VertexShader
} ShaderType;

// Loops through and print all the errors related to OpenGL
bool win32_gl_get_errors(char* label);

// Returns the pointer to the function given or returns NULL on failure
void* win32_gl_get_function_address(const char* functionName);

// Given a devide context handle, initializes a ready to use OpenGl context with a (hopefully) RGBA32 pixel format
void win32_gl_initialize_context(HDC DeviceContextHandle);

void win32_gl_initialize_extensions();

void win32_gl_swap_pixel_buffer(HDC deviceContextHandle);

GLuint win32_gl_load_shader(const char* shaderSource, int sourceSize, ShaderType type);

GLuint win32_gl_generate_shader_program_and_clean(GLuint vertexShaderObject, GLuint fragmentShaderObject);

GLuint win32_gl_load_texture_rgba_u32(void* data, GLsizei w, GLsizei h, int textureSlot);

GLuint win32_gl_create_texture(int textureSlot);

void win32_gl_enable_blending();

GLuint win32_gl_start_vertex_array_object_configuration();

void win32_gl_finish_vertex_array_object_configuration();

GLuint win32_gl_generate_and_initialize_element_buffer_012023(unsigned long long maxQuads);

GLuint win32_gl_generate_and_zero_initialize_vertex_buffer(unsigned long long maxQuads, unsigned long long sizeOfVertex);

void win32_gl_initialize_renderer(unsigned long long maxQuads);

void win32_gl_render(unsigned long clientWidth, unsigned long clientHeight, HDC deviceContextHandle, int textureSlot, GLuint vertexArrayObject, GLuint shaderProgramObject, GLuint vertexBufferObject, GLuint textureObject, int vertexSize, unsigned long quadsToRender, float* vertexBuffer, int textureHeight, int textureWidth);
