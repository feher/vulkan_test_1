# Prerequisites

You have to install these manually.

- Vulkan SDK (tested with 1.3.196.0)
- CMake (tested with 3.29.5-msvc4)

# Build

Set the `VCPKG_ROOT` environment variable to your vcpkg directory or edit the path in `CMakePreset.json`.
For example, if you use the built-in vcpkg that comes with Visual Studio (2022 and later),
then `VCPKG_ROOT` should be set to `C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\vcpkg`.

# Notes

## Setup

vcpkg new --application
vcpkg add port glfw3

## Vulkan

Vulkan Instance
- References the Vulkan Context.
- Defines the Vulkan version and its capabilities.

Physical Device
- References the GPU itself.
- Holds
  - Memory
  - Queues
    - Commands submitted to the GPU in FIFO order.
    - Different queues for different types of commands.
  - Queue Family
    - Collection of queues.
    - Types of families:
      - Graphics: A family for graphics commands.
      - Compute: A family for compute shaders (generic commands).
      - Transfer: A family for data transfer commands.
      - or a mixture of the above.
- Retrieved, enumerated (not created).

Logical Device
- Interface to Physical Device.
- Creation
  - Define queue families and number of queues.
  - Define device features.
  - Define extensions.

Extensions
- E.g. to create/manage OS GUI windows.

Validation Layers
- Vulkan does not report errors by default. It just crashes (for performance reasons).
- Split into layers.
- Each layer must be explcitly enabled.
  - Done through the Instance.
- VK_LAYER_LUNARG_standard_validation is a common all-round validation layer.
- Errors are reported via yet another optional extension.

Surfaces
- Interface between the OS window and the image in the swapchain.
- Provided by an extension.

Swapchains
- Group of Images that can be drawn on and be presented.
- Provided by an extension.
- The Graphics Queue has the feature to present swapchain images.
  - Presentation Queue == Graphics Queue
- 3 main parts:
  - Surface Capabilities (e.g. size)
  - Surface Format (e.g. RGB)
  - Presentation Mode (e.g. order and timing)

Images
- The swapchain creates a group of images.

Image Views
- An interface to an Image.
- Describes how to interpret (read/write) the Image.
  - 2D or 3D, format, etc
  - color channels, mip levels, etc
