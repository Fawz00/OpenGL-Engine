#version 330 core

#line 3

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

    // Alpha test
    float alphaMask = step(0.1, color.a);
    gl_FragDepth = gl_FragCoord.z * alphaMask + (1.0 - alphaMask); // mix(1.0, gl_FragCoord.z, alphaMask);

	FragColor = color;
}