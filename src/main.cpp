#include "Factory.hpp"
#include "window/IWindow.hpp"
#include "renderer/IRenderer.hpp"

// Use a clip space between 0 to 1.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>
#include <print>

using namespace VkTest1;

int main()
{
    auto factory = Factory{};

    auto window = factory.createWindow();
    auto renderer = factory.createRenderer(window.get());

    std::println("Running.");

    while (!window->shouldClose())
    {
        window->handleEvents();
    }

    std::println("Exitting.");

    return EXIT_SUCCESS;
}
