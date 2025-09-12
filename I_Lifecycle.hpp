#pragma once

class I_Lifecycle {
	public:
		virtual ~I_Lifecycle() = default;

		virtual void onEnable() = 0;
		virtual void onDisable() = 0;
		virtual void onInit() = 0;
		virtual void onUpdate(float deltaTime) = 0;
		virtual void onFixedUpdate(float fixedDeltaTime) = 0;
		virtual void onLateUpdate(float deltaTime) = 0;
		virtual void onDestroy() = 0;
};