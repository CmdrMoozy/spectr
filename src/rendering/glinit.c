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
#include <stdlib.h>

#ifdef SPECTR_DEBUG
	#include <stdio.h>
#endif

#include "config.h"

int s_init_program();
int s_set_uniforms();
int s_init_vbo(s_vbo_t *, size_t);

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

	"varying float magnitude;\n"

	"void main()\n"
	"{\n"
		"\tmagnitude = position[2];\n"

		"\tvec2 pixelrnd = vec2(position[0], position[1]) + 0.5;\n"
		"\tvec2 zeroToOne = pixelrnd / resolution;\n"
		"\tvec2 zeroToTwo = zeroToOne * 2.0;\n"
		"\tvec2 clipSpace = zeroToTwo - 1.0;\n"
		"\tgl_Position = vec4(clipSpace * vec2(1.0, -1.0), 0.0, 1.0);\n"
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

	"uniform float maxMagnitude;\n"
	"varying float magnitude;\n"

	"void main()\n"
	"{\n"
		"\tif(magnitude < 0.0)\n"
		"\t{\n"
			"\t\tgl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
		"\t}\n"
		"\telse\n"
		"\t{\n"
			"\t\tvec4 color;\n"

			"\t\tvec4 black = vec4(0.0, 0.0, 0.0, 1.0);\n"
			"\t\tvec4 blue = vec4(0.0, 0.0, 0.25, 1.0);\n"
			"\t\tvec4 purple = vec4(0.5, 0.0, 0.5, 1.0);\n"
			"\t\tvec4 red = vec4(1.0, 0.0, 0.0, 1.0);\n"
			"\t\tvec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);\n"
			"\t\tvec4 white = vec4(1.0, 1.0, 1.0, 1.0);\n"

			"\t\tfloat step1 = 0.0;\n"
			"\t\tfloat step2 = maxMagnitude * 0.2;\n"
			"\t\tfloat step3 = maxMagnitude * 0.4;\n"
			"\t\tfloat step4 = maxMagnitude * 0.6;\n"
			"\t\tfloat step5 = maxMagnitude * 0.8;\n"
			"\t\tfloat step6 = maxMagnitude;\n"

			"\t\tcolor = mix(black, blue, "
				"smoothstep(step1, step2, magnitude));\n"
			"\t\tcolor = mix(color, purple, "
				"smoothstep(step2, step3, magnitude));\n"
			"\t\tcolor = mix(color, red, "
				"smoothstep(step3, step4, magnitude));\n"
			"\t\tcolor = mix(color, yellow, "
				"smoothstep(step4, step5, magnitude));\n"
			"\t\tcolor = mix(color, white, "
				"smoothstep(step5, step6, magnitude));\n"

			"\t\tgl_FragColor = color;\n"
		"\t}\n"
	"}\n"
};

/*!
 * \brief Stores our OpenGL program, from glCreateProgram().
 */
static GLuint s_program = 0;

/*!
 * \brief Stores our OpenGL vertex array object.
 */
static GLuint *s_vao;

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
int s_init_gl(int (*fptr)(const s_stft_t *, GLuint *),
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

		r = fptr(stft, s_vao);

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
 * This function sets the maximum magnitude value our fragment shader will
 * account for. This determines how we will color points with a non-negative
 * Z component.
 *
 * \param m The new maximum magnitude value.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_set_max_magnitude(GLfloat m)
{
	GLint uniform;

	uniform = glGetUniformLocation(s_program, "maxMagnitude");

	if(uniform == -1)
		return 0;

	glUniform1f(uniform, m);

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

#ifdef SPECTR_DEBUG
	GLchar buf[8192];
	GLsizei bufl;
#endif

	// Initialize our vertex shader.

	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &s_vertex_shader_src, NULL);
	glCompileShader(vertexShader);

	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);

	if(status == GL_FALSE)
	{
#ifdef SPECTR_DEBUG
		glGetShaderInfoLog(vertexShader, 8192, &bufl, buf);
		printf("%s\n", buf);
#endif

		return -EINVAL;
	}

	// Initialize our fragment shader.

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &s_fragment_shader_src, NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);

	if(status == GL_FALSE)
	{
#ifdef SPECTR_DEBUG
		glGetShaderInfoLog(fragmentShader, 8192, &bufl, buf);
		printf("%s\n", buf);
#endif

		return -EINVAL;
	}

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

	// Set some default maximum magnitude.

	r = s_set_max_magnitude(0.0f);

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
 * NOTE: This function also initializes a VAO for each VBO, keeping track of
 * its rendering state.
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
		glGenBuffers(1, &(o[i].obj));

		glBindBuffer(GL_ARRAY_BUFFER, o[i].obj);
		glBufferData(GL_ARRAY_BUFFER, o[i].length * sizeof(GLfloat),
			o[i].data, o[i].usage);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	s_vao = calloc(l, sizeof(GLuint));

	if(s_vao == NULL)
		return -ENOMEM;

	for(i = 0; i < l; ++i)
	{
		glGenVertexArrays(1, &(s_vao[i]));
		glBindVertexArray(s_vao[i]);

		glBindBuffer(GL_ARRAY_BUFFER, o[i].obj);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}

	return 0;
}
