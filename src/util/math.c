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

#include <errno.h>

#include "util/bitwise.h"

/*!
 * This function computes the STFT window size we should be using, assuming
 * that our spectrogram will be w pixels wide, and assuming that our input
 * audio file contains s samples.
 *
 * This window size will be the largest power of two which means that there
 * will be at least one DFT result for each "column" in the spectrogram.
 *
 * Note that this does imply that s must be >= w, since the smallest window
 * size we can choose is 1, and a window size of 1 doesn't fulfill the above
 * guarantee for any s < w.
 *
 * \param o This will receive the computed window size.
 * \param w The width of the spectrogram, in pixels.
 * \param s The number of samples in the input file.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_get_window_size(size_t *o, size_t w, size_t s)
{
	if(s < w)
		return -EINVAL;

	*o = s / w;
	*o = s_flp2(*o);

	return 0;
}

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

