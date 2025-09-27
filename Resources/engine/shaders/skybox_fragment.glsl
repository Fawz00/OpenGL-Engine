#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in vec3 position;

uniform samplerCube tex;

void main()
{
    vec2 uv = TexCoord;
    vec4 color = texture(tex, position);

    gl_FragDepth = 1.0; // Set depth to farthest

    FragColor = vec4(color);
}
