module;

#include <bgfx/bgfx.h>

export module Entity;

export class Entity {

public:

	bgfx::VertexBufferHandle vertexBuffer;
	bgfx::IndexBufferHandle indexBuffer;

	Entity() {
		// Take in vertex + indice buffer and whatever else

	}


};