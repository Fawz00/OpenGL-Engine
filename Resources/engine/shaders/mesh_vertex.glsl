#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent; 
layout (location = 5) in ivec4 boneIds; 
layout (location = 6) in vec4 weights;

out vec2 TexCoords;
out vec3 Normal;
out vec3 Position;
out vec4 vColor;
out mat3 TBN;
out mat3 TBN_transpose;

out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

uniform mat4 lightSpaceMatrix;

void main()
{
    mat3 normalMatrix = transpose(inverse(mat3(model)));

    TexCoords = aTexCoords;
    Normal = normalize(normalMatrix * aNormal);
    Position = vec3(model * vec4(aPos, 1.0));

    vec3 N = Normal;
    vec3 T = normalize(normalMatrix * aTangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    TBN = transpose(mat3(T, B, N));
    TBN_transpose = transpose(TBN);

    FragPosLightSpace = lightSpaceMatrix * vec4(Position, 1.0);
    gl_Position =  projection * view * vec4(Position, 1.0);
}