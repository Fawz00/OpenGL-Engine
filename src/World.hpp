#pragma once

#include <vector>
#include <string>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "Actor.hpp"

class World {
public:
	World();
	~World();

	// No copy
	World(const World&) = delete;
	World& operator=(const World&) = delete;

	// Move allowed
	World(World&&) noexcept = default;
	World& operator=(World&&) noexcept = default;

	void addActor(std::unique_ptr<Actor> actor);

	// Serialization methods
	std::string toJson() const;
	static World fromJson(const std::string& jsonStr);

private:
	std::string name;
	std::vector<std::unique_ptr<Actor>> actors;

	void drawScene() const; // Draw all actors in the world
	friend class EngineRenderer;
};