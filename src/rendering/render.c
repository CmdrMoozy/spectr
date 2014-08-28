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

int s_render_loop(int, int, const s_stft_t *);
s_spectrogram_viewport s_get_spectrogram_viewport(int, int);
void s_gl_legend_color();
int s_render_legend_frame(int, int);
int s_render_legend_labels(int, int, const s_stft_t *);
int s_render_stft(int, int, const s_stft_t *);

/*!
 * This function starts our OpenGL rendering loop, to render the given STFT's
 * spectrogram.
 *
 * \param stft The STFT whose spectrogram should be rendered.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_render(const s_stft_t *stft)
{
	return s_init_gl(s_render_loop, stft);
}

/*!
 * This function is passed to our OpenGL initialization function, and is called
 * during each render loop. See s_init_gl for details.
 *
 * \param width The width of the framebuffer.
 * \param height The height of the framebuffer.
 * \param stft The STFT whose spectrogram should be rendered.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_render_loop(int width, int height, const s_stft_t *stft)
{
	int r;

	// Render the frame / legend around the output.

	r = s_render_legend_frame(width, height);

	if(r < 0)
		return r;

	r = s_render_legend_labels(width, height, stft);

	if(r < 0)
		return r;

	// Render the actual graphical STFT output.

	r = s_render_stft(width, height, stft);

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
 * \param fbw The framebuffer width.
 * \param fbh The framebuffer height.
 * \return The spectrogram viewport for the given framebuffer.
 */
s_spectrogram_viewport s_get_spectrogram_viewport(int fbw, int fbh)
{
	s_spectrogram_viewport v;

	v.xmin = 75;
	v.ymin = 5;

	v.xmax = fbw - 5;
	v.ymax = fbh - 30;

	return v;
}

/*!
 * This function renders the spectrogram legend using OpenGL. This includes the
 * frame around the spectrogram, as well as the frequency and time labels for
 * the loaded track.
 *
 * \param fbw The framebuffer width.
 * \param fbh The framebuffer height.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_render_legend_frame(int fbw, int fbh)
{
	s_spectrogram_viewport view = s_get_spectrogram_viewport(fbw, fbh);

	// Draw the frame which will contain the actual spectrogram.

	glBegin(GL_LINE_LOOP);

		glVertex2i(view.xmin, view.ymin);
		glVertex2i(view.xmax, view.ymin);
		glVertex2i(view.xmax, view.ymax);
		glVertex2i(view.xmin, view.ymax);

	glEnd();

	// Draw the extra lines which we'll render labels next to.

	glBegin(GL_LINE_LOOP);

		glVertex2i(view.xmin - S_SPEC_LGND_TICK_SIZE, view.ymin);
		glVertex2i(view.xmin, view.ymin);

	glEnd();

	glBegin(GL_LINE_LOOP);

		glVertex2i(view.xmin - S_SPEC_LGND_TICK_SIZE, view.ymax);
		glVertex2i(view.xmin, view.ymax);

	glEnd();

	glBegin(GL_LINE_LOOP);

		glVertex2i(view.xmin, view.ymax);
		glVertex2i(view.xmin, view.ymax + S_SPEC_LGND_TICK_SIZE);

	glEnd();

	glBegin(GL_LINE_LOOP);

		glVertex2i(view.xmax, view.ymax);
		glVertex2i(view.xmax, view.ymax + S_SPEC_LGND_TICK_SIZE);

	glEnd();

	// Done!

	return 0;
}

int s_render_legend_labels(int fbw, int fbh, const s_stft_t *stft)
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

	view = s_get_spectrogram_viewport(fbw, fbh);

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

int s_render_stft(int fbw, int fbh, const s_stft_t *stft)
{
	return 0;
}
