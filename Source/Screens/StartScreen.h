#pragma once
#include "Buffers/VAO.h"
#include "Buffers/VBO.h"
#include "Buffers/EBO.h"
#include "Texture/Texture.h"
#include "Shader/Shader.h"
#include "Audio/AudioManager.h"
#include <vector>

using namespace std;

/*
* a screen displaying start menu
*/
class StartScreen {
private:
    //rectangle vertices
    vector<GLfloat> vertices = {
        1.0f,  1.0f, 0.0f,  1.0f, 1.0f, // top right
        1.0f, -1.0f, 0.0f,  1.0f, 0.0f, // bottom right
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, // bottom left
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f  // top left 
    };
    vector<GLuint> indices = {  
        0, 1, 3,   
        1, 2, 3    
    };
	VAO vao = VAO();
	VBO vbo = VBO(vertices.data(), sizeof(GLfloat) * vertices.size(), GL_STATIC_DRAW);
    EBO ebo = EBO(indices.data(), sizeof(GLuint) * indices.size(), GL_STATIC_DRAW);
    Shader shader = Shader("Resources/Shaders/2d_component.vert", "Resources/Shaders/2d_component.frag");
    Texture texture = Texture("Resources/Textures/menu_background.png", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGBA, GL_UNSIGNED_BYTE);
public:
    /*
    * calls initial setups for the screen, often links buffers attributes
    */
    StartScreen() {
        vao.bind();
        vao.link_attrib(vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (void*)0);
        vao.link_attrib(vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (void*)(3 * sizeof(GLfloat)));
        texture.set_unit(shader, "texture1", 0);
        texture.set_unit(shader, "use_texture", GL_TRUE);
        music::moog_city2.play();
    };

    /*
    * draws the start menu scene. this must be called every frame
    */
    void draw() {
        if (music::moog_city2.is_playing()) {
            music::moog_city2.update_buffer_stream();
        }
        shader.activate();
        texture.activate(GL_TEXTURE0);
        texture.bind();
        vao.bind();
        ebo.bind();
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    };
};