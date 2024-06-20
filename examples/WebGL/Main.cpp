#include <GLFW/glfw3.h>
#include <iostream>

// Include the EMSCRIPTEN specific headers
#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#include "wander.h"

// For example purpose we use global variables
#define SPEED 0.005f
GLFWwindow* window = nullptr;
float red = 0.0f;
bool increase = true;

const wander::RenderTree* tree;
wander::IRuntime* runtime;

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

	tree = runtime->GetRenderTree(tree_id);

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

    // Tear down the system
    std::cout << "Loop ended" << std::endl;
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
