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

#include "config.h"

int s_render_legend(const s_stft_t *);
int s_render_stft(const s_stft_t *);

/*!
 * This function creates a new OpenGL window and renders our output inside it.
 *
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_render_loop(const s_stft_t *stft)
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
		glClear(GL_COLOR_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-0.5f, (width - 1) + 0.5f,
			(height - 1) + 0.5f, -0.5f, 0.0f, 1.0f);

		// Render the frame / legend around the output.

		r = s_render_legend(stft);

		if(r < 0)
		{
			glfwTerminate();
			return r;
		}

		// Render the actual graphical STFT output.

		r = s_render_stft(stft);

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

int s_render_legend(const s_stft_t *stft)
{
	return 0;
}

int s_render_stft(const s_stft_t *stft)
{
	return 0;
}
