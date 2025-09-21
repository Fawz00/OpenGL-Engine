#include "EngineRenderer.hpp"

// Placeholder for camera movement
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);

void EngineRenderer::onInit() {

	// Models and their matrices
	models.push_back(new Model("Resources/engine/models/cube.obj"));
	glm::mat4 mat0 = glm::mat4(1.0f);
	modelMatrices.push_back(mat0);

	models.push_back(new Model("Resources/engine/models/backpack/backpack.obj"));
	glm::mat4 mat1 = glm::mat4(1.0f);
	mat1 = glm::translate(mat1, glm::vec3(0.0f, 0.0f, -2.0f));
	modelMatrices.push_back(mat1);

	models.push_back(new Model("Resources/engine/models/char6.gltf"));
	glm::mat4 mat2 = glm::mat4(1.0f);
	mat2 = glm::translate(mat2, glm::vec3(2.0f, 0.0f, -2.0f));
	modelMatrices.push_back(mat2);

	models.push_back(new Model("Resources/engine/models/vampire/dancing_vampire.dae", Model::SKINNED));
	glm::mat4 mat3 = glm::mat4(1.0f);
	mat3 = glm::translate(mat3, glm::vec3(2.0f, 0.0f, 2.0f));
	modelMatrices.push_back(mat3);

	danceAnimation = new Animation("Resources/engine/models/vampire/dancing_vampire.dae", models[3]);
	animator = new Animator(danceAnimation);

	// Cube map (skybox)
	const char* skyboxFaces[6] = {
		"Resources/engine/skybox/bc/right.png",
		"Resources/engine/skybox/bc/left.png",
		"Resources/engine/skybox/bc/top.png",
		"Resources/engine/skybox/bc/bottom.png",
		"Resources/engine/skybox/bc/front.png",
		"Resources/engine/skybox/bc/back.png"
	};
	skybox = new TextureCubeMap(skyboxFaces, TextureCubeMap::TextureCubeMapSkybox);

	skyboxShader = new Shader("Resources/engine/shaders/skybox_vertex.glsl",
		"Resources/engine/shaders/skybox_fragment.glsl");
    shader = new Shader("Resources/engine/shaders/mesh_skinned_vertex.glsl",
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
	if (Input::keyboard.isKeyDown(GLFW_KEY_S)) {
		glm::vec3 forward = camera->getForward();
		cameraPos -= forward * Time::getLastDeltaTime() * 2.0f;
		camera->setPivotPosition(cameraPos.x, cameraPos.y, cameraPos.z);
	}

	// Render skybox first
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_CULL_FACE);

	skyboxShader->use();

	float viewNoTranslation[16];
	camera->getViewMatrix(viewNoTranslation);
	viewNoTranslation[12] = 0.0f;
	viewNoTranslation[13] = 0.0f;
	viewNoTranslation[14] = 0.0f;
	skyboxShader->setMat4("view", viewNoTranslation);
	float projection[16];
	camera->getProjectionMatrix(projection);
	skyboxShader->setMat4("projection", projection);

	skybox->bind(0);
	skyboxShader->setInt("skybox", 0);

	models[0]->Draw(*skyboxShader);
	skyboxShader->stop();

	// Render rest of scene
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	shader->use();

	animator->UpdateAnimation(shader);

	shader->setFloat("time", Time::getTime());

	float cmProjection[16];
	camera->getProjectionMatrix(cmProjection);
	shader->setMat4("projection", cmProjection);

	float view[16];
	camera->getViewMatrix(view);
	shader->setMat4("view", view);

	shader->setVec3("viewPos", camera->getWorldPosition());

	for (int i = 0; i < models.size(); i++) {
		Model* m = models[i];
		glm::mat4 modelMatrix = modelMatrices[i];
		shader->setMat4("model", glm::value_ptr(modelMatrix));
		m->Draw(*shader);
	}

    shader->stop();
}

void EngineRenderer::onDestroy() {
    delete shader;
	delete skyboxShader;
	delete camera;
	delete skybox;

	for (Model* m : models) {
		delete m;
	}
	models.clear();
	modelMatrices.clear();
	delete danceAnimation;
	delete animator;
}

EngineRenderer::~EngineRenderer() {
}