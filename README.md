# Prerequisites

You have to install these manually.

- Vulkan SDK (tested with 1.3.196.0)
- CMake (tested with 3.29.5-msvc4)

# Build

Set the `VCPKG_ROOT` environment variable to your vcpkg directory or edit the path in `CMakePresets.json`.

For example, if you use the built-in vcpkg that comes with Visual Studio (2022 and later),
then `VCPKG_ROOT` should be set to `C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\vcpkg`.
