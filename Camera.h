#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include "Window.h"

class Camera {
public:
	enum ProjectionType {
		PERSPECTIVE,
		ORTHOGRAPHIC
	};

	Camera();
	~Camera();
	void setPivotPosition(float x, float y, float z);
	void getPivotPosition(float& x, float& y, float& z) const;
	glm::vec3 getPivotPosition() const {
		return glm::vec3(pivotPosition[0], pivotPosition[1], pivotPosition[2]);
	}
	void getAbsolutePosition(float& x, float& y, float& z) const; // position in world space
	glm::vec3 getAbsolutePosition() const {
		float x, y, z;
		getAbsolutePosition(x, y, z);
		return glm::vec3(x, y, z);
	}
	void setRotation(float pitch, float yaw, float roll);
    void getRotation(float& pitch, float& yaw, float& roll) const;
	glm::vec3 getRotation() const {
		return glm::vec3(rotation[0], rotation[1], rotation[2]);
	}
	void setPivotDistance(float distance);
	float getPivotDistance() const;
	void setPerspective(float fov, float nearPlane, float farPlane);
	void setOrthographic(float scale, float nearPlane, float farPlane);
	void setProjectionType(ProjectionType type);
	ProjectionType getProjectionType() const;
	void setScale(float s);
	float getScale() const;

	void getProjectionMatrix(float* matrix) const;
	void getViewMatrix(float* matrix) const;

	void setAspectRatio(int width, int height) {
		if (height == 0) height = 1;
		aspectRatio = static_cast<float>(width) / static_cast<float>(height);
	}

private:
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