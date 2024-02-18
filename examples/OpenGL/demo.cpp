#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define GLFW_INCLUDE_NONE
#include <iostream>

#include <GL/gl3w.h>
#include "GLFW/glfw3.h"


#ifdef _WIN32

#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "glfw/lib-vc2022/glfw3.lib")

extern "C"
{
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}

extern "C"
{
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

#define countof(x) (sizeof(x) / sizeof(0[x]))

#define M_PI 3.141592653589793
#define ATTRIB_POINT 0

static GLuint
compile_shader(GLenum type, const GLchar *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint param;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &param);
    if (!param) {
        GLchar log[4096];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        fprintf(stderr, "error: %s: %s\n",
                type == GL_FRAGMENT_SHADER ? "frag" : "vert", (char *) log);
        exit(EXIT_FAILURE);
    }
    return shader;
}

static GLuint
link_program(GLuint vert, GLuint frag)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    GLint param;
    glGetProgramiv(program, GL_LINK_STATUS, &param);
    if (!param) {
        GLchar log[4096];
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        fprintf(stderr, "error: link: %s\n", (char *) log);
        exit(EXIT_FAILURE);
    }
    return program;
}

struct graphics_context {
    GLFWwindow *window;
    GLuint program;
    GLint uniform_angle;
    GLuint vbo_point;
    GLuint vao_point;
    double angle;
    long framecount;
    double lastframe;
};

const float SQUARE[] = {
    -1.0f,  1.0f,
    -1.0f, -1.0f,
     1.0f,  1.0f,
     1.0f, -1.0f
};

static void
render(struct graphics_context *context)
{
    glClearColor(0.15, 0.15, 0.15, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(context->program);
    glUniform1f(context->uniform_angle, context->angle);
    glBindVertexArray(context->vao_point);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, countof(SQUARE) / 2);
    glBindVertexArray(0);
    glUseProgram(0);

    /* Physics */
    double now = glfwGetTime();
    double udiff = now - context->lastframe;
    context->angle += 1.0 * udiff;
    if (context->angle > 2 * M_PI)
        context->angle -= 2 * M_PI;
    context->framecount++;
    if ((long)now != (long)context->lastframe) {
        printf("FPS: %ld\n", context->framecount);
        context->framecount = 0;
    }
    context->lastframe = now;

    glfwSwapBuffers(context->window);
}

static void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    (void) scancode;
    (void) mods;
    if (key == GLFW_KEY_Q && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void error_callback(int error, const char *description)
{
	fprintf(stderr, "Error: %s\n", description);
}


int main(int argc, char **argv)
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    /* Options */
    bool fullscreen = false;
    const char *title = "OpenGL 3.3 Demo";

    /*
    int opt;
    while ((opt = getopt(argc, argv, "f")) != -1) {
        switch (opt) {
            case 'f':
                fullscreen = true;
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }
	*/

    /* Create window and OpenGL context */
    struct graphics_context context{};
    if (!glfwInit()) {
        fprintf(stderr, "GLFW3: failed to initialize\n");
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwSetErrorCallback(error_callback);

    if (fullscreen) {
        GLFWmonitor *monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *m = glfwGetVideoMode(monitor);
        context.window =
            glfwCreateWindow(m->width, m->height, title, monitor, NULL);
    } else {
        context.window =
            glfwCreateWindow(640, 640, title, NULL, NULL);
    }

    glfwMakeContextCurrent(context.window);
    glfwSwapInterval(1);

    /* Initialize gl3w */
    if (gl3wInit()) {
        fprintf(stderr, "gl3w: failed to initialize\n");
        exit(EXIT_FAILURE);
    }

    /* Shader sources */
    const GLchar *vert_shader =
        "#version 330\n"
        "layout(location = 0) in vec2 point;\n"
        "uniform float angle;\n"
        "void main() {\n"
        "    mat2 rotate = mat2(cos(angle), -sin(angle),\n"
        "                       sin(angle), cos(angle));\n"
        "    gl_Position = vec4(0.75 * rotate * point, 0.0, 1.0);\n"
        "}\n";
    const GLchar *frag_shader =
        "#version 330\n"
        "out vec4 color;\n"
        "void main() {\n"
        "    color = vec4(1, 0.15, 0.15, 0);\n"
        "}\n";

    /* Compile and link OpenGL program */
    GLuint vert = compile_shader(GL_VERTEX_SHADER, vert_shader);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, frag_shader);
    context.program = link_program(vert, frag);
    context.uniform_angle = glGetUniformLocation(context.program, "angle");
    glDeleteShader(frag);
    glDeleteShader(vert);

    /* Prepare vertex buffer object (VBO) */
    glGenBuffers(1, &context.vbo_point);
    glBindBuffer(GL_ARRAY_BUFFER, context.vbo_point);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SQUARE), SQUARE, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /* Prepare vertrex array object (VAO) */
    glGenVertexArrays(1, &context.vao_point);
    glBindVertexArray(context.vao_point);
    glBindBuffer(GL_ARRAY_BUFFER, context.vbo_point);
    glVertexAttribPointer(ATTRIB_POINT, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(ATTRIB_POINT);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    /* Start main loop */
    glfwSetKeyCallback(context.window, key_callback);
    context.lastframe = glfwGetTime();
    context.framecount = 0;
    while (!glfwWindowShouldClose(context.window)) {
        render(&context);
        glfwPollEvents();
    }
    fprintf(stderr, "Exiting ...\n");

    /* Cleanup and exit */
    glDeleteVertexArrays(1, &context.vao_point);
    glDeleteBuffers(1, &context.vbo_point);
    glDeleteProgram(context.program);

    glfwTerminate();
    return 0;
}
