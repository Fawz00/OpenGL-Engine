#include "EngineRendererMesh.hpp"

float vertices__[] = {
    // pos              // tex
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,

    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f
};

unsigned int indices__[] = {
    0, 1, 2, 2, 3, 0,
    1, 5, 6, 6, 2, 1,
    5, 4, 7, 7, 6, 5,
    4, 0, 3, 3, 7, 4,
    3, 2, 6, 6, 7, 3,
    4, 5, 1, 1, 0, 4
};

void EngineRendererMesh::onInit() {
    // Convert raw arrays to std::vector for Mesh constructor
    std::vector<Vertex> meshVertices;
    std::vector<unsigned int> meshIndices(indices__, indices__ + sizeof(indices__) / sizeof(unsigned int));

    // Convert raw vertex data to Vertex struct
    for (int i = 0; i < 8; ++i) {
        Vertex v;
        v.Position = glm::vec3(vertices__[i * 5 + 0], vertices__[i * 5 + 1], vertices__[i * 5 + 2]);
        v.TexCoords = glm::vec2(vertices__[i * 5 + 3], vertices__[i * 5 + 4]);
        v.Normal = glm::vec3(0.0f, 0.0f, 1.0f); // Default normal, adjust as needed
        v.Tangent = glm::vec3(0.0f);
        v.Bitangent = glm::vec3(0.0f);
        for (int j = 0; j < MAX_BONE_INFLUENCE; ++j) {
            v.m_BoneIDs[j] = 0;
            v.m_Weights[j] = 0.0f;
        }
        meshVertices.push_back(v);
    }

    texture = new Texture2D("Resources/engine/textures/image.jpg");
    std::vector<Texture2D*> meshTextures;
    meshTextures.push_back(texture);

	mesh = new Mesh(meshVertices, meshIndices, meshTextures);
    ourModel = new Model("Resources/engine/models/angkot.obj");

    shader = new Shader("Resources/engine/shaders/mesh_vertex.glsl",
        "Resources/engine/shaders/mesh_fragment.glsl");

	camera = new Camera();
	camera->setPivotPosition(0.0f, 0.0f, 0.0f);
	camera->setRotation(0.0f, 0.0f, 0.0f);
	camera->setPivotDistance(3.0f);
	camera->setPerspective( 60.0f, 0.1f, 100.0f);
}

void EngineRendererMesh::onWindowResize() {
    camera->setAspectRatio(Window::width(), Window::height());
}

void EngineRendererMesh::onUpdate() {
    camera->setRotation(Input::mouse.getMouseY() * 360.0f / Window::width(), Input::mouse.getMouseX()*360.0f/Window::width(), 0.0f);

    shader->use();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

	shader->setFloat("time", AppTime::getTime());

	float projection[16];
	camera->getProjectionMatrix(projection);
	shader->setMat4("projection", projection);

	float view[16];
	camera->getViewMatrix(view);
	shader->setMat4("view", view);

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
	model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
	shader->setMat4("model", glm::value_ptr(model));

	ourModel->Draw(*shader);

    shader->stop();
}

void EngineRendererMesh::onDestroy() {
    delete shader;
	delete camera;
	delete ourModel;
	delete mesh;
	delete texture;
}

EngineRendererMesh::~EngineRendererMesh() {
}