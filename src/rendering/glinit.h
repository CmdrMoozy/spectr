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

#ifndef INCLUDE_SPECTR_RENDERING_GLINIT_H
#define INCLUDE_SPECTR_RENDERING_GLINIT_H

#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include "types.h"

extern int s_init_gl(int (*)(const s_stft_t *, GLuint *),
	s_vbo_t *, size_t, const s_stft_t *);

#endif
