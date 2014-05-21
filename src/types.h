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

#ifndef INCLUDE_SPECTR_TYPES_H
#define INCLUDE_SPECTR_TYPES_H

#include <stdint.h>
#include <stddef.h>

/*!
 * \brief This enum contains all of our supported file types.
 */
typedef enum {
	FTYPE_MP3,
	FTYPE_FLAC,
	FTYPE_OGG,
	FTYPE_AAC,
	FTYPE_INVALID
} s_ftype_t;

/*!
 * \brief This struct defines the various properties of an audio stream.
 */
typedef struct s_audio_stat
{
	s_ftype_t type;
	uint32_t bit_depth;
	uint32_t sample_rate;
} s_audio_stat_t;

/*!
 * \brief This struct defines a single stereo audio sample.
 *
 * Note that, although this struct stores a 32-bit sample, the sample actually
 * being stored might in fact be a 16-bit or 24-bit sample. It is up to the
 * user of this struct to keep track of this.
 */
typedef struct s_stereo_sample
{
	int32_t l;
	int32_t r;
} s_stereo_sample_t;

/*!
 * \brief This struct stores the contents of a raw audio file.
 */
typedef struct s_raw_audio
{
	s_audio_stat_t stat;
	size_t samples_length;
	s_stereo_sample_t *samples;
} s_raw_audio_t;

#endif
