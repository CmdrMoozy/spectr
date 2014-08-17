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

#ifndef INCLUDE_SPECTR_RENDERING_RENDER_H
#define INCLUDE_SPECTR_RENDERING_RENDER_H

#include "types.h"

/*!
 * \brief This structure defines the bounds of our spectrogram viewport.
 *
 * The points (xmin, ymin) and (xmax, ymax) are the upper-left and lower-right
 * corners of our spectrogram viewport, respectively.
 */
typedef struct s_spectrogram_viewport
{
	int xmin;
	int ymin;
	int xmax;
	int ymax;
} s_spectrogram_viewport;

extern int s_render_loop(const s_stft_t *stft);

#endif
