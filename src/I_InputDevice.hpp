#pragma once

class I_InputDevice {
public:
	virtual ~I_InputDevice() = default;

	virtual void init() = 0;
	virtual void update() = 0;
	virtual void destroy() = 0;
};