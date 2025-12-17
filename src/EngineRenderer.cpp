#include "EngineRenderer.hpp"

void EngineRenderer::onInit() {
	ScreenQuad::init();

	renderTexture = new RenderTexture(Window::width(), Window::height());
	renderTexture->addColorAttachment(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, Texture2D::FilterLinear, false);
	renderTexture->useDepthRBO();

	shadowMap = new RenderTexture(720, 720);
	shadowMap->useDepthTexture(Texture2D::FilterNearest);

	// Models and their matrices
	models.push_back(new Model("Resources/engine/models/cube.obj"));
	glm::mat4 mat0 = glm::mat4(1.0f);
	modelMatrices.push_back(mat0);

	models.push_back(new Model("Resources/engine/models/backpack/backpack.obj"));
	glm::mat4 mat2 = glm::mat4(1.0f);
	mat2 = glm::translate(mat2, glm::vec3(0.0f, 1.0f, -1.0f));
	mat2 = glm::scale(mat2, glm::vec3(0.3f));
	modelMatrices.push_back(mat2);

	models.push_back(new Model("Resources/engine/models/tropical.fbx"));
	glm::mat4 mat1 = glm::mat4(1.0f);
	mat1 = glm::translate(mat1, glm::vec3(-612.0f, -19.0f, 465.0f));
	modelMatrices.push_back(mat1);

	models.push_back(new Model("Resources/engine/models/char13.fbx", Model::SKINNED));
	glm::mat4 mat_4 = glm::mat4(1.0f);
	mat_4 = glm::translate(mat_4, glm::vec3(2.0f, 0.0f, -2.0f));
	mat_4 = glm::scale(mat_4, glm::vec3(0.01f)); // FBX model is huge
	modelMatrices.push_back(mat_4);

	models.push_back(new Model("Resources/engine/models/Sponza/Sponza.gltf"));
	glm::mat4 mat5 = glm::mat4(1.0f);
	mat5 = glm::translate(mat5, glm::vec3(0.0f, 0.0f, 0.0f));
	mat5 = glm::scale(mat5, glm::vec3(0.01f)); // Sponza model is huge
	modelMatrices.push_back(mat5);

	mixamoRetarget = new AnimationRetarget("Resources/engine/models/mixamo_retarget.json", AnimationRetarget::Mode::Flexible);
	danceAnimation = new Animation("Resources/engine/models/Happy.fbx", models[3], mixamoRetarget);
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
	skybox = new TextureCubeMap(skyboxFaces, TextureCubeMap::Skybox);

	skyboxShader
		.attachShader(ShaderType::VertexShader, "Resources/engine/shaders/skybox_vertex.glsl")
		.attachShader(ShaderType::FragmentShader, "Resources/engine/shaders/skybox_fragment.glsl")
		.link();

	shader
		.attachShader(ShaderType::VertexShader, "Resources/engine/shaders/mesh_vertex.glsl", {"SKINNED"})
		.attachShader(ShaderType::FragmentShader, "Resources/engine/shaders/mesh_fragment.glsl")
		.link();

	quadShader
		.attachShader(ShaderType::VertexShader, "Resources/engine/shaders/quad_vertex.glsl")
		.attachShader(ShaderType::FragmentShader, "Resources/engine/shaders/quad_fragment.glsl")
		.link();

	shadowShader
		.attachShader(ShaderType::VertexShader, "Resources/engine/shaders/mesh_vertex.glsl", {"SKINNED"})
		.attachShader(ShaderType::FragmentShader, "Resources/engine/shaders/mesh_simple_fragment.glsl")
		.link();

	camera = new Camera();
	camera->setPivotDistance(0.01f); // FPS style camera
	camera->setRotation(0.0f, 0.0f, 0.0f);
	camera->setPerspective( 70.0f, 0.1f, 1000.0f);
	camera->setAspectRatio(Window::width(), Window::height());
	camera->setRotationMode(Camera::ROTATION_LIMITED);

	lightCamera = new Camera();
	lightCamera->setPivotDistance(1.0f);
	lightCamera->setRotation(-76.0f, -27.0f, 0.0f);
	lightCamera->setOrthographic(40.0f, -40.0f, 40.0f);
	lightCamera->setAspectRatio(1, 1);
	lightCamera->setRotationMode(Camera::ROTATION_FREE);

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

	lightCamera->setPivotPosition(camera->getPivotPosition());

	// SHADOW PASS
	shadowMap->bind();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	shadowShader.bind();
	float lightProjection[16];
	lightCamera->getProjectionMatrix(lightProjection);
	shadowShader.setMat4("projection", lightProjection);
	float lightView[16];
	lightCamera->getViewMatrix(lightView);
	shadowShader.setMat4("view", lightView);
	for (int i = 1; i < models.size(); i++) {
		Model* m = models[i];
		glm::mat4 modelMatrix = modelMatrices[i];
		shadowShader.setMat4("model", glm::value_ptr(modelMatrix));
		m->draw(shadowShader);
	}
	shadowShader.unbind();
	shadowMap->unbind();



	// MAIN RENDER PASS
	renderTexture->bind();

	// Render skybox first
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_CULL_FACE);

	skyboxShader.bind();

	float viewNoTranslation[16];
	camera->getViewMatrix(viewNoTranslation);
	viewNoTranslation[12] = 0.0f;
	viewNoTranslation[13] = 0.0f;
	viewNoTranslation[14] = 0.0f;
	skyboxShader.setMat4("view", viewNoTranslation);
	float projection[16];
	camera->getProjectionMatrix(projection);
	skyboxShader.setMat4("projection", projection);

	skybox->bind(0);
	skyboxShader.setInt("skybox", 0);

	models[0]->draw(skyboxShader);
	skyboxShader.unbind();



	// Render rest of scene
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	shader.bind();

	shader.setFloat("uTime", Time::getTime());

	float cmProjection[16];
	camera->getProjectionMatrix(cmProjection);
	shader.setMat4("projection", cmProjection);

	float view[16];
	camera->getViewMatrix(view);
	shader.setMat4("view", view);

	shader.setVec3("uViewPos", camera->getWorldPosition());
	shader.setVec3("uLightDir", glm::normalize(lightCamera->getForward()));

	shadowMap->getDepthTexture()->bind(8);
	shader.setInt("shadowMap", 8);

	float lightSpaceMatrix[16];
	lightCamera->getProjectionMatrix(lightSpaceMatrix);
	float lightViewMatrix[16];
	lightCamera->getViewMatrix(lightViewMatrix);
	glm::mat4 lightSpace = glm::make_mat4(lightSpaceMatrix) * glm::make_mat4(lightViewMatrix);
	shader.setMat4("lightSpaceMatrix", glm::value_ptr(lightSpace));

	for (int i = 1; i < models.size(); i++) {
		if ((models[i]->getMode() & Model::ModelMode::SKINNED) == Model::ModelMode::SKINNED) {
			animator->updateAnimation(shader);
		}

		Model* m = models[i];
		glm::mat4 modelMatrix = modelMatrices[i];
		shader.setMat4("model", glm::value_ptr(modelMatrix));
		m->draw(shader);
	}

    shader.unbind();
	renderTexture->unbind();

	// Render screen quad
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	// Clear screen
	glViewport(0, 0, Window::width(), Window::height());
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	quadShader.bind();
	renderTexture->getColorAttachments()[0].texture->bind(0);
	quadShader.setInt("TextureColor", 0);
	ScreenQuad::draw();
	quadShader.unbind();
}

void EngineRenderer::onDestroy() {
	delete camera;
	delete skybox;

	for (Model* m : models) {
		delete m;
	}
	models.clear();
	modelMatrices.clear();
	delete danceAnimation;
	delete mixamoRetarget;
	delete animator;

	delete renderTexture;
	delete shadowMap;
	ScreenQuad::destroy();
}
