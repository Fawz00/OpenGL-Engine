#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <algorithm>

#include "Window.hpp"

class Camera {
public:
	enum ProjectionType {
		PERSPECTIVE,
		ORTHOGRAPHIC
	};
	enum RotationMode {
		ROTATION_FREE,
		ROTATION_LIMITED
	};

	// Constructor / Destructor
	Camera();
	~Camera();

	// Pivot position
	void setPivotPosition(float x, float y, float z);
	void getPivotPosition(float& x, float& y, float& z) const;
	glm::vec3 getPivotPosition() const {
		return glm::vec3(pivotPosition[0], pivotPosition[1], pivotPosition[2]);
	}
	void getForward(float& x, float& y, float& z) const
	{
		glm::vec3 forward = getForward();
		x = forward.x;
		y = forward.y;
		z = forward.z;
	}

	// Direction vector
	glm::vec3 getForward() const;
	glm::vec3 getUp() const;
	glm::vec3 getRight() const;

	// World position
	void getWorldPosition(float& x, float& y, float& z) const; // position in world space
	glm::vec3 getWorldPosition() const {
		float x, y, z;
		getWorldPosition(x, y, z);
		return glm::vec3(x, y, z);
	}

	// Rotation (in degrees)
	void setRotation(float pitch, float yaw, float roll);
    void getRotation(float& pitch, float& yaw, float& roll) const;
	glm::vec3 getRotation() const {
		return glm::vec3(rotation[0], rotation[1], rotation[2]);
	}

	// Zoom (distance from pivot)
	void setPivotDistance(float distance);
	float getPivotDistance() const;

	// Projection
	void setPerspective(float fov, float nearPlane, float farPlane);
	void setOrthographic(float scale, float nearPlane, float farPlane);
	void setProjectionType(ProjectionType type);
	ProjectionType getProjectionType() const;

	// Scale (for orthographic)
	void setScale(float s);
	float getScale() const;

	// Matrices
	void getProjectionMatrix(float* matrix) const;
	void getViewMatrix(float* matrix) const;

	// Rotation mode
	void setRotationMode(RotationMode mode);
	RotationMode getRotationMode() const;

	// Aspect ratio
	void setAspectRatio(int width, int height) {
		if (height == 0) height = 1;
		aspectRatio = static_cast<float>(width) / static_cast<float>(height);
	}

private:
	RotationMode rotationMode;
	float pivotPosition[3];
	float rotation[3]; // pitch, yaw, roll
	float aspectRatio;
	float pivotDistance;
	float farPlane;
	float nearPlane;
	float scale;

	// Perspective parameters
	float fov; // in degrees

	ProjectionType projectionType;
};