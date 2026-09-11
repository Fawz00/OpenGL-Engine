#pragma once
#include <string>

// Interfce for texture classes
class I_Texture {
public:
	enum FilterMode {
		FilterNearest = GL_NEAREST,
		FilterLinear = GL_LINEAR
	};

	virtual ~I_Texture() = default;
	virtual void bind(unsigned int slot = 0) const = 0;
	static void unbind() {}
	
	virtual int* getDimension() const = 0;
	virtual int getSize() const = 0;
	virtual unsigned int getId() const = 0;
	virtual std::string getPath() const = 0;
};