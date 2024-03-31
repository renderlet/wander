#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <iostream>

#define GLFW_INCLUDE_NONE
#include <GL/gl3w.h>
#include "GLFW/glfw3.h"

#include "glm/glm.hpp"
#include "glm/ext.hpp"

#include "wander.h"

#include "bmpread.h"

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

#ifdef _WIN64
// TODO - move to wander lib
#pragma comment(lib, "wasmtime.dll.lib")
#endif

#endif


#define countof(x) (sizeof(x) / sizeof(0[x]))

#ifndef M_PI
#define M_PI 3.141592653589793
#endif

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
	GLuint program_wasm;
	GLint uniform_transform;
	GLint uniform_projection;
    GLuint vbo_point;
    GLuint vao_point;
    GLuint texture_window;
    GLuint texture_roof;
    GLuint texture_white;
    double angle;
    long framecount;
    double lastframe;
	wander::IRuntime* runtime;
	const wander::RenderTree* tree;
};

const float SQUARE[] = {
    -1.0f,  1.0f,
    -1.0f, -1.0f,
     1.0f,  1.0f,
     1.0f, -1.0f
};

struct float3
{
	float x, y, z;
};
struct matrix
{
	float m[4][4];
};

matrix operator*(const matrix &m1, const matrix &m2);

float w = 1.0f; // width (aspect ratio)
float h = 1.0f; // height
float n = 1.0f; // near
float f = 90.0f; // far

float3 modelRotation = {0.0f, 0.0f, 0.0f};
float3 modelScale = {0.8f, 0.8f, 0.8f};
float3 modelTranslation = {-3.0f, -8.0f, -15.0f};

