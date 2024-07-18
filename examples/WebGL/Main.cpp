#include <GLFW/glfw3.h>
#include <iostream>

// Include the EMSCRIPTEN specific headers
#include <emscripten.h>
#include <emscripten/fetch.h>
#include <GLES3/gl3.h>
#include <GLES3/gl2ext.h>
#include <GLES3/gl3platform.h> 
#include <OpenGL/glm/glm.hpp>
#include <OpenGL/glm/ext.hpp>

#include "../OpenGL/bmpread.h"

#include "wander.h"

// For example purpose we use global variables
#define SPEED 0.005f
GLFWwindow* window = nullptr;
float red = 0.0f;
bool increase = true;

const wander::RenderTree* tree;
wander::IRuntime* runtime;

GLuint program_wasm;
GLint uniform_transform;
GLint uniform_projection;


void downloadSucceeded(emscripten_fetch_t *fetch)
{
	printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);
	// The data is now available at fetch->data[0] through fetch->data[fetch->numBytes-1];

	FILE *f1 = fopen("/window.bmp", "wb");
	if (!f1)
	{
		printf("Open write failed\n");
	}
	else
	{
		printf("Open write success\n");
	}
	size_t r1 = fwrite(fetch->data, 1, fetch->numBytes, f1);
	fclose(f1);

	emscripten_fetch_close(fetch); // Free data associated with the fetch.

	auto* f2 = fopen("/window.bmp", "rb");

	if (!f2)
	{
		printf("Open file failed\n");
	}
	
	fclose(f2);

	bmpread_t bitmap;
	bmpread("/window.bmp", BMPREAD_ANY_SIZE, &bitmap);
	printf("Height %d %d %d\n", bitmap.height, 0, 1);
	//printf("Height %d %d %d\n", bitmap.height, b[0], b[1]);
}

void downloadFailed(emscripten_fetch_t *fetch)
{
	printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);
	emscripten_fetch_close(fetch); // Also free data on failure.
}


static GLuint compile_shader(GLenum type, const GLchar *source)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	GLint param;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &param);
	if (!param)
	{
		GLchar log[4096];
		glGetShaderInfoLog(shader, sizeof(log), NULL, log);
		fprintf(stderr, "error: %s: %s\n", type == GL_FRAGMENT_SHADER ? "frag" : "vert", (char *)log);
		exit(EXIT_FAILURE);
	}
	return shader;
}

static GLuint link_program(GLuint vert, GLuint frag)
{
	GLuint program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);
	GLint param;
	glGetProgramiv(program, GL_LINK_STATUS, &param);
	if (!param)
	{
		GLchar log[4096];
		glGetProgramInfoLog(program, sizeof(log), NULL, log);
		fprintf(stderr, "error: link: %s\n", (char *)log);
		exit(EXIT_FAILURE);
	}
	return program;
}

struct float3
{
	float x, y, z;
};
struct matrix
{
	float m[4][4];
};

matrix operator*(const matrix &m1, const matrix &m2);

float3 modelRotation = {0.0f, 0.0f, 0.0f};
float3 modelTranslation = {-3.0f, -8.0f, -15.0f};

/**
 * @brief This function just increases and decreases the red value.
 *
 * @return The red value.
 */
float getRed()
{
    if (increase)
    {
        red += SPEED;
        if (red > 1.0f)
        {
            red = 1.0f;
            increase = false;
        }
    }
    else
    {
        red -= SPEED;
        if (red < 0.0f)
        {
            red = 0.0f;
            increase = true;
        }
    }

    return red;
}

/**
 * @brief This function is called every frame.
 */
