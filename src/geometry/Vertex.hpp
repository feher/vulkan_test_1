#pragma once

#include <glm/glm.hpp>

namespace VkTest1::Geometry
{

using Position = glm::vec3;
using Color = glm::vec3;

// Vertex data representation
struct Vertex
{
    Position position; // Vertex Position (x, y, z)
    Color color; // Vertex Colour (r, g, b)
};

class Mesh
{
public:
    Mesh();
};

} // namespace VkTest1::Geometry
