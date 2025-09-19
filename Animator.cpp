#include "Animator.hpp"

Animator::Animator(Animation* animation) : m_CurrentAnimation(animation), m_CurrentTime(0.0f), m_DeltaTime(0.0f)
{
	m_CurrentTime = 0.0f;
	m_CurrentAnimation = animation;

	m_FinalBoneMatrices.reserve(100);
	
	for (int i = 0; i < 100; i++)
		m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
}

void Animator::UpdateAnimation(Shader* shader)
{
	m_DeltaTime = Time::getLastDeltaTime();
	if (m_CurrentAnimation)
	{
		m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * m_DeltaTime;
		m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
		CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
	}

	auto transforms = GetFinalBoneMatrices();
	for (int i = 0; i < transforms.size(); ++i)
		shader->setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);
}

void Animator::PlayAnimation(Animation* pAnimation)
{
	m_CurrentAnimation = pAnimation;
	m_CurrentTime = 0.0f;
}

void Animator::CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform)
{
	std::string nodeName = node->name;
	glm::mat4 nodeTransform = node->transformation;
	Bone* bone = m_CurrentAnimation->FindBone(nodeName);
	if (bone)
	{
		bone->Update(m_CurrentTime);
		nodeTransform = bone->GetLocalTransform();
	}
	glm::mat4 globalTransformation = parentTransform * nodeTransform;
	auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
	if (boneInfoMap.find(nodeName) != boneInfoMap.end())
	{
		int index = boneInfoMap[nodeName].id;
		glm::mat4 offset = boneInfoMap[nodeName].offset;
		m_FinalBoneMatrices[index] = globalTransformation * offset;
	}
	for (int i = 0; i < node->childrenCount; i++)
	{
		CalculateBoneTransform(&node->children[i], globalTransformation);
	}
}
