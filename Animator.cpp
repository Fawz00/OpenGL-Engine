#include "Animator.hpp"

Animator::Animator(Animation* animation)
	: m_CurrentAnimation(animation),
	m_CurrentTime(0.0f),
	m_DeltaTime(0.0f)
{
	// If animation is valid, resize final matrices to match number of bones
	if (animation) {
		m_FinalBoneMatrices.resize(animation->GetBoneIDMap().size(), glm::mat4(1.0f));
	}
	else {
		m_FinalBoneMatrices.resize(100, glm::mat4(1.0f)); // fallback default
	}

	// UBO
	glGenBuffers(1, &m_BonesUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, m_BonesUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * m_FinalBoneMatrices.size(), nullptr, GL_DYNAMIC_DRAW);
	
	// binding point = 0
	glBindBufferRange(GL_UNIFORM_BUFFER, m_BindingPoint, m_BonesUBO, 0, sizeof(glm::mat4) * m_FinalBoneMatrices.size());
}

Animator::~Animator() {
	glDeleteBuffers(1, &m_BonesUBO);
}

void Animator::UpdateAnimation(Shader* shader) {
	m_DeltaTime = Time::getLastDeltaTime();

	if (m_CurrentAnimation) {
		m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * m_DeltaTime;
		m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());

		// Recursive update
		CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
	}

	// Update shader with final bone matrices
	// Use UBO instead of setting each matrix individually
	glBindBuffer(GL_UNIFORM_BUFFER, m_BonesUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4) * m_FinalBoneMatrices.size(), m_FinalBoneMatrices.data());

	GLuint blockIndex = glGetUniformBlockIndex(shader->getID(), "Bones");
	if (blockIndex != GL_INVALID_INDEX)
		glUniformBlockBinding(shader->getID(), blockIndex, m_BindingPoint);
}

void Animator::PlayAnimation(Animation* animation) {
	m_CurrentAnimation = animation;
	m_CurrentTime = 0.0f;

	if (animation) {
		m_FinalBoneMatrices.assign(animation->GetBoneIDMap().size(), glm::mat4(1.0f));
	}
}

void Animator::CalculateBoneTransform(const AssimpNodeData* node, const glm::mat4& parentTransform) {
	glm::mat4 nodeTransform = node->transformation;

	Bone* bone = m_CurrentAnimation->FindBone(node->name);
	if (bone) {
		bone->Update(m_CurrentTime);
		nodeTransform = bone->GetLocalTransform();
	}

	glm::mat4 globalTransform = parentTransform * nodeTransform;

	const auto& boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
	auto it = boneInfoMap.find(node->name);
	if (it != boneInfoMap.end()) {
		int index = it->second.id;
		const glm::mat4& offset = it->second.offset;
		if (index < static_cast<int>(m_FinalBoneMatrices.size())) {
			m_FinalBoneMatrices[index] = globalTransform * offset;
		}
	}

	for (int i = 0; i < node->childrenCount; i++) {
		CalculateBoneTransform(&node->children[i], globalTransform);
	}
}
