#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;
out vec3 position;

void main()
{
    vec4 pos = projection * view * vec4(aPos, 1.0);
    position = aPos;
    TexCoord = aTexCoord;

    gl_Position = vec4(pos);
}