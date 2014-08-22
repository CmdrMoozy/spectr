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

#include "glinit.h"

#include <errno.h>

#include <GLFW/glfw3.h>

#include "config.h"

/*!
 * This is a utility function which initializes OpenGL in a way that it's ready
 * to render 2D graphics. We then call the given user-supplied function,
 * passing it the framebuffer width, height, and the given STFT instance, to
 * do the actual rendering.
 *
 * \param fptr The function to call after initialization to do the rendering.
 * \param stft The STFT instance to pass to the given function.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_init_gl(int (*fptr)(int, int, const s_stft_t *), const s_stft_t *stft)
{
	int r;
	GLFWwindow *window;

	int width;
	int height;

	if(!glfwInit())
		return -EINVAL;

	window = glfwCreateWindow(S_WINDOW_W, S_WINDOW_H,
		"Spectr", NULL, NULL);

	if(!window)
	{
		glfwTerminate();
		return -EINVAL;
	}

	glfwMakeContextCurrent(window);

	while(!glfwWindowShouldClose(window))
	{
		// Initialize the GL viewport.

		glfwGetFramebufferSize(window, &width, &height);

		glViewport(0, 0, width, height);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);



		// Call the user-provided rendering function.

		r = fptr(width, height, stft);

		if(r < 0)
		{
			glfwTerminate();
			return r;
		}

		// End GL rendering, swap the buffer, and poll for events.

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}
