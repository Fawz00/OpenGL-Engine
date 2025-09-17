#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 Position;
in vec3 tangentPos;
in vec3 tangentViewPos;
in vec3 tangentLightPos;
in mat3 TBN;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_height1;

uniform uint uTextureFlags;
uniform float time;

#define HAS_DIFFUSE  (1u << 0u)
#define HAS_SPECULAR (1u << 1u)
#define HAS_NORMAL   (1u << 2u)
#define HAS_HEIGHT   (1u << 3u)

void main()
{
    vec3 lightColor      = vec3(1.0, 0.9, 0.875);

    vec3 lightDir = normalize(tangentLightPos - tangentPos);
    vec3 viewDir = normalize(tangentViewPos - tangentPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    // Diffuse color mapping
    float useDiffuse = float((uTextureFlags & HAS_DIFFUSE) != 0u);
    vec4 color = mix(vec4(1.0), texture(texture_diffuse1, TexCoords), useDiffuse);
    if (color.a < 0.1)
        discard;

    // Normal mapping
    float useNormal = float((uTextureFlags & HAS_NORMAL) != 0u);
    vec3 norm = mix(Normal, texture(texture_normal1, TexCoords).rgb * 2.0 - 1.0, useNormal);
    norm = normalize(TBN * norm);

	// Ambient
    vec3 ambient = vec3(0.5) * color.rgb;

	// diffuse 
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = lightColor * diff * color.rgb;

    // specular
    float useSpecular = float((uTextureFlags & HAS_SPECULAR) != 0u);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 8.0 );
    vec3 specular = lightColor * spec * mix(0.5, texture(texture_specular1, TexCoords).r, useSpecular);

    color = vec4(ambient + diffuse + specular, color.a);

    //color.rgb = mix( clamp((viewPos - Position)*20.0, 0.0, 1.0), color.rgb, 0.5);

	FragColor = color;
}