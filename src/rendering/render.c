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
void s_set_spectrogram_vec3(GLfloat *, size_t, uint32_t *,
	size_t, size_t, GLfloat, GLfloat, GLfloat);
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
		S_VIEW_X_MIN - S_SPEC_LGND_TICK_SIZE, S_VIEW_Y_MIN, -1.0f,
		S_VIEW_X_MAX, S_VIEW_Y_MIN, -1.0f,

		S_VIEW_X_MAX, S_VIEW_Y_MIN, -1.0f,
		S_VIEW_X_MAX, S_VIEW_Y_MAX + S_SPEC_LGND_TICK_SIZE, -1.0f,

		S_VIEW_X_MAX, S_VIEW_Y_MAX, -1.0f,
		S_VIEW_X_MIN - S_SPEC_LGND_TICK_SIZE, S_VIEW_Y_MAX, -1.0f,

		S_VIEW_X_MIN, S_VIEW_Y_MAX + S_SPEC_LGND_TICK_SIZE, -1.0f,
		S_VIEW_X_MIN, S_VIEW_Y_MIN, -1.0f
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
 * \param avg The number of values being averaged in each array cell.
 * \param ix The X index of the pixel being set.
 * \param iy The Y index of the pixel being set.
 * \param x The X-component of the 3-vector.
 * \param y The Y-component of the 3-vector.
 * \param z The Z-component of the 3-vector.
 */
void s_set_spectrogram_vec3(GLfloat *arr, size_t arrw,
	uint32_t *avg, size_t ix, size_t iy, GLfloat x, GLfloat y, GLfloat z)
{
	size_t aidx;
	size_t idx;

	aidx = arrw * iy + ix;
	idx = aidx * 3;

	arr[idx] = x;
	arr[idx + 1] = y;

	arr[idx + 2] = (arr[idx + 2] * (avg[aidx] / (avg[aidx] + 1.0))) +
		(z / (avg[aidx] + 1.0));;

	++avg[aidx];
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

	// Set our rounding mode.

	r = fesetround(FE_TONEAREST);

	if(r != 0)
		return -EINVAL;

	/*
	 * Allocate memory for the points. Note that spectrogram points are
	 * 3-vectors of (time, frequency, magnitude).
	 */

	vbo->data = (GLfloat *)
		calloc(S_VIEW_W * S_VIEW_W * 3, sizeof(GLfloat));

	if(vbo->data == NULL)
		return -ENOMEM;

	vbo->length = S_VIEW_W * S_VIEW_H * 3;
	vbo->usage = GL_STATIC_DRAW;
	vbo->mode = GL_POINTS;

	/*
	 * Allocate an array to keep track of average counts, so we can compute
	 * the average of DFT results that all fall in the same pixel.
	 */

	uint32_t *averageCount = (uint32_t *)
		calloc(S_VIEW_W * S_VIEW_H, sizeof(uint32_t));

	if(averageCount == NULL)
	{
		free(vbo->data);
		return -ENOMEM;
	}

	// Compute the value of each point we'll render.

	for(stfti = 0; stfti < stft->length; ++stfti)
	{
		for(dfti = 1; dfti < stft->dfts[stfti]->length / 2; ++dfti)
		{
			/*
			 * Scale the X value to the range of pixels in our
			 * spectrogram viewport. For the Y value, we can just
			 * shift it up to be inside the spectrogram viewport,
			 * since our viewport's rows are mapped 1-1 to DFT
			 * frequency bins.
			 */

			x = s_scale(0, stft->length, S_VIEW_X_MIN + 1,
				S_VIEW_X_MAX - 1, stfti);

			y = (double) (dfti + S_VIEW_Y_MIN);

			// Round the X value to the nearest integer pixel.

			x = rint(x);

			// Clip the X value to be inside our viewport.

			x = fmax(x, S_VIEW_X_MIN + 1);
			x = fmin(x, S_VIEW_X_MAX - 1);

			/*
			 * Compute the Z value of this DFT result. We take the
			 * base-10 logarithm of the value, since e.g. decibels
			 * are a logarithmic scale, so our output will map more
			 * directly to e.g. human hearing.
			 */

			z = s_magnitude(&(stft->dfts[stfti]->dft[dfti]));
			z = log10(z);

			// If we got a bogus Z value, just skip it.

			if(isinf(z) || isnan(z))
				continue;

			// Set the value in our list.

			s_set_spectrogram_vec3(
				vbo->data,
				S_VIEW_W,
				averageCount,

				/*
				 * The X and Y positions *in the array* need to
				 * be 0-indexed, not S_VIEW_Y_MIN + 1 indexed.
				 */
				x - ((double) S_VIEW_X_MIN) - 1.0,
				y - ((double) S_VIEW_Y_MIN) - 1.0,

				x,

				/*
				 * Shift our Y value to be inside the
				 * spectrogram viewport area. We are reversing
				 * it, since in our OpenGL projection pixel
				 * (0,0) is at the top left of the window,
				 * instead of the bottom left.
				 */
				((double) S_VIEW_Y_MAX) - y +
					((double) S_VIEW_Y_MIN),

				z
			);
		}
	}

	// Get the minimum and maximum Z values.

	for(idx = 2; idx < vbo->length; idx += 3)
	{
		/*
		 * If this value is still zero (i.e., it was never set in the
		 * loop above), then don't include it in the range computation.
		 */

		if(fabs(vbo->data[idx]) < 0.0001)
			continue;

		minz = fmin(minz, vbo->data[idx]);
		maxz = fmax(maxz, vbo->data[idx]);
	}

	/*
	 * Shift the values down so they are in the range [0, maxz]. This makes
	 * it easier to color the pixels. See our fragment shader in glinit.c
	 * for more details.
	 */

	for(idx = 2; idx < vbo->length; idx += 3)
		vbo->data[idx] = fmax(vbo->data[idx] - minz, 0.0f);

	maxz -= minz;
	minz = 0.0;

	/*
	 * Set the variable containing our maximum magnitude. The fragment
	 * shader's uniform will be set to this value later, in the rendering
	 * loop, since we can't set uniform values until glUseProgram() is
	 * called.
	 */

	s_max_magnitude = maxz;

	// Done!

	free(averageCount);

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
