#include "World.hpp"

World::World() {
	// Constructor implementation (if needed)
}

World::~World() {
	// Destructor implementation (if needed)
}

void World::addActor(std::unique_ptr<Actor> actor) {
	actors.push_back(std::move(actor));
}

// Convert Scene to JSON
std::string World::toJson() const {
    json j;
    j["name"] = name;
    j["actors"] = json::array();
    for (const auto& actor : actors) {
        j["actors"].push_back({
            {"filepath", actor.get()->getFilePath()},
            {"position", {actor.get()->getPosition().x, actor.get()->getPosition().y, actor.get()->getPosition().z}}
        });
    }
    return j.dump(4); // Pretty print with 4 spaces
}

// Load Scene from JSON
World World::fromJson(const std::string& jsonStr) {
    World scene;
    json j = json::parse(jsonStr);
    scene.name = j.value("name", "");
    for (const auto& m : j["actors"]) {
		std::string path = m.value("filepath", "");
        std::unique_ptr<Actor> actor = std::make_unique<Actor>(path);

		glm::vec3 pos = glm::vec3(0.0f);
        if (m.contains("position") && m["position"].is_array() && m["position"].size() == 3) {
            pos.x = m["position"][0].get<float>();
            pos.y = m["position"][1].get<float>();
            pos.z = m["position"][2].get<float>();
		}
        actor.get()->setPosition(pos);
        scene.actors.push_back(std::move(actor));
    }
    return scene;
}