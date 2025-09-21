#pragma once

#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include "Animation.hpp"
#include "Bone.hpp"
#include "Time.hpp"
#include "Shader.hpp"

class Animator {
public:
    explicit Animator(Animation* animation);
	~Animator();

    void UpdateAnimation(Shader* shader);
    void PlayAnimation(Animation* animation);

	// No copy
    inline const std::vector<glm::mat4>& GetFinalBoneMatrices() const noexcept {
        return m_FinalBoneMatrices;
    }

private:
    void CalculateBoneTransform(const AssimpNodeData* node, const glm::mat4& parentTransform);

    std::vector<glm::mat4> m_FinalBoneMatrices;
    Animation* m_CurrentAnimation{ nullptr };
    float m_CurrentTime{ 0.0f };
    float m_DeltaTime{ 0.0f };

    GLuint m_BonesUBO;
    GLuint m_BindingPoint = 0;
};