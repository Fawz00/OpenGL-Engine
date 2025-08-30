#include "EngineRenderer.h"

float vertices[] = {
    // positions         // colors
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // bottom right
    -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // bottom left
     0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // top 
};
unsigned int VBO;
unsigned int VAO;
Shader* EngineRenderer::shader = nullptr;

void EngineRenderer::onInit() {
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    shader = new Shader("Resources/engine/shaders/basic_vertex.txt",
        "Resources/engine/shaders/basic_fragment.txt");

	// Position attribute
    shader->setAttr(0, 3, 6 * sizeof(float), (void*)0);
	// Color attribute
    shader->setAttr(1, 3, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void EngineRenderer::onUpdate(float deltaTime) {
    shader->use();
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    shader->stop();
}

EngineRenderer::~EngineRenderer() {
    if (shader) {
        shader->destroy();
        delete shader;
        shader = nullptr;
    }

    if (VBO) glDeleteBuffers(1, &VBO);
    if (VAO) glDeleteVertexArrays(1, &VAO);
}