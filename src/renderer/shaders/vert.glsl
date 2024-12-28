// GLSL 4.5
#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec4 fragmentColor;

void main()
{
    gl_Position = vec4(position, 1.0);
    fragmentColor = vec4(color, 1.0);
}
