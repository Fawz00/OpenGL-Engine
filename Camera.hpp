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
	void setPivotPosition(glm::vec3);
	void setPivotPosition(float x, float y, float z);
	void getPivotPosition(float& x, float& y, float& z) const;
	glm::vec3 getPivotPosition() const;

	// Direction vector
	glm::vec3 getForward() const;
	void getForward(float& x, float& y, float& z) const;
	glm::vec3 getUp() const;
	glm::vec3 getRight() const;

	// World position
	void getWorldPosition(float& x, float& y, float& z) const; // position in world space
	glm::vec3 getWorldPosition() const;

	// Rotation (in degrees)
	void setRotation(glm::vec3 rot); // pitch, yaw, roll
	void setRotation(float pitch, float yaw, float roll);
    void getRotation(float& pitch, float& yaw, float& roll) const;
	glm::vec3 getRotation() const;

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
	glm::mat4 getProjectionMatrix() const;
	void getViewMatrix(float* matrix) const;
	glm::mat4 getViewMatrix() const;

	// Rotation mode
	void setRotationMode(RotationMode mode);
	RotationMode getRotationMode() const;

	// Aspect ratio
	void setAspectRatio(int width, int height);

private:
	float wrapAngle180(float angle);
	void updateDirectionVectors() const;

	// Cached variables for optimization
	mutable glm::mat4 cachedProjectionMatrix;
	mutable glm::mat4 cachedViewMatrix;

	mutable glm::vec3 cachedForward;
	mutable glm::vec3 cachedRight;
	mutable glm::vec3 cachedUp;

	mutable bool projectionDirty;
	mutable bool viewDirty;

	RotationMode rotationMode;
	glm::vec3 pivotPosition;
	glm::vec3 rotation; // pitch, yaw, roll
	float aspectRatio;
	float pivotDistance;
	float farPlane;
	float nearPlane;

	// Orthographic parameter
	float scale;

	// Perspective parameter
	float fov; // in degrees

	ProjectionType projectionType;
};