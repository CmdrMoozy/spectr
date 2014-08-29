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
#include <linux/limits.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <GLFW/glfw3.h>

#include "config.h"
#include "decoding/stat.h"
#include "rendering/glinit.h"
#include "util/fonts.h"

int s_render_loop(const s_stft_t *);
s_spectrogram_viewport s_get_spectrogram_viewport();
void s_gl_legend_color();
int s_render_legend_frame();
int s_render_legend_labels(const s_stft_t *);
int s_render_stft(const s_stft_t *);

/*!
 * \brief This list stores all of the VBO's we use for rendering.
 */
static s_vbo_t *s_vbo_list = NULL;

/*!
 * \brief This stores the length of s_vbo_list.
 */
static size_t s_vbo_list_length = 0;

/*!
 * This function starts our OpenGL rendering loop, to render the given STFT's
 * spectrogram.
 *
 * \param stft The STFT whose spectrogram should be rendered.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_render(const s_stft_t *stft)
{
	int r;
	s_spectrogram_viewport view =
		s_get_spectrogram_viewport(S_WINDOW_W, S_WINDOW_H);

	int vieww = view.xmax - view.xmin - 1;
	int viewh = view.ymax - view.ymin - 1;
	int viewa = vieww * viewh;

	// Allocate the list of VBO objects we'll use.

	s_vbo_list_length = 2;
	s_vbo_list = (s_vbo_t *) malloc(sizeof(s_vbo_t) * s_vbo_list_length);

	if(s_vbo_list == NULL)
		return -ENOMEM;

	// Set the values of the legend frame vertices.

	s_vbo_list[0].data = (GLfloat[16]) {
		view.xmin - S_SPEC_LGND_TICK_SIZE, view.ymin,
		view.xmax, view.ymin,

		view.xmax, view.ymin,
		view.xmax, view.ymax + S_SPEC_LGND_TICK_SIZE,

		view.xmax, view.ymax,
		view.xmin - S_SPEC_LGND_TICK_SIZE, view.ymax,

		view.xmin, view.ymax + S_SPEC_LGND_TICK_SIZE,
		view.xmin, view.ymin
	};

	s_vbo_list[0].length = 16;
	s_vbo_list[0].usage = GL_STATIC_DRAW;
	s_vbo_list[0].mode = GL_LINES;

	// Initialize the structure for the spectrogram itself.

	s_vbo_list[1].data = (GLfloat *) malloc(sizeof(GLfloat) * viewa * 2);

	if(s_vbo_list[1].data == NULL)
	{
		free(s_vbo_list);
		s_vbo_list = NULL;

		return -ENOMEM;
	}

	s_vbo_list[1].length = viewa * 2;
	s_vbo_list[1].usage = GL_DYNAMIC_DRAW;
	s_vbo_list[1].mode = GL_POINTS;

	// Initialize the GL context, and start the rendering loop.

	r = s_init_gl(s_render_loop, s_vbo_list, s_vbo_list_length, stft);

	// Free our VBO's.

	free(s_vbo_list[1].data);
	free(s_vbo_list);
	s_vbo_list = NULL;

	// Done.

	return r;
}

/*!
 * This function is passed to our OpenGL initialization function, and is called
 * during each render loop. See s_init_gl for details.
 *
 * \param stft The STFT whose spectrogram should be rendered.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_render_loop(const s_stft_t *stft)
{
	int r;

	// Render the frame / legend around the output.

	r = s_render_legend_frame();

	if(r < 0)
		return r;

	r = s_render_legend_labels(stft);

	if(r < 0)
		return r;

	// Render the actual graphical STFT output.

	r = s_render_stft(stft);

	if(r < 0)
		return r;

	// Done!

	return 0;
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

	return v;
}

/*!
 * This function renders the spectrogram legend using OpenGL. This includes the
 * frame around the spectrogram, as well as the frequency and time labels for
 * the loaded track.
 *
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_render_legend_frame()
{
	glBindBuffer(GL_ARRAY_BUFFER, s_vbo_list[0].obj);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(s_vbo_list[0].mode, 0, s_vbo_list[0].length / 2);

	glBindVertexArray(0);

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

int s_render_stft(const s_stft_t *stft)
{
	return 0;
}
