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
#include <math.h>
#include <linux/limits.h>
#include <fenv.h>
#include <float.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <GLFW/glfw3.h>

#include "config.h"
#include "decoding/stat.h"
#include "rendering/glinit.h"
#include "util/complex.h"
#include "util/fonts.h"
#include "util/math.h"

int s_render_loop(const s_stft_t *, GLuint *);
void s_set_spectrogram_vec3(GLfloat *, size_t, size_t,
	size_t, GLfloat, GLfloat, GLfloat);
int s_alloc_stft_vbo(s_vbo_t *, const s_stft_t *);
int s_render_legend_frame(GLuint *);
int s_render_legend_labels(const s_stft_t *);
int s_render_stft(GLuint *);

/*!
 * \brief This list stores all of the VBO's we use for rendering.
 */
static s_vbo_t *s_vbo_list = NULL;

/*!
 * \brief This stores the length of s_vbo_list.
 */
static size_t s_vbo_list_length = 0;

/*!
 * \brief This stores our maximum DFT magnitude.
 */
static double s_max_magnitude = 0.0f;

/*!
 * This function starts our OpenGL rendering loop, to render the given STFT's
 * spectrogram.
 *
 * \param stft The STFT whose spectrogram should be rendered.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_render(const s_stft_t *stft)
{
	int ret = 0;
	int r;

	s_spectrogram_viewport view =
		s_get_spectrogram_viewport(S_WINDOW_W, S_WINDOW_H);

	// Allocate the list of VBO objects we'll use.

	s_vbo_list_length = 2;
	s_vbo_list = (s_vbo_t *) malloc(sizeof(s_vbo_t) * s_vbo_list_length);

	if(s_vbo_list == NULL)
	{
		ret = -ENOMEM;
		goto done;
	}

	// Set the values of the legend frame vertices.

	s_vbo_list[0].data = (GLfloat[24]) {
		view.xmin - S_SPEC_LGND_TICK_SIZE, view.ymin, -1.0f,
		view.xmax, view.ymin, -1.0f,

		view.xmax, view.ymin, -1.0f,
		view.xmax, view.ymax + S_SPEC_LGND_TICK_SIZE, -1.0f,

		view.xmax, view.ymax, -1.0f,
		view.xmin - S_SPEC_LGND_TICK_SIZE, view.ymax, -1.0f,

		view.xmin, view.ymax + S_SPEC_LGND_TICK_SIZE, -1.0f,
		view.xmin, view.ymin, -1.0f
	};

	s_vbo_list[0].length = 24;
	s_vbo_list[0].usage = GL_STATIC_DRAW;
	s_vbo_list[0].mode = GL_LINES;

	// Initialize the structure for the spectrogram itself.

	r = s_alloc_stft_vbo(&(s_vbo_list[1]), stft);

	if(r < 0)
	{
		ret = r;
		goto err_after_vbo_alloc;
	}

	// Initialize the GL context, and start the rendering loop.

	r = s_init_gl(s_render_loop, s_vbo_list, s_vbo_list_length, stft);

	// Clean up and return.

	free(s_vbo_list[1].data);
err_after_vbo_alloc:
	free(s_vbo_list);
done:
	return ret;
}

/*!
 * This function computes the spectrogram viewport we're using from the given
 * framebuffer width and height values (from glfwGetFramebufferSize()).
 *
 * The returned structure should be used to render our spectrogram, to avoid
 * scattering various "magic numbers" throughout the code.
 *
 * \return The spectrogram viewport for the given framebuffer.
 */
s_spectrogram_viewport s_get_spectrogram_viewport()
{
	s_spectrogram_viewport v;

	v.xmin = 75;
	v.ymin = 5;

	v.xmax = S_WINDOW_W - 5;
	v.ymax = S_WINDOW_H - 30;

	v.w = v.xmax - v.xmin - 1;
	v.h = v.ymax - v.ymin - 1;

	return v;
}

/*!
 * This function is passed to our OpenGL initialization function, and is called
 * during each render loop. See s_init_gl for details.
 *
 * \param stft The STFT whose spectrogram should be rendered.
 * \param vao The VAO which has been configured for each of our VBO's.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_render_loop(const s_stft_t *stft, GLuint *vao)
{
	int r;

	// Render the frame / legend around the output.

	r = s_render_legend_frame(vao);

	if(r < 0)
		return r;

	r = s_render_legend_labels(stft);

	if(r < 0)
		return r;

	// Render the actual graphical STFT output.

	r = s_render_stft(vao);

	if(r < 0)
		return r;

	// Done!

	return 0;
}

/*!
 * This is a small convenience function which sets the values of a 3-vector
 * at a given position in our list of spectrogram points.
 *
 * \param arr The list of spectrogram points.
 * \param arrw The width of a "row" in the array; the maximum y value, incl.
 * \param ix The X index of the pixel being set.
 * \param iy The Y index of the pixel being set.
 * \param x The X-component of the 3-vector.
 * \param y The Y-component of the 3-vector.
 * \param z The Z-component of the 3-vector.
 */
void s_set_spectrogram_vec3(GLfloat *arr, size_t arrw, size_t ix, size_t iy,
	GLfloat x, GLfloat y, GLfloat z)
{
	size_t idx = arrw * iy + ix;
	idx = idx * 3;

	arr[idx] = x;
	arr[idx + 1] = y;

	arr[idx + 2] = z;
}

