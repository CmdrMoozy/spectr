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

#include "math.h"

#include <math.h>

/*!
 * This function returns the mono version of the given stereo sample. This is
 * computed by averaging the two channels of the given sample, without
 * overflowing.
 *
 * \param s The sample to convert to a mono sample.
 * \return The mono sample value as close as possible to the given sample.
 */
int32_t s_mono_sample(s_stereo_sample_t s)
{
	double l = (double) s.l;
	double r = (double) s.r;
	double m = (l / 2.0) + (r / 2.0);

	return (int32_t) floor(m);
}

/*!
 * This is a very simple function which scales the value v, which is currenty
 * in the range [omin, omax] to the range [nmin, nmax].
 *
 * \param omin The old minimum (inclusive).
 * \param omax The old maximum (inclusive).
 * \param nmin The new minimum (inclusive).
 * \param nmax The new maximum (inclusive).
 * \param v The value to scale.
 * \return The scaled value.
 */
double s_scale(double omin, double omax, double nmin, double nmax, double v)
{
	double nv = (nmax - nmin) * (v - omin);
	nv /= (omax - omin);
	nv += nmin;

	return nv;
}
