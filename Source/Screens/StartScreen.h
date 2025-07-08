#pragma once
#include "Buffers/VAO.h"
#include "Buffers/VBO.h"
#include "Buffers/EBO.h"
#include "Texture/Texture.h"
#include "Shader/ShaderManager.h"
#include "Entity/Geometries.h"
#include "Audio/AudioManager.h"
#include <vector>

using namespace std;

/*
* a screen displaying start menu
*/
class StartScreen {
private:
    ShaderManager& sm;
  
	VAO vao = VAO();
	VBO vbo = VBO(quad_vertices.data(), sizeof(vertex_2d) * quad_vertices.size(), GL_STATIC_DRAW);
    Texture texture = Texture("Resources/Textures/menu_background.png", GL_TEXTURE0, GL_TEXTURE_2D, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
public:
    /*
    * calls initial setups for the screen, often links buffers attributes
    */
    StartScreen() : sm(ShaderManager::get_instance()) {
        vao.bind();
        vao.link_attrib(vbo, 0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)0);
        vao.link_attrib(vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)(2 * sizeof(float)));
        sm.HUD_shader.activate();
        sm.HUD_shader.set_uniform_1i("texture1", 0);
        sm.HUD_shader.set_uniform_1i("use_texture", GL_TRUE);
        music::moog_city2.play();
    };

    /*
    * draws the start menu scene. this must be called every frame
    */
    void draw() {
        if (music::moog_city2.is_playing()) {
            music::moog_city2.update_buffer_stream();
        }
        glDisable(GL_DEPTH_TEST);
        sm.HUD_shader.activate();
        texture.activate();
        texture.bind();
        vao.bind();
        glDrawArrays(GL_TRIANGLES, 0, quad_vertices.size());
    };
};