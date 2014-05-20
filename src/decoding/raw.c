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

#include "raw.h"

#include <errno.h>
#include <stdlib.h>

#include "decoding/decode.h"
#include "decoding/stat.h"

/*!
 * This function initializes (allocates) a s_raw_audio_t variable. If the
 * pointer is non-NULL, we will not allocate a new value on top of it.
 *
 * \param r The s_raw_audio_t to allocate.
 * \return 0 on success, or an error number otherwise.
 */
int s_init_raw_audio(s_raw_audio_t **r)
{
	if(*r != NULL)
		return -EINVAL;

	*r = malloc(sizeof(s_raw_audio_t));

	if(r == NULL)
		return -ENOMEM;

	(*r)->stat.type = FTYPE_INVALID;
	(*r)->stat.bit_depth = 0;
	(*r)->stat.sample_rate = 0;

	(*r)->samples_length = 0;
	(*r)->samples = NULL;

	return 0;
}

/*!
 * This function frees the given s_raw_audio_t structure, including the list
 * of samples it contains (if any). Note that this function is safe against
 * double-frees.
 *
 * \param r The s_raw_audio_t to free.
 * \return 0 on success, or an error number otherwise.
 */
int s_free_raw_audio(s_raw_audio_t **r)
{
	if(*r == NULL)
		return 0;

	if((*r)->samples != NULL)
	{
		free((*r)->samples);
		(*r)->samples = NULL;
	}

	free(*r);
	*r = NULL;

	return 0;
}

int s_decode_raw_audio(s_raw_audio_t *raw, const char *f)
{
	int ret = 0;
	int r;
	char *audio;
	size_t bytes;

	// Try decoding the input file.

	r = s_decode(&audio, &bytes, f);

	if(r < 0)
	{
		ret = r;
		goto done;
	}

	r = s_audio_stat(&(raw->stat), f);

	if(r < 0)
	{
		ret = r;
		goto err_after_decode;
	}



err_after_decode:
	free(audio);
done:
	return ret;
}
