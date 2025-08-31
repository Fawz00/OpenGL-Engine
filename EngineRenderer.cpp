#include "EngineRenderer.h"

float vertices[] = {
    // positions          // colors           // texture coords
     0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
     0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
    -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
    -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left 
};
unsigned int indices[] = {
    0, 1, 3, // first triangle
    1, 2, 3  // second triangle
};
unsigned int VBO, VAO, EBO;
Shader* EngineRenderer::shader = nullptr;
Texture* EngineRenderer::texture = nullptr;

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

    shader = new Shader("Resources/engine/shaders/basic_vertex.txt",
        "Resources/engine/shaders/basic_fragment.txt");

	// Position attribute
    shader->setAttr(0, 3, 8 * sizeof(float), (void*)0);
	// Color attribute
    shader->setAttr(1, 3, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	// Texture coord attribute
	shader->setAttr(2, 2, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void EngineRenderer::onUpdate(float deltaTime) {
    shader->use();
	shader->setInt("ourTexture", 0);
	texture->bind(0);
    glBindVertexArray(VAO);
    //glDrawArrays(GL_TRIANGLES, 0, 6);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    shader->stop();
}

EngineRenderer::~EngineRenderer() {
    if (shader) {
        shader->destroy();
        delete shader;
        shader = nullptr;
    }
    if (texture) {
        delete texture;
        texture = nullptr;
	}

	if (EBO) glDeleteBuffers(1, &EBO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (VAO) glDeleteVertexArrays(1, &VAO);
}