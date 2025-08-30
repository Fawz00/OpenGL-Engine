#include "EngineRenderer.h"

const float vertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
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
    shader->setAttr(0, 3, 3 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0); // optional, unbind
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