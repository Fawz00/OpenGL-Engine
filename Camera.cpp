#include "Camera.hpp"

Camera::Camera()
    : pivotPosition{ glm::vec3(0.0f) },
    rotation{ glm::vec3(0.0f)},
    aspectRatio(16.0f / 9.0f),
    pivotDistance(5.0f),
    farPlane(1000.0f),
    nearPlane(0.1f),
    scale(1.0f),
    fov(60.0f),
    projectionType(PERSPECTIVE),
    rotationMode(ROTATION_FREE),
    projectionDirty(true),
    viewDirty(true)
{
	setAspectRatio(Window::width(), Window::height());
}

Camera::~Camera() {}

// -------------------- Aspect Ratio --------------------

void Camera::setAspectRatio(int width, int height) {
    if (height == 0) height = 1;
    aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	projectionDirty = true;
}

// -------------------- Pivot --------------------

void Camera::setPivotPosition(glm::vec3 pos) {
    if (glm::all(glm::epsilonEqual(pos, pivotPosition, 1e-6f))) return;
    pivotPosition = pos;
    viewDirty = true;
}

void Camera::setPivotPosition(float x, float y, float z) {
	setPivotPosition(glm::vec3(x, y, z));
}

void Camera::getPivotPosition(float& x, float& y, float& z) const {
    x = pivotPosition.x;
    y = pivotPosition.y;
    z = pivotPosition.z;
}

glm::vec3 Camera::getPivotPosition() const {
    return pivotPosition;
}

// -------------------- World Position --------------------

void Camera::getWorldPosition(float& x, float& y, float& z) const {
    glm::vec3 cameraPos = getWorldPosition();
    x = cameraPos.x;
    y = cameraPos.y;
    z = cameraPos.z;
}

glm::vec3 Camera::getWorldPosition() const {
    // Calculate camera position in world space
    glm::vec3 direction = getForward();
    glm::vec3 cameraPos = pivotPosition - direction * pivotDistance;
	return cameraPos;
}

// -------------------- Rotation --------------------

void Camera::setRotationMode(RotationMode mode) {
    rotationMode = mode;
}

Camera::RotationMode Camera::getRotationMode() const {
    return rotationMode;
}

void Camera::setRotation(glm::vec3 rot) {
    setRotation(rot.x, rot.y, rot.z);
}

void Camera::setRotation(float pitch, float yaw, float roll) {
    if (rotationMode == ROTATION_LIMITED) {
        // Clamp pitch to [-90° 90°]
        pitch = std::clamp(pitch, -89.9f, 89.9f);
    }

    if (glm::all(glm::epsilonEqual(glm::vec3(pitch, yaw, roll), rotation, 1e-6f))) return;

	// Wrap to [-180° 180°]
    pitch = wrapAngle180(pitch);
	yaw   = wrapAngle180(yaw);
	roll  = wrapAngle180(roll);

	rotation = glm::vec3( pitch, yaw, roll );
    viewDirty = true;
}

void Camera::getRotation(float& pitch, float& yaw, float& roll) const {
    pitch = rotation.x;
    yaw = rotation.y;
    roll = rotation.z;
}

glm::vec3 Camera::getRotation() const {
    return rotation;
}

// -------------------- Zoom / Distance --------------------

void Camera::setPivotDistance(float distance) {
    if (std::abs(distance - pivotDistance) < 1e-6f) return;

	if (distance < 0.01f) distance = 0.01f; // Prevent flipping
    pivotDistance = distance;
    viewDirty = true;
}

float Camera::getPivotDistance() const {
    return pivotDistance;
}

// -------------------- Projection --------------------

void Camera::setPerspective(float fov_, float nearPlane_, float farPlane_) {
    fov_ = std::clamp(fov_, 1.0f, 179.0f);

    if (std::abs(fov_ - fov) < 1e-6f &&
        std::abs(nearPlane_ - nearPlane) < 1e-6f &&
        std::abs(farPlane_ - farPlane) < 1e-6f) return;

    fov = fov_;
    nearPlane = nearPlane_;
    farPlane = farPlane_;
    projectionType = PERSPECTIVE;
    projectionDirty = true;
}

void Camera::setOrthographic(float scale_, float nearPlane_, float farPlane_) {
    if (scale_ <= 0.0f) scale_ = 0.01f;

    if (std::abs(scale_ - scale) < 1e-6f &&
        std::abs(nearPlane_ - nearPlane) < 1e-6f &&
        std::abs(farPlane_ - farPlane) < 1e-6f) return;

    scale = scale_;
    nearPlane = nearPlane_;
    farPlane = farPlane_;
    projectionType = ORTHOGRAPHIC;
    projectionDirty = true;
}

