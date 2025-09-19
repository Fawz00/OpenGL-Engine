#pragma once

#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include "Animation.hpp"
#include "Bone.hpp"
#include "Time.hpp"

class Animator
{
public:
	Animator(Animation* animation);

	void UpdateAnimation(Shader* shader);
	void PlayAnimation(Animation* pAnimation);
	void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform);
	inline std::vector<glm::mat4> GetFinalBoneMatrices()
	{
		return m_FinalBoneMatrices;
	}

private:
	std::vector<glm::mat4> m_FinalBoneMatrices;
	Animation* m_CurrentAnimation;
	float m_CurrentTime;
	float m_DeltaTime;
};