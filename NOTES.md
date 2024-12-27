# Notes

## Setup

```
vcpkg new --application
vcpkg add port glfw3
```

## Vulkan

```
Instance ---- Physical Device --+-- Graphics Queue Family ---- Queue
                |               |
                |               +-- Compute Queue Family
                |               |
                |               +-- Transfer Queue Family
                |
              Logical Device 
```

```
GLFW Window ---- Vulkan Window Surface
                   ^
                   |
                   |
                 Swapchain --+-- Image <---- Image View <---- Framebuffer
                             |
                             +-- Image <---- Image View <---- Framebuffer
                             |
                             +-- ...
```


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

Pipeline
- Configures a fixed pipeline (there can be multiple pipelines).
- Contains configurations for:
  - shader modules,
  - vertex input,
  - input assembly,
  - viewport and scissor,
  - rasterizer,
  - multisampling,
  - color blending

Render Pass
???

Frame Buffers
- Just a reference/view to an Image.
- The Render Pass writes its output into an Image referenced via the Frame Buffer.

Command Buffer
- Pre-recorded commands. Submitted to the GPU all at once.
- Allocated from a pool.
- Usual order of commands in a command buffer:
  1. Start Render Pass.
  2. Bind a Pipeline.
  3. Bind vertex, index data.
  4. Bind Descriptor Sets and Push Constants.
  5. Draw.

Queues
- Command Buffers are submitted to a Queue (e.g. Graphics Queue).

Synchronization
- Semaphores
  - Only for synchronization between both GPU-GPU and CPU-GPU.
  - Examples:
    - Signal that an image is available for writing.
    - Signal that an image is available for presenting.
- Fences
  - Can be used for CPU-GPU synchronization.
    - CPU can un-signal (lock) it and wait on it (for unlock). Only the GPU can signal (unlock) it.
  - Examples:
    - Wait for a frame to be available (to avoid flooding the queue with draw/present commands).
