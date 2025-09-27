#include "EngineRenderer.hpp"

void EngineRenderer::onInit() {
	ScreenQuad::init();

	renderTexture = new RenderTexture(Window::width(), Window::height());
	renderTexture->addColorAttachment(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, Texture2D::FilterLinear, false);
	renderTexture->useDepthRBO();
	renderTexture->resize(Window::width(), Window::height());

	// Models and their matrices
	models.push_back(new Model("Resources/engine/models/cube.obj"));
	glm::mat4 mat0 = glm::mat4(1.0f);
	modelMatrices.push_back(mat0);

	models.push_back(new Model("Resources/engine/models/backpack/backpack.obj"));
	glm::mat4 mat2 = glm::mat4(1.0f);
	mat2 = glm::translate(mat2, glm::vec3(0.0f, 0.0f, 3.0f));
	mat2 = glm::scale(mat2, glm::vec3(0.3f));
	modelMatrices.push_back(mat2);

	models.push_back(new Model("Resources/engine/models/tropical.fbx"));
	glm::mat4 mat1 = glm::mat4(1.0f);
	mat1 = glm::translate(mat1, glm::vec3(-612.0f, -19.0f, 465.0f));
	modelMatrices.push_back(mat1);

	models.push_back(new Model("Resources/engine/models/char13.fbx", Model::SKINNED));
	glm::mat4 mat_4 = glm::mat4(1.0f);
	mat_4 = glm::translate(mat_4, glm::vec3(2.0f, 0.0f, 2.0f));
	mat_4 = glm::scale(mat_4, glm::vec3(0.01f)); // FBX model is huge
	modelMatrices.push_back(mat_4);

	danceAnimation = new Animation("Resources/engine/models/Happy.fbx", models[3]);
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
	quadShader = new Shader("Resources/engine/shaders/quad_vertex.glsl",
		"Resources/engine/shaders/quad_fragment.glsl");

	camera = new Camera();
	camera->setPivotDistance(0.01f); // FPS style camera
	camera->setRotation(0.0f, 0.0f, 0.0f);
	camera->setPerspective( 70.0f, 0.1f, 1000.0f);
	camera->setAspectRatio(Window::width(), Window::height());
	camera->setRotationMode(Camera::ROTATION_LIMITED);

	Window::showCursor(false);
}

void EngineRenderer::onWindowResize() {
    camera->setAspectRatio(Window::width(), Window::height());

	renderTexture->resize(Window::width(), Window::height());
}

void EngineRenderer::onUpdate() {
    camera->setRotation(-((Input::mouse.getMouseY()/Window::height())-0.5f)*180.0f, Input::mouse.getMouseX()*360.0f/Window::width(), 0.0f);

	if (Input::mouse.isButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
		Debug::log("Mouse X: " + std::to_string(Input::mouse.getMouseX()) + " Y: " + std::to_string(Input::mouse.getMouseY()));
	}

	if (Input::keyboard.isKeyDown(GLFW_KEY_W)) {
		glm::vec3 forward = camera->getForward();
        camera->setPivotPosition( camera->getPivotPosition() + forward * Time::getLastDeltaTime() * 2.0f );
	}
	if (Input::keyboard.isKeyDown(GLFW_KEY_S)) {
		glm::vec3 forward = camera->getForward();
		camera->setPivotPosition( camera->getPivotPosition() - forward * Time::getLastDeltaTime() * 2.0f);
	}
	if (Input::keyboard.isKeyDown(GLFW_KEY_A)) {
		glm::vec3 right = camera->getRight();
		camera->setPivotPosition( camera->getPivotPosition() - right * Time::getLastDeltaTime() * 2.0f);
	}
	if (Input::keyboard.isKeyDown(GLFW_KEY_D)) {
		glm::vec3 right = camera->getRight();
		camera->setPivotPosition( camera->getPivotPosition() + right * Time::getLastDeltaTime() * 2.0f);
	}
	if (Input::keyboard.isKeyDown(GLFW_KEY_Q)) {
		camera->setPivotPosition( camera->getPivotPosition() - glm::vec3(0.0f, 1.0f, 0.0f) * Time::getLastDeltaTime() * 2.0f);
	}
	if (Input::keyboard.isKeyDown(GLFW_KEY_E)) {
		camera->setPivotPosition( camera->getPivotPosition() + glm::vec3(0.0f, 1.0f, 0.0f) * Time::getLastDeltaTime() * 2.0f);
	}

	renderTexture->bind();

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
	shader->setVec3("lightDir", glm::normalize(glm::vec3(-0.2f, -1.0f, -0.3f)));

	for (int i = 1; i < models.size(); i++) {
		Model* m = models[i];
		glm::mat4 modelMatrix = modelMatrices[i];
		shader->setMat4("model", glm::value_ptr(modelMatrix));
		m->Draw(*shader);
	}

    shader->stop();

	renderTexture->unbind();

	// Render screen quad
	quadShader->use();
	renderTexture->getColorAttachments()[0].texture->bind(0);
	shader->setInt("TextureColor", 0);
	ScreenQuad::draw();
	quadShader->stop();
}

void EngineRenderer::onDestroy() {
    delete shader;
	delete skyboxShader;
	delete quadShader;

	delete camera;
	delete skybox;

	for (Model* m : models) {
		delete m;
	}
	models.clear();
	modelMatrices.clear();
	delete danceAnimation;
	delete animator;

	delete renderTexture;
	ScreenQuad::destroy();
}
