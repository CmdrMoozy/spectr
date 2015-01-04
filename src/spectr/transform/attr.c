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

#include "attr.h"

#include <errno.h>

#include "spectr/defines.h"
#include "spectr/util/bitwise.h"

/*!
 * This function computes the STFT window size we should be using, assuming
 * that our spectrogram will be w pixels wide, and assuming that our input
 * audio file contains s samples.
 *
 * \param o This will receive the computed window size.
 * \param w The width of the spectrogram, in pixels.
 * \param h The height of the spectrogram, in pixels.
 * \param s The number of samples in the input file.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_get_window_size(size_t *o, size_t UNUSED(w),
	size_t UNUSED(h), size_t UNUSED(s))
{
	*o = 512;
	return 0;
}
