#include "Entity.h"

#include "Rendering/Shaders.h"

// Objects have negative ids
int Entity::idCount = -1;

void Entity::SetShader(Shader shaderType) {

	this->shaderType = shaderType;
	
	switch (shaderType) {
		case Shader::PlainWhite:
			FragmentShader = &Shaders::PlainColor; break;
		case Shader::Normals:
			FragmentShader = &Shaders::Normals; break;
		case Shader::Textured:
			FragmentShader = &Shaders::Textured; break;
		case Shader::Grid:
			FragmentShader = &Shaders::Grid; break;
		case Shader::Debug:
			FragmentShader = &Shaders::Debug; break;
		default:
			FragmentShader = &Shaders::PlainColor; break;
	}
}
