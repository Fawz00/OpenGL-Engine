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
out vec3 tangentPos;
out vec3 tangentViewPos;
out vec3 tangentLightPos;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;

layout(std140) uniform Bones {
    mat4 finalBonesMatrices[MAX_BONES];
};

void main()
{
    TexCoords = aTexCoords;

    // Skinning position
    vec4 totalPosition = vec4(0.0);
    vec3 skinnedNormal = vec3(0.0);
    vec3 skinnedTangent = vec3(0.0);
    vec3 skinnedBitangent = vec3(0.0);

    for(int i = 0; i < MAX_BONE_INFLUENCE; i++) {
        int id = boneIds[i];
        float w = weights[i];
        if(id < 0 || w <= 0.0) continue;

        mat4 boneTransform = finalBonesMatrices[id];
        totalPosition += (boneTransform * vec4(aPos, 1.0)) * w;
        skinnedNormal   += mat3(boneTransform) * aNormal    * w;
        skinnedTangent  += mat3(boneTransform) * aTangent   * w;
        skinnedBitangent+= mat3(boneTransform) * aBitangent * w;
    }

    // fallback if no bones affect this vertex
    if(length(totalPosition) < 1e-6) {
        totalPosition = vec4(aPos, 1.0);
        skinnedNormal = aNormal;
        skinnedTangent = aTangent;
        skinnedBitangent = aBitangent;
    }

    Position = vec3(model * totalPosition);
    Normal = normalize(mat3(model) * skinnedNormal);

    vec3 T = normalize(mat3(model) * skinnedTangent);
    vec3 B = normalize(mat3(model) * skinnedBitangent);
    vec3 N = normalize(Normal);
    TBN = transpose(mat3(T, B, N));

    tangentLightPos = TBN * vec3(1.0, 1.7, 10.0);
    tangentViewPos  = TBN * viewPos;
    tangentPos      = TBN * Position;

    gl_Position = projection * view * vec4(Position, 1.0);
}