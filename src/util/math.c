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
	int64_t mono = ( ((int64_t) s.l) + ((int64_t) s.r) ) >> 1;
	return (int32_t) mono;
}
