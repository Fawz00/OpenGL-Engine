#include "EngineRenderer.h"
#include "Input.h"

float vertices[] = {
    // positions           // texture coords
    // Front face
    -0.5f, -0.5f,  0.5f,   0.0f, 0.0f, // bottom left
     0.5f, -0.5f,  0.5f,   1.0f, 0.0f, // bottom right
     0.5f,  0.5f,  0.5f,   1.0f, 1.0f, // top right
    -0.5f,  0.5f,  0.5f,   0.0f, 1.0f, // top left
    // Back face
    -0.5f, -0.5f, -0.5f,   1.0f, 0.0f, // bottom left
     0.5f, -0.5f, -0.5f,   0.0f, 0.0f, // bottom right
     0.5f,  0.5f, -0.5f,   0.0f, 1.0f, // top right
    -0.5f,  0.5f, -0.5f,   1.0f, 1.0f  // top left
};
unsigned int indices[] = {
    // Front face
    0, 1, 2,
    2, 3, 0,
    // Right face
    1, 5, 6,
    6, 2, 1,
    // Back face
    5, 4, 7,
    7, 6, 5,
    // Left face
    4, 0, 3,
    3, 7, 4,
    // Top face
    3, 2, 6,
    6, 7, 3,
    // Bottom face
    4, 5, 1,
    1, 0, 4
};

void EngineRenderer::onInit() {
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	texture = new Texture("Resources/engine/textures/image.jpg");

    shader = new Shader("Resources/engine/shaders/basic_vertex.glsl",
        "Resources/engine/shaders/basic_fragment.glsl");

	// Position attribute
    shader->setAttr(0, 3, 5 * sizeof(float), (void*)0);
	// Texture coord attribute
	shader->setAttr(1, 2, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

	camera = new Camera();
	camera->setPivotPosition(0.0f, 0.0f, 0.0f);
	camera->setRotation(0.0f, 0.0f, 0.0f);
	camera->setPivotDistance(3.0f);
	camera->setPerspective( 60.0f, 0.1f, 100.0f);
}

void EngineRenderer::onUpdate() {
    camera->setAspectRatio(Window::width(), Window::height());
    camera->setRotation(0.0f, Input::mouse.getMouseX()*360.0f/Window::width(), 0.0f);

    shader->use();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

	float projection[16];
	camera->getProjectionMatrix(projection);
	shader->setMat4("projection", projection);

	float view[16];
	camera->getViewMatrix(view);
	shader->setMat4("view", view);

	shader->setInt("ourTexture", 0);
	texture->bind(0);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    shader->stop();
}

void EngineRenderer::onDestroy() {
    delete shader;
	delete texture;
	delete camera;

	if (EBO) glDeleteBuffers(1, &EBO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (VAO) glDeleteVertexArrays(1, &VAO);
}

EngineRenderer::~EngineRenderer() {
}