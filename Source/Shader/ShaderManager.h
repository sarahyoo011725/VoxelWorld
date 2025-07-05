#pragma once
#include <Shader/Shader.h>

class ShaderManager {
private:
	ShaderManager() {};
	ShaderManager(const ShaderManager&) = delete;
	ShaderManager& operator = (const ShaderManager&) = delete;
public:
	static ShaderManager& get_instance() {
		static ShaderManager instance;
		return instance;
	}
	Shader default_shader = Shader("Resources/Shaders/default.vert", "Resources/Shaders/default.frag");
	Shader wave_shader = Shader("Resources/Shaders/wave.vert", "Resources/Shaders/wave.frag");
	Shader outline_shader = Shader("Resources/Shaders/outline.vert", "Resources/Shaders/outline.frag");
	Shader HUD_shader = Shader("Resources/Shaders/2d_component.vert", "Resources/Shaders/2d_component.frag");
};