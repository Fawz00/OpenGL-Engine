#include "EngineRenderer.hpp"

// Placeholder for camera movement
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);

void EngineRenderer::onInit() {
	models.push_back(new Model("Resources/engine/models/backpack/backpack.obj"));
	models.push_back(new Model("Resources/engine/models/char6.gltf"));

	models.shrink_to_fit();

    shader = new Shader("Resources/engine/shaders/mesh_vertex.glsl",
        "Resources/engine/shaders/mesh_fragment.glsl");

	camera = new Camera();
	camera->setPivotDistance(0.01f); // FPS style camera
	camera->setRotation(0.0f, 0.0f, 0.0f);
	camera->setPerspective( 70.0f, 0.1f, 100.0f);
	camera->setAspectRatio(Window::width(), Window::height());
	camera->setRotationMode(Camera::ROTATION_LIMITED);

	Window::showCursor(false);
}

void EngineRenderer::onWindowResize() {
    camera->setAspectRatio(Window::width(), Window::height());
}

void EngineRenderer::onUpdate() {
    camera->setRotation(((Input::mouse.getMouseY()/Window::height())-0.5f)*180.0f, Input::mouse.getMouseX()*360.0f/Window::width(), 0.0f);

	if (Input::mouse.isButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
		Debug::log("Mouse X: " + std::to_string(Input::mouse.getMouseX()) + " Y: " + std::to_string(Input::mouse.getMouseY()));
	}

	if (Input::keyboard.isKeyDown(GLFW_KEY_W)) {
		glm::vec3 forward = camera->getForward();

        cameraPos += forward * Time::getLastDeltaTime() * 2.0f;
		camera->setPivotPosition(cameraPos.x, cameraPos.y, cameraPos.z);
	}

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	shader->use();

	shader->setFloat("time", Time::getTime());

	float projection[16];
	camera->getProjectionMatrix(projection);
	shader->setMat4("projection", projection);

	float view[16];
	camera->getViewMatrix(view);
	shader->setMat4("view", view);

	shader->setVec3("viewPos", camera->getWorldPosition());

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
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