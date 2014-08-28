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
#include <GL/gl.h>

#include "config.h"
#include "rendering/glutils.h"

int s_init_program();
int s_set_uniforms();

/*!
 * \brief This is the source code for our vertex shader.
 *
 * We load this shader into our program when initializing it, and then set its uniform
 * based upon the type of projection we want to render.
 */
static const GLchar *s_vertex_shader_src = {
	"#version 440\n"
	"in vec4 position;\n"
	"uniform mat4 projection;\n"
	"void main()\n"
	"{\n"
		"\tgl_Position = projection * position;\n"
	"}\n"
};

/*!
 * \brief This is the source code for our fragment shader.
 *
 * We load this shader into our program when initializing it, and then set its uniform
 * based upon what color we want to use for rendering.
 */
static const GLchar *s_fragment_shader_src = {
	"#version 440\n"
	"uniform vec4 color;\n"
	"out vec4 outputColor;\n"
	"void main()\n"
	"{\n"
		"outputColor = color;\n"
	"}\n"
};

/*!
 * \brief Stores our OpenGL program, from glCreateProgram().
 */
static GLuint s_program = 0;

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

	r = s_init_program();

	if(r < 0)
	{
		glfwTerminate();
		return -EINVAL;
	}

	while(!glfwWindowShouldClose(window))
	{
		// Initialize the GL viewport.

		glfwGetFramebufferSize(window, &width, &height);

		r = s_set_uniforms(width, height);

		if(r < 0)
		{
			glfwTerminate();
			return r;
		}

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

/*!
 * This function initializes the OpenGL program we will link our shaders into for
 * rendering our spectrogram.
 *
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_init_program()
{
	GLint status;
	GLuint vertexShader;
	GLuint fragmentShader;

	// Initialize our vertex shader.

	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &s_vertex_shader_src, NULL);
	glCompileShader(vertexShader);

	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);

	if(status == GL_FALSE)
		return -EINVAL;

	// Initialize our fragment shader.

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &s_fragment_shader_src, NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);

	if(status == GL_FALSE)
		return -EINVAL;

	// Initialize the program.

	s_program = glCreateProgram();

	glAttachShader(s_program, vertexShader);
	glAttachShader(s_program, fragmentShader);

	glLinkProgram(s_program);

	glGetProgramiv(s_program, GL_LINK_STATUS, &status);

	if(status == GL_FALSE)
		return -EINVAL;

	// Detach the shaders, now that the program is linked.

	glDetachShader(s_program, vertexShader);
	glDetachShader(s_program, fragmentShader);

	// Done!

	return 0;
}

/*!
 * This function sets the uniforms used in our various shaders to the proper values for
 * our orthographic projection.
 *
 * \param width The framebuffer width.
 * \param height The framebuffer height.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_set_uniforms(int width, int height)
{
	GLint orthoMatrix;
	GLfloat matrix[16];

	GLint colorVec;
	GLfloat color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

	// Set our orthographic projection uniform.

	orthoMatrix = glGetUniformLocation(s_program, "projection");

	if(orthoMatrix == -1)
		return -EINVAL;

	s_ortho_matrix(matrix, -0.5f, ((GLfloat) (width - 1)) + 0.5f,
		((GLfloat) (height - 1)) + 0.5f, -0.5f, 0.0f, 1.0f);

	glUniformMatrix4fv(orthoMatrix, 1, GL_FALSE, matrix);

	// Set our rendering color uniform.

	colorVec = glGetUniformLocation(s_program, "color");

	if(colorVec == -1)
		return -EINVAL;

	glUniform4fv(colorVec, 1, color);

	// Done!

	return 0;
}
