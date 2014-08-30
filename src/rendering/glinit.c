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

#include "config.h"

int s_init_program();
int s_set_uniforms();
int s_init_vbo(s_vbo_t *, size_t);
int s_init_vao();

/*!
 * \brief This is the source code for our vertex shader.
 *
 * We load this shader into our program when initializing it, and then set its
 * resolution uniform based upon the size of the window we're doing 2D
 * rendering on.
 */
static const GLchar *s_vertex_shader_src = {
	"#version 440\n"
	"in vec3 position;\n"
	"uniform vec2 resolution;\n"
	"void main()\n"
	"{\n"
		"\tvec2 zeroToOne = vec2(position[0], position[1]) / resolution;\n"
		"\tvec2 zeroToTwo = zeroToOne * 2.0;\n"
		"\tvec2 clipSpace = zeroToTwo - 1.0;\n"
		"\tgl_Position = vec4(clipSpace * vec2(1.0, -1.0), position[2], 1.0);\n"
	"}\n"
};

/*!
 * \brief This is the source code for our fragment shader.
 *
 * We load this shader into our program when initializing it, and then set its
 * uniform based upon what color we want to use for rendering.
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
 * \brief Stores our OpenGL vertex array object.
 */
static GLuint s_vao = 0;

/*!
 * This is a utility function which initializes OpenGL in a way that it's ready
 * to render 2D graphics. We then call the given user-supplied function,
 * passing it the given STFT instance, to do the actual rendering.
 *
 * NOTE: The projection we initialize is such that the origin (0,0) is in the
 * top-left corner, and the "largest" vertex that is on-screen will be
 * (width, height), in the bottom-right corner.
 *
 * \param fptr The function to call after initialization to do the rendering.
 * \param vbo The list of s_vbo_t objects to initialize.
 * \param vbol The number of objects in the VBO list.
 * \param stft The STFT instance to pass to the given function.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_init_gl(int (*fptr)(const s_stft_t *),
	s_vbo_t *vbo, size_t vbol, const s_stft_t *stft)
{
	int r;
	GLFWwindow *window;

	if(!glfwInit())
		return -EINVAL;

	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

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

	r = s_init_vbo(vbo, vbol);

	if(r < 0)
	{
		glfwTerminate();
		return -EINVAL;
	}

	r = s_init_vao();

	if(r < 0)
	{
		glfwTerminate();
		return -EINVAL;
	}

	while(!glfwWindowShouldClose(window))
	{
		// Initialize the GL viewport.

		glViewport(0, 0, S_WINDOW_W, S_WINDOW_H);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(s_program);

		r = s_set_uniforms();

		if(r < 0)
		{
			glfwTerminate();
			return r;
		}

		// Call the user-provided rendering function.

		r = fptr(stft);

		if(r < 0)
		{
			glfwTerminate();
			return r;
		}

		// End GL rendering, swap the buffer, and poll for events.

		glUseProgram(0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}

/*!
 * This function sets our rendering color. Note that s_init_gl MUST have been
 * called first. Note that each component must be in the range [0, 1].
 *
 * \param r The red component.
 * \param g The green component.
 * \param b The blue component.
 * \param a The alpha component.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_gl_color(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	GLint colorVec;
	GLfloat color[4] = {r, g, b, a};

	colorVec = glGetUniformLocation(s_program, "color");

	if(colorVec == -1)
		return -EINVAL;

	glUniform4fv(colorVec, 1, color);

	return 0;
}

/*!
 * This function initializes the OpenGL program we will link our shaders into
 * for rendering our spectrogram.
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
 * This function sets the uniforms used in our various shaders to the proper
 * default values so we can start rendering in 2D.
 *
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_set_uniforms()
{
	int r;
	GLint resu;
	GLfloat res[] = { S_WINDOW_W, S_WINDOW_H };

	// Set our framebuffer resolution in our vertex shader.

	resu = glGetUniformLocation(s_program, "resolution");

	if(resu == -1)
		return -EINVAL;

	glUniform2fv(resu, 1, res);

	// Set our rendering color uniform.

	r = s_gl_color(1.0f, 1.0f, 1.0f, 1.0f);

	if(r < 0)
		return r;

	// Done!

	return 0;
}

/*!
 * This function initializes all of the vertex buffer objects in the given list
 * of s_vbo_t structures. This function sets the "obj" field in each structure,
 * from glGenBuffers.
 *
 * \param o The list of s_vbo_t structures to initialize.
 * \param l The number of VBO's in the given list.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_init_vbo(s_vbo_t *o, size_t l)
{
	size_t i;

	for(i = 0; i < l; ++i)
	{
		glGenBuffers(1, &(o->obj));

		glBindBuffer(GL_ARRAY_BUFFER, o->obj);
		glBufferData(GL_ARRAY_BUFFER, o->length * sizeof(GLfloat),
			o->data, o->usage);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	return 0;
}

/*!
 * This function initializes our vertex array object.
 *
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_init_vao()
{
	glGenVertexArrays(1, &s_vao);
	glBindVertexArray(s_vao);

	return 0;
}
