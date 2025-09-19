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

uniform float time;
uniform vec3 viewPos;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];

void main()
{
    TexCoords = aTexCoords;
    Normal = aNormal;
    Position = vec3(model * vec4(aPos, 1.0));

    vec3 T = normalize(vec3(model * vec4(aTangent,   0.0)));
    vec3 B = normalize(vec3(model * vec4(aBitangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(aNormal,    0.0)));
    TBN = transpose(mat3(T, B, N));

    tangentLightPos = TBN * vec3(1.0, 1.7, 10.0);
    tangentViewPos  = TBN * viewPos;
    tangentPos      = TBN * Position;

    vec4 totalPosition = vec4(0.0f);
    for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
    {
        if(boneIds[i] == -1) 
            continue;
        if(boneIds[i] >= MAX_BONES) 
        {
            totalPosition = vec4(aPos,1.0f);
            break;
        }
        vec4 localPosition = finalBonesMatrices[boneIds[i]] * vec4(aPos,1.0f);
        totalPosition += localPosition * weights[i];
        vec3 localNormal = mat3(finalBonesMatrices[boneIds[i]]) * aNormal;
   }

   if (totalPosition == vec4(0.0f))
       totalPosition = vec4(aPos,1.0f);

    mat4 viewModel = view * model;
    gl_Position =  projection * viewModel * totalPosition;
}