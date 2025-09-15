#include "Camera.hpp"

Camera::Camera()
    : pivotPosition{ 0.0f, 0.0f, 0.0f },
    rotation{ 0.0f, 0.0f, 0.0f },
    aspectRatio(16.0f / 9.0f),
    pivotDistance(5.0f),
    farPlane(1000.0f),
    nearPlane(0.1f),
    scale(1.0f),
    fov(60.0f),
    projectionType(PERSPECTIVE),
    rotationMode(ROTATION_FREE)
{
	setAspectRatio(Window::width(), Window::height());
}

Camera::~Camera() {}

void Camera::setPivotPosition(float x, float y, float z) {
    pivotPosition[0] = x;
    pivotPosition[1] = y;
    pivotPosition[2] = z;
}

void Camera::getPivotPosition(float& x, float& y, float& z) const {
    x = pivotPosition[0];
    y = pivotPosition[1];
    z = pivotPosition[2];
}

glm::vec3 Camera::getForward() const {
    // Calculate camera direction
    glm::vec3 direction;
    direction.x = std::cos(glm::radians(rotation[1])) * std::cos(glm::radians(rotation[0]));
    direction.y = std::sin(glm::radians(rotation[0]));
    direction.z = std::sin(glm::radians(rotation[1])) * std::cos(glm::radians(rotation[0]));
    return glm::normalize(direction);
}

glm::vec3 Camera::getUp() const {
    glm::vec3 right = glm::normalize(glm::cross(getForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
    return glm::normalize(glm::cross(right, getForward()));
}
glm::vec3 Camera::getRight() const {
    return glm::normalize(glm::cross(getForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

void Camera::getWorldPosition(float& x, float& y, float& z) const {
    // Calculate camera position in world space
    glm::vec3 pivot(pivotPosition[0], pivotPosition[1], pivotPosition[2]);
    glm::vec3 direction = getForward();
    glm::vec3 cameraPos = pivot - direction * pivotDistance;
    x = cameraPos.x;
    y = cameraPos.y;
    z = cameraPos.z;
}

void Camera::setRotationMode(RotationMode mode) {
    rotationMode = mode;
}

Camera::RotationMode Camera::getRotationMode() const {
    return rotationMode;
}

void Camera::setRotation(float pitch, float yaw, float roll) {
    if (rotationMode == ROTATION_LIMITED) {
        // Clamp pitch to [-90° 90°]
        pitch = std::clamp(pitch, -89.99f, 89.99f);
    }

	// Wrap to [-180° 180°]
	pitch = (glm::fract((pitch - 180.0f) / 360.0f) * 360.0f) - 180.0f;
    yaw   = (glm::fract((yaw   - 180.0f) / 360.0f) * 360.0f) - 180.0f;
	roll  = (glm::fract((roll  - 180.0f) / 360.0f) * 360.0f) - 180.0f;

    rotation[0] = pitch;
    rotation[1] = yaw;
    rotation[2] = roll;
}

void Camera::getRotation(float& pitch, float& yaw, float& roll) const {
    pitch = rotation[0];
    yaw = rotation[1];
    roll = rotation[2];
}

void Camera::setPivotDistance(float distance) {
    pivotDistance = distance;
}

float Camera::getPivotDistance() const {
    return pivotDistance;
}

void Camera::setPerspective(float fov_, float nearPlane_, float farPlane_) {
    fov = fov_;
    nearPlane = nearPlane_;
    farPlane = farPlane_;
    projectionType = PERSPECTIVE;
}

void Camera::setOrthographic(float scale_, float nearPlane_, float farPlane_) {
    scale = scale_;
    nearPlane = nearPlane_;
    farPlane = farPlane_;
    projectionType = ORTHOGRAPHIC;
}

void Camera::setProjectionType(ProjectionType type) {
    projectionType = type;
}

Camera::ProjectionType Camera::getProjectionType() const {
    return projectionType;
}

void Camera::setScale(float s) {
    scale = s;
}

float Camera::getScale() const {
    return scale;
}

void Camera::getProjectionMatrix(float* matrix) const {
    glm::mat4 proj;
    if (projectionType == PERSPECTIVE) {
        proj = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    }
    else {
        float halfWidth = scale * aspectRatio * 0.5f;
        float halfHeight = scale * 0.5f;
        proj = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane);
    }
    std::memcpy(matrix, glm::value_ptr(proj), sizeof(float) * 16);
}

void Camera::getViewMatrix(float* matrix) const {
    glm::vec3 pivot(pivotPosition[0], pivotPosition[1], pivotPosition[2]);
	glm::vec3 cameraPos = getWorldPosition();
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    if (rotation[0] >= 90.0f || rotation[0] <= -90.0f) {
        up = glm::vec3(0.0f, -1.0f, 0.0f); // Invert up vector when looking straight up or down
	}
    
    glm::mat4 view = glm::lookAt(
        cameraPos,
        pivot,
        up
    );
    std::memcpy(matrix, glm::value_ptr(view), sizeof(float) * 16);
}