void Camera::setProjectionType(ProjectionType type) {
	if (type == projectionType) return;
    projectionType = type;
	projectionDirty = true;
}

Camera::ProjectionType Camera::getProjectionType() const {
    return projectionType;
}

void Camera::setScale(float s) {
	if (s <= 0.0f) s = 0.01f;
	if (s == scale) return;
    scale = s;

    if (projectionType == ORTHOGRAPHIC) {
        projectionDirty = true;
	}
}

float Camera::getScale() const {
    return scale;
}

// -------------------- Matrices --------------------

glm::mat4 Camera::getProjectionMatrix() const {
    if (projectionDirty) {
        if (projectionType == PERSPECTIVE) {
            cachedProjectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        }
        else {
            float halfWidth = scale * aspectRatio * 0.5f;
            float halfHeight = scale * 0.5f;
            cachedProjectionMatrix = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane);
        }
		projectionDirty = false;
	}
	return cachedProjectionMatrix;
}

void Camera::getProjectionMatrix(float* matrix) const {
	glm::mat4 proj = getProjectionMatrix();
    std::memcpy(matrix, glm::value_ptr(proj), sizeof(float) * 16);
}

glm::mat4 Camera::getViewMatrix() const {
    if (viewDirty) {
        updateDirectionVectors(); // updates cachedForward/right/up and clears viewDirty
        glm::vec3 cameraPos = getWorldPosition();

        glm::vec3 f = glm::normalize(cachedForward);
        glm::vec3 r = glm::normalize(cachedRight);
        glm::vec3 u = glm::normalize(cachedUp);

        glm::mat4 view(1.0f);
        // column-major: set basis vectors as columns
        view[0][0] = r.x; view[1][0] = r.y; view[2][0] = r.z;
        view[0][1] = u.x; view[1][1] = u.y; view[2][1] = u.z;
        view[0][2] = -f.x; view[1][2] = -f.y; view[2][2] = -f.z;

        // translation (4th row)
        view[3][0] = -glm::dot(r, cameraPos);
        view[3][1] = -glm::dot(u, cameraPos);
        view[3][2] = glm::dot(f, cameraPos);
        // view[3][3] is already 1.0f from glm::mat4(1.0f)

        cachedViewMatrix = view;
    }
    return cachedViewMatrix;
}

void Camera::getViewMatrix(float* matrix) const {
    glm::mat4 view = getViewMatrix();
	std::memcpy(matrix, glm::value_ptr(view), sizeof(float) * 16);
}

// -------------------- Direction Vectors --------------------

void Camera::getForward(float& x, float& y, float& z) const
{
    glm::vec3 forward = getForward();
    x = forward.x;
    y = forward.y;
    z = forward.z;
}

glm::vec3 Camera::getForward() const { return cachedForward; }
glm::vec3 Camera::getRight()   const { return cachedRight; }
glm::vec3 Camera::getUp()      const { return cachedUp; }

void Camera::updateDirectionVectors() const {
    if (rotationMode == ROTATION_FREE) {
        glm::quat q(glm::radians(rotation));
        glm::mat4 rotMat = glm::mat4_cast(q);

        cachedRight = glm::normalize(glm::vec3(rotMat[0]));
        cachedUp = glm::normalize(glm::vec3(rotMat[1]));
        cachedForward = -glm::normalize(glm::vec3(rotMat[2]));
    }
    else {
        glm::vec3 dir;
        dir.x = std::cos(glm::radians(rotation.y)) * std::cos(glm::radians(rotation.x));
        dir.y = std::sin(glm::radians(rotation.x));
        dir.z = std::sin(glm::radians(rotation.y)) * std::cos(glm::radians(rotation.x));

        cachedForward = glm::normalize(dir);
        cachedRight = glm::normalize(glm::cross(cachedForward, glm::vec3(0.0f, 1.0f, 0.0f)));
        cachedUp = glm::normalize(glm::cross(cachedRight, cachedForward));
    }
}

// -------------------- Utility --------------------

float Camera::wrapAngle180(float angle) {
    angle = std::fmod(angle, 360.0f);
    if (angle > 180.0f) {
        angle -= 360.0f;
    }
    else if (angle < -180.0f) {
        angle += 360.0f;
    }
    return angle;
}