/*!
 * This function allocates and computes the vertices for the buffer which will
 * render our spectrogram.
 *
 * \param vbo The VBO being populated with spectrogram vertices.
 * \param stft The STFT data being rendered.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_alloc_stft_vbo(s_vbo_t *vbo, const s_stft_t *stft)
{
	int r;

	size_t idx;
	size_t stfti;
	size_t dfti;

	double x;
	double y;
	double z;

	double minz = DBL_MAX;
	double maxz = 0.0;

	// Get the dimensions of the view we're rendering.

	s_spectrogram_viewport view =
		s_get_spectrogram_viewport(S_WINDOW_W, S_WINDOW_H);

	// Set our rounding mode.

	r = fesetround(FE_TONEAREST);

	if(r != 0)
		return -EINVAL;

	/*
	 * Allocate memory for the points. Note that spectrogram points are
	 * 3-vectors of (time, frequency, magnitude).
	 */

	vbo->data = (GLfloat *) calloc(view.w * view.h * 3, sizeof(GLfloat));

	if(vbo->data == NULL)
		return -ENOMEM;

	vbo->length = view.w * view.h * 3;
	vbo->usage = GL_STATIC_DRAW;
	vbo->mode = GL_POINTS;

	// Compute the value of each point we'll render.

	for(stfti = 0; stfti < stft->length; ++stfti)
	{
		for(dfti = 0; dfti < stft->dfts[stfti]->length / 2; ++dfti)
		{
			// Get the X and Y values in the right range.

			x = s_scale(0, stft->length, view.xmin + 1,
				view.xmax - 1, stfti);
			y = s_scale(0, stft->dfts[stfti]->length / 2 - 1,
				view.ymin + 1, view.ymax - 1, dfti);

			x = rint(x);
			y = rint(y);

			x = fmax(x, view.xmin + 1);
			x = fmin(x, view.xmax - 1);

			y = fmax(y, view.ymin + 1);
			y = fmin(y, view.ymax - 1);

			// Deal with the Z value.

			z = s_magnitude(&(stft->dfts[stfti]->dft[dfti]));
			z = log10(pow(z, 2.0));

			// Set the value in our list.

			s_set_spectrogram_vec3(
				vbo->data,
				view.w,
				x - (view.xmin + 1),
				y - (view.ymin + 1),
				x,
				y,
				z);
		}
	}

	// Get the minimum and maximum Z values.

	for(idx = 2; idx < vbo->length; idx += 3)
	{
		if(fabs(vbo->data[idx]) < 0.0001)
			continue;

		minz = fmin(minz, vbo->data[idx]);
		maxz = fmax(maxz, vbo->data[idx]);
	}

	// Shift the values down so they are in the range [0, maxz].

	for(idx = 2; idx < vbo->length; idx += 3)
	{
		if(fabs(vbo->data[idx]) < 0.0001)
			continue;

		vbo->data[idx] = vbo->data[idx] - minz;
	}

	maxz -= minz;
	minz = 0.0;

	// Set our maximum magnitude, so we can give it to the shader later.

	s_max_magnitude = maxz;

	// Done!

	return 0;
}

/*!
 * This function renders the spectrogram legend using OpenGL. This includes the
 * frame around the spectrogram, as well as the frequency and time labels for
 * the loaded track.
 *
 * \param vao The VAO which contains our legend frame VBO's state.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_render_legend_frame(GLuint *vao)
{
	glBindVertexArray(vao[0]);
	glDrawArrays(s_vbo_list[0].mode, 0, s_vbo_list[0].length / 3);

	return 0;
}

int s_render_legend_labels(const s_stft_t *stft)
{
	int r;
	FT_Library ft;
	char fontpath[PATH_MAX];
	FT_Face font;
	char fnstr[32];
	char durstr[32];

	int ret = 0;

	s_spectrogram_viewport view;

	// Initialize the FreeType library, and get the font we'll use.

	if(FT_Init_FreeType(&ft))
	{
		ret = -ELIBACC;
		goto done;
	}

	r = s_get_mono_font_path(fontpath, PATH_MAX);

	if(r < 0)
	{
		ret = r;
		goto err_after_lib_alloc;
	}

	if(FT_New_Face(ft, fontpath, 0, &font))
	{
		ret= -EIO;
		goto err_after_lib_alloc;
	}

	if(FT_Set_Pixel_Sizes(font, 0, 20))
	{
		ret = -ELIBACC;
		goto err_after_font_alloc;
	}

	// Get the viewport parameters we'll use to render.

	view = s_get_spectrogram_viewport();

	// Get the frequency and duration labels.

	r = s_audio_duration_str(fnstr, 32,
		&(stft->raw_stat), stft->raw_length);

	if(r < 0)
	{
		ret = r;
		goto err_after_font_alloc;
	}

	r = s_nyquist_frequency_str(durstr, 32, &(stft->raw_stat));

	if(r < 0)
	{
		ret = r;
		goto err_after_font_alloc;
	}



	// Done!

err_after_font_alloc:
	FT_Done_Face(font);
err_after_lib_alloc:
	FT_Done_FreeType(ft);
done:
	return ret;
}

/*!
 * This function renders our spectrogram by binding the given VAO which is
 * associated with its VBO and then drawing the points to the screen.
 *
 * \param vao The VAO containing our spectrogram's draw state information.
 * \return 0 on success, or an error number otherwise.
 */
int s_render_stft(GLuint *vao)
{
	int r;

	r = s_set_max_magnitude(s_max_magnitude);

	if(r < 0)
		return r;

	glBindVertexArray(vao[1]);
	glDrawArrays(s_vbo_list[1].mode, 0, s_vbo_list[1].length / 3);

	return 0;
}
