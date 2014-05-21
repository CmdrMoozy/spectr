/*
 * spectr - A very simple spectrum analyzer for audio files.
 * Copyright (C) 2014 Axel Rasmussen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "render.h"

#include <errno.h>

#include <GLFW/glfw3.h>

#define S_WINDOW_WIDTH 800
#define S_WINDOW_HEIGHT 250

/*!
 * This function creates a new OpenGL window and renders our output inside it.
 *
 * \return 0 on success, or an error number if something goes wrong.
 */
int render_loop()
{
	GLFWwindow *window;

	if(!glfwInit())
		return -EINVAL;

	window = glfwCreateWindow(S_WINDOW_WIDTH, S_WINDOW_HEIGHT,
		"Spectr", NULL, NULL);

	if(!window)
	{
		glfwTerminate();
		return -EINVAL;
	}

	glfwMakeContextCurrent(window);

	while(!glfwWindowShouldClose(window))
	{


		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}
