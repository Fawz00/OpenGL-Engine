#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;

uniform uint uTextureFlags;
uniform float uTime;

#define HAS_DIFFUSE  (1u << 0u)

void main()
{
    // Texture mapping
    float useDiffuse = float((uTextureFlags & HAS_DIFFUSE) != 0u);
    vec4 color = mix(vec4(1.0), texture(texture_diffuse1, TexCoords), useDiffuse);

#ifdef ALPHA_TEST
    if (color.a < 0.1)
        discard;
#endif

	FragColor = color;
}