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

#include "glutils.h"

/*!
 * This function computes a very standard orthographic projection matrix using
 * the given inputs. We produce the same type of matrix that e.g. glOrtho()
 * used to use. See http://en.wikipedia.org/wiki/Orthographic_projection for
 * more information.
 *
 * \param m The column-major list of matrix values. Should be length 16.
 * \param l The left value.
 * \param r The right value.
 * \param b The bottom value.
 * \param t The top value.
 * \param n The near value.
 * \param f The far value.
 */
void s_ortho_matrix(GLfloat *m, GLfloat l, GLfloat r,
	GLfloat b, GLfloat t, GLfloat n, GLfloat f)
{
	m[0] = 2.0f / (r - l);
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;

	m[4] = 0.0f;
	m[5] = 2.0f / (t - b);
	m[6] = 0.0f;
	m[7] = 0.0f;

	m[8] = 0.0f;
	m[9] = 0.0f;
	m[10] = -2.0f / (f - n);
	m[11] = 0.0f;

	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = 0.0f;
	m[15] = 1.0f;
}
