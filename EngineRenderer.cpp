#include "EngineRenderer.hpp"

void EngineRenderer::onInit() {
	models.push_back(new Model("Resources/engine/models/angkot.obj"));
	models.push_back(new Model("Resources/engine/models/cc201_body.obj"));

    shader = new Shader("Resources/engine/shaders/mesh_vertex.glsl",
        "Resources/engine/shaders/mesh_fragment.glsl");

	camera = new Camera();
	camera->setPivotPosition(0.0f, 0.0f, 0.0f);
	camera->setRotation(0.0f, 0.0f, 0.0f);
	camera->setPivotDistance(3.0f);
	camera->setPerspective( 60.0f, 0.1f, 100.0f);
}

void EngineRenderer::onWindowResize() {
    camera->setAspectRatio(Window::width(), Window::height());
}

void EngineRenderer::onUpdate() {
    camera->setRotation(Input::mouse.getMouseY() * 360.0f / Window::width(), Input::mouse.getMouseX()*360.0f/Window::width(), 0.0f);

    shader->use();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	shader->setFloat("time", AppTime::getTime());

	float projection[16];
	camera->getProjectionMatrix(projection);
	shader->setMat4("projection", projection);

	float view[16];
	camera->getViewMatrix(view);
	shader->setMat4("view", view);

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f)); // translate it down so it's at the center of the scene
	model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
	shader->setMat4("model", glm::value_ptr(model));

	for (Model* m : models) {
		m->Draw(*shader);
	}

    shader->stop();
}

void EngineRenderer::onDestroy() {
    delete shader;
	delete camera;
	for (Model* m : models) {
		delete m;
	}
}

EngineRenderer::~EngineRenderer() {
}