#pragma once

#include"Logger.h"
#include"GuiRenderer.h"

#include<iostream>
#include<format>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

//Simple GL display backend. Takes in pixel data from emu and renders as a textured quad

namespace GLRenderer
{
	struct vec3
	{
		float x;
		float y;
		float z;
	};

	struct vec2
	{
		float x;
		float y;
	};

	struct Vertex
	{
		vec3 position;
		vec3 uv;
	};
}

class Display
{
public:
	Display(int scaleFactor);
	~Display();

	bool getShouldClose();
	void draw();
	void update(void* newData);	//unsafe but size is known :)

	bool getPressed(unsigned int key);
	bool getLeftMouseClick();
	void getCursorPos(int& x, int& y);
private:
	GLFWwindow* m_window;

	GLuint m_VBO = 0, m_VAO = 0, m_program = 0;
	GLuint m_texHandle = 0;
	double m_lastTime = 0;
};