static void
render(struct graphics_context *context)
{
    glClearColor(0.15, 0.15, 0.15, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(context->program);
    glUniform1f(context->uniform_angle, context->angle);
    glBindVertexArray(context->vao_point);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, countof(SQUARE) / 2);
    glBindVertexArray(0);
    glUseProgram(0);

	glUseProgram(context->program_wasm);

    matrix rotateX   = { 1, 0, 0, 0, 0, static_cast<float>(cos(modelRotation.x)), -static_cast<float>(sin(modelRotation.x)), 0, 0, static_cast<float>(sin(modelRotation.x)), static_cast<float>(cos(modelRotation.x)), 0, 0, 0, 0, 1 };
    matrix rotateY   = { static_cast<float>(cos(modelRotation.y)), 0, static_cast<float>(sin(modelRotation.y)), 0, 0, 1, 0, 0, -static_cast<float>(sin(modelRotation.y)), 0, static_cast<float>(cos(modelRotation.y)), 0, 0, 0, 0, 1 };
    matrix rotateZ   = { static_cast<float>(cos(modelRotation.z)), -static_cast<float>(sin(modelRotation.z)), 0, 0, static_cast<float>(sin(modelRotation.z)), static_cast<float>(cos(modelRotation.z)), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    matrix scale     = { modelScale.x, 0, 0, 0, 0, modelScale.y, 0, 0, 0, 0, modelScale.z, 0, 0, 0, 0, 1 };
    matrix translate = {
    	1, 0, 0, 0,
    	0, 1, 0, 0,
    	0, 0, 1, 0,
    	modelTranslation.x, modelTranslation.y, modelTranslation.z, 1 };

    modelRotation.x += 0.005f;
    modelRotation.y += 0.009f;
    modelRotation.z += 0.01f;

	//matrix Transform = rotateX * rotateY * rotateZ * scale * translate;
	matrix Transform = rotateY * scale * translate;

    glm::mat4 Projection = glm::perspective(90.0f, 3.0f / 3.0f, 0.1f, 1000.f);

    glUniformMatrix4fv(context->uniform_transform, 1, GL_FALSE, (GLfloat *)Transform.m);
	glUniformMatrix4fv(context->uniform_projection, 1, GL_FALSE, glm::value_ptr(Projection)); 

    

    glDisable(GL_CULL_FACE);

    

    for (auto i = 0; i < context->tree->Length(); ++i)
	{
        auto node = context->tree->NodeAt(i);
        if (node->Metadata().find("roof") != std::string::npos)
        {
            //printf("texture %d", context->texture_roof);
            //glBindTexture(GL_TEXTURE_2D, context->texture_roof);
            glActiveTexture(GL_TEXTURE0); // activate the texture unit first before binding texture
            glBindTexture(GL_TEXTURE_2D, context->texture_roof);
        }
        else if (node->Metadata().find("window") != std::string::npos)
        {
            //printf("texture %d", context->texture_window);
            glActiveTexture(GL_TEXTURE0); // activate the texture unit first before binding texture
            glBindTexture(GL_TEXTURE_2D, context->texture_window);
        }
        else
        {
            glActiveTexture(GL_TEXTURE0); // activate the texture unit first before binding texture
            glBindTexture(GL_TEXTURE_2D, context->texture_white);
        }
        
        //glBindTexture(GL_TEXTURE_2D, context->texture_window);

        //glUniform1i(glGetUniformLocation(context->program_wasm, "ourTexture"), 0); // set it manually

		node->RenderFixedStride(context->runtime, sizeof(GLfloat) * 11);
        //abort();
	}

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
{
    /* Options */
    bool fullscreen = false;
    const char *title = "Wander OpenGL 3.3 Demo";

    /* Create window and OpenGL context */
    struct graphics_context context{};
    if (!glfwInit()) {
        fprintf(stderr, "GLFW3: failed to initialize\n");
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwSetErrorCallback(error_callback);

	context.window = glfwCreateWindow(640, 640, title, NULL, NULL);

    glfwMakeContextCurrent(context.window);
    glfwSwapInterval(1);

    /* Initialize gl3w */
    if (gl3wInit()) {
        fprintf(stderr, "gl3w: failed to initialize\n");
        exit(EXIT_FAILURE);
    }

    /* Shader sources */
    const GLchar *vert_shader =
        "#version 410\n"
        "layout(location = 0) in vec2 point;\n"
        "uniform float angle;\n"
        "void main() {\n"
        "    mat2 rotate = mat2(cos(angle), -sin(angle),\n"
        "                       sin(angle), cos(angle));\n"
        "    gl_Position = vec4(0.75 * rotate * point, 0.99, 1.0);\n"
        "}\n";
    const GLchar *frag_shader =
        "#version 410\n"
        "out vec4 color;\n"
        "void main() {\n"
        "    color = vec4(1, 0.15, 0.15, 0);\n"
        "}\n";

    const GLchar *vert_shader_wasm = 
        "#version 410\n"
	    "#extension GL_ARB_separate_shader_objects : require \n"
		"layout(location = 0) in vec3 vs_position;\n"
		"layout(location = 1) in vec3 vs_normal;\n"
		"layout(location = 2) in vec2 vs_texcoord;\n"
		"layout(location = 3) in vec3 vs_color;\n"
		"layout(location = 0) out vec2 ps_texcoord;\n"
		"layout(location = 1) out vec4 ps_color;\n"
		"uniform mat4 transform;\n"
		"uniform mat4 projection;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = projection * transform * vec4(vs_position, 1.0);\n"
		"	ps_texcoord = vs_texcoord;\n"
		"	ps_color = vec4(vs_color, 1.0);\n"
		"}\n";

    const GLchar *frag_shader_wasm = 
        "#version 410\n"
		"#extension GL_ARB_separate_shader_objects : require \n"
        "uniform sampler2D ourTexture;\n"
		"out vec4 out_color;\n"
		"layout(location = 0) in vec2 ps_texcoord;\n"
		"layout(location = 1) in vec4 ps_color;\n"
		"void main()\n"
		"{\n"
        "    out_color = texture(ourTexture, ps_texcoord) * ps_color;\n"
		"    //out_color = ps_color;\n"
		"}\n";

    /* Compile and link OpenGL program */
    GLuint vert = compile_shader(GL_VERTEX_SHADER, vert_shader);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, frag_shader);
    context.program = link_program(vert, frag);
    context.uniform_angle = glGetUniformLocation(context.program, "angle");
    glDeleteShader(frag);
    glDeleteShader(vert);

    GLuint vert_wasm = compile_shader(GL_VERTEX_SHADER, vert_shader_wasm);
	GLuint frag_wasm = compile_shader(GL_FRAGMENT_SHADER, frag_shader_wasm);
	context.program_wasm = link_program(vert_wasm, frag_wasm);
	context.uniform_transform = glGetUniformLocation(context.program_wasm, "transform");
	context.uniform_projection = glGetUniformLocation(context.program_wasm, "projection");
	glDeleteShader(frag_wasm);
	glDeleteShader(vert_wasm);

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

    ///////////////////////////////////////////////////////////////////////////////////////////////


    unsigned int TextureDataWhite[] = // 1x1 pixel white
    {
        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
    };

    glGenTextures(1, &context.texture_white);
    glBindTexture(GL_TEXTURE_2D, context.texture_white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, TextureDataWhite);

    

    bmpread_t bitmap;

    bmpread("window.bmp", BMPREAD_ANY_SIZE, &bitmap);

    glGenTextures(1, &context.texture_window);
    glBindTexture(GL_TEXTURE_2D, context.texture_window);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bitmap.width, bitmap.height, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmap.data);

    //glTexImage2D(GL_TEXTURE_2D, 0, 4, bitmap.width, bitmap.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap.data);
    //printf("%d, %d \n", bitmap.width, bitmap.height);
    //bmpread_free(&bitmap);

    bmpread_t bitmap2;

    bmpread("roof2.bmp", BMPREAD_ANY_SIZE , &bitmap2);

    glGenTextures(1, &context.texture_roof);
    glBindTexture(GL_TEXTURE_2D, context.texture_roof);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bitmap2.width, bitmap2.height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, bitmap2.data);

    //bmpread_free(&bitmap2);

	const auto pal = wander::Factory::CreatePal(wander::EPalType::OpenGL, (void*)context.window);
	context.runtime = wander::Factory::CreateRuntime(pal);

	auto renderlet_id = context.runtime->LoadFromFile(L"../Building.rlt", "start");

	auto tree_id = context.runtime->Render(renderlet_id);
	context.tree = context.runtime->GetRenderTree(tree_id);

    glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 11, 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 11, (void *)(sizeof(GLfloat) * 3));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 11, (void *)(sizeof(GLfloat) * 6));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 11, (void *)(sizeof(GLfloat) * 8));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);

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

    glDeleteProgram(context.program_wasm);
	context.runtime->Release();

    glfwTerminate();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

