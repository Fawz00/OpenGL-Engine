#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D TextureColor;

void main()
{
    vec2 uv = TexCoord;
    vec4 color = texture(TextureColor, uv);

    FragColor = color;
}