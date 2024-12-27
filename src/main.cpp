#include "Factory.hpp"
#include "common/IFileSystem.hpp"
#include "renderer/IRenderer.hpp"
#include "window/IWindow.hpp"

// Use a clip space between 0 to 1.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>
#include <print>

using namespace VkTest1;

int main()
{
    try
    {
        auto factory = Factory{};

        auto fileSystem = factory.createFileSystem();
        auto window = factory.createWindow();
        auto renderer = factory.createRenderer(fileSystem.get(), window.get());

        std::println("Running.");

        while (!window->shouldClose())
        {
            renderer->draw();
            window->handleEvents();
        }

        std::println("Exitting.");
    }
    catch (const std::exception& ex)
    {
        std::println("EXCEPTION: {}", ex.what());
    }

    return EXIT_SUCCESS;
}
