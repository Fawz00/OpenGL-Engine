#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 Position;
in vec3 vColor;
in mat3 TBN;
in mat3 TBN_transpose;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_height1;

uniform uint uTextureFlags;
uniform float time;
uniform vec3 lightDir;
uniform vec3 viewPos;

#define HAS_DIFFUSE  (1u << 0u)
#define HAS_SPECULAR (1u << 1u)
#define HAS_NORMAL   (1u << 2u)
#define HAS_HEIGHT   (1u << 3u)

void main()
{
    vec3 lightColor = vec3(1.0, 0.85, 0.75) * 0.8;

    vec3 lightDir = normalize(-lightDir);
    vec3 viewDir = normalize(viewPos - Position);

    // Texture mapping
    float useDiffuse = float((uTextureFlags & HAS_DIFFUSE) != 0u);
    vec4 color = mix(vec4(1.0), texture(texture_diffuse1, TexCoords), useDiffuse);
    if (color.a < 0.1)
        discard;

    // Normal mapping
    float useNormal = float((uTextureFlags & HAS_NORMAL) != 0u);
    vec3 norm = mix(
    // False
        normalize(Normal),
    // True
        normalize( TBN_transpose * (texture(texture_normal1, TexCoords).rgb * 2.0 - 1.0) ),
    // ?
        useNormal
    );

	// Ambient
    vec3 ambient = vec3(0.5) * color.rgb;

	// diffuse 
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = lightColor * diff * color.rgb;

    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 64.0);

    float useSpecular = float((uTextureFlags & HAS_SPECULAR) != 0u);
    float specStrength = mix(
    // False
        0.2,
    // True
        texture(texture_specular1, TexCoords).r,
    // ?
        useSpecular
    );
    vec3 specular = lightColor * spec * specStrength;

    color = vec4(ambient + diffuse + specular, color.a);

	FragColor = color;
}