void mainLoop()
{
    // Clear the screen with a color
    glClearColor(getRed(), 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(program_wasm);

    matrix rotateY   = { static_cast<float>(cos(modelRotation.y)), 0, static_cast<float>(sin(modelRotation.y)), 0, 0, 1, 0, 0, -static_cast<float>(sin(modelRotation.y)), 0, static_cast<float>(cos(modelRotation.y)), 0, 0, 0, 0, 1 };
    matrix scale     = { 1.0f, 0, 0, 0, 0, 1.0f, 0, 0, 0, 0, 1.0f, 0, 0, 0, 0, 1.0f };
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

    glUniformMatrix4fv(uniform_transform, 1, GL_FALSE, (GLfloat *)Transform.m);
	glUniformMatrix4fv(uniform_projection, 1, GL_FALSE, glm::value_ptr(Projection)); 

    glDisable(GL_CULL_FACE);

	for (auto i = 0; i < tree->Length(); ++i)
	{
		auto node = tree->NodeAt(i);

        node->RenderFixedStride(runtime, sizeof(GLfloat) * 11);
	}

    // Swap the buffers of the window
    glfwSwapBuffers(window);

    // Poll for the events
    glfwPollEvents();
}


/**
 * @brief The normal main() function.
 *
 * @return Exit code.
 */
int main()
{
    // Initialize glfw
    if (!glfwInit())
        exit(EXIT_FAILURE);

    // Create the window
    window = glfwCreateWindow(640, 480, "Emscripten webgl example", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // Make this window the current context
    glfwMakeContextCurrent(window);

	const auto pal = wander::Factory::CreatePal(wander::EPalType::OpenGL, (void *)window); // TODO WebGL
	runtime = wander::Factory::CreateRuntime(pal);

	const auto renderlet_id = runtime->LoadFromFile(L"../demo.rlt", "start");

	const auto tree_id = runtime->Render(renderlet_id);

	// This should move to library code for shader vertex attributes
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

	tree = runtime->GetRenderTree(tree_id);

	const GLchar *vert_shader_wasm = "#version 300 es\n"
									 "in vec3 vs_position;\n"
									 "in vec3 vs_normal;\n"
									 "in vec2 vs_texcoord;\n"
									 "in vec3 vs_color;\n"
									 "out vec2 ps_texcoord;\n"
									 "out vec4 ps_color;\n"
									 "uniform mat4 transform;\n"
									 "uniform mat4 projection;\n"
									 "void main()\n"
									 "{\n"
									 "	gl_Position = projection * transform * vec4(vs_position, 1.0);\n"
									 "	ps_texcoord = vs_texcoord;\n"
									 "	ps_color = vec4(vs_color, 1.0);\n"
									 "}\n";

	const GLchar *frag_shader_wasm = "#version 300 es\n"
									 "precision highp float;\n"
									 "out vec4 out_color;\n"
									 "in vec2 ps_texcoord;\n"
									 "in vec4 ps_color;\n"
									 "void main()\n"
									 "{\n"
									 "    out_color = ps_color;\n"
									 "}\n";

	GLuint vert_wasm = compile_shader(GL_VERTEX_SHADER, vert_shader_wasm);
	GLuint frag_wasm = compile_shader(GL_FRAGMENT_SHADER, frag_shader_wasm);
	program_wasm = link_program(vert_wasm, frag_wasm);
	uniform_transform = glGetUniformLocation(program_wasm, "transform");
	uniform_projection = glGetUniformLocation(program_wasm, "projection");
	glDeleteShader(frag_wasm);
	glDeleteShader(vert_wasm);

	emscripten_fetch_attr_t attr;
	emscripten_fetch_attr_init(&attr);
	strcpy(attr.requestMethod, "GET");
	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
	attr.onsuccess = downloadSucceeded;
	attr.onerror = downloadFailed;
	emscripten_fetch(&attr, "https://rltdemoapi2.azurewebsites.net/compiler/output/test/window.bmp");

#ifdef EMSCRIPTEN
    // Define a mail loop function, that will be called as fast as possible
    emscripten_set_main_loop(&mainLoop, 0, 1);
#else
    // This is the normal C/C++ main loop
    while (!glfwWindowShouldClose(window))
    {
        mainLoop();
    }
#endif

	runtime->Release();

    // Tear down the system
    std::cout << "Loop ended" << std::endl;
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}


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