matrix operator*(const matrix &m1, const matrix &m2)
{
	return {
		m1.m[0][0] * m2.m[0][0] + m1.m[0][1] * m2.m[1][0] + m1.m[0][2] * m2.m[2][0] + m1.m[0][3] * m2.m[3][0],
		m1.m[0][0] * m2.m[0][1] + m1.m[0][1] * m2.m[1][1] + m1.m[0][2] * m2.m[2][1] + m1.m[0][3] * m2.m[3][1],
		m1.m[0][0] * m2.m[0][2] + m1.m[0][1] * m2.m[1][2] + m1.m[0][2] * m2.m[2][2] + m1.m[0][3] * m2.m[3][2],
		m1.m[0][0] * m2.m[0][3] + m1.m[0][1] * m2.m[1][3] + m1.m[0][2] * m2.m[2][3] + m1.m[0][3] * m2.m[3][3],
		m1.m[1][0] * m2.m[0][0] + m1.m[1][1] * m2.m[1][0] + m1.m[1][2] * m2.m[2][0] + m1.m[1][3] * m2.m[3][0],
		m1.m[1][0] * m2.m[0][1] + m1.m[1][1] * m2.m[1][1] + m1.m[1][2] * m2.m[2][1] + m1.m[1][3] * m2.m[3][1],
		m1.m[1][0] * m2.m[0][2] + m1.m[1][1] * m2.m[1][2] + m1.m[1][2] * m2.m[2][2] + m1.m[1][3] * m2.m[3][2],
		m1.m[1][0] * m2.m[0][3] + m1.m[1][1] * m2.m[1][3] + m1.m[1][2] * m2.m[2][3] + m1.m[1][3] * m2.m[3][3],
		m1.m[2][0] * m2.m[0][0] + m1.m[2][1] * m2.m[1][0] + m1.m[2][2] * m2.m[2][0] + m1.m[2][3] * m2.m[3][0],
		m1.m[2][0] * m2.m[0][1] + m1.m[2][1] * m2.m[1][1] + m1.m[2][2] * m2.m[2][1] + m1.m[2][3] * m2.m[3][1],
		m1.m[2][0] * m2.m[0][2] + m1.m[2][1] * m2.m[1][2] + m1.m[2][2] * m2.m[2][2] + m1.m[2][3] * m2.m[3][2],
		m1.m[2][0] * m2.m[0][3] + m1.m[2][1] * m2.m[1][3] + m1.m[2][2] * m2.m[2][3] + m1.m[2][3] * m2.m[3][3],
		m1.m[3][0] * m2.m[0][0] + m1.m[3][1] * m2.m[1][0] + m1.m[3][2] * m2.m[2][0] + m1.m[3][3] * m2.m[3][0],
		m1.m[3][0] * m2.m[0][1] + m1.m[3][1] * m2.m[1][1] + m1.m[3][2] * m2.m[2][1] + m1.m[3][3] * m2.m[3][1],
		m1.m[3][0] * m2.m[0][2] + m1.m[3][1] * m2.m[1][2] + m1.m[3][2] * m2.m[2][2] + m1.m[3][3] * m2.m[3][2],
		m1.m[3][0] * m2.m[0][3] + m1.m[3][1] * m2.m[1][3] + m1.m[3][2] * m2.m[2][3] + m1.m[3][3] * m2.m[3][3],
	};
}