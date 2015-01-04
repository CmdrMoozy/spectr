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

#include "spectr/decoding/decode.h"
#include "spectr/decoding/stat.h"
#include "spectr/util/math.h"

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
 */
void s_free_raw_audio(s_raw_audio_t **r)
{
	if(*r == NULL)
		return;

	if((*r)->samples != NULL)
	{
		free((*r)->samples);
		(*r)->samples = NULL;
	}

	free(*r);
	*r = NULL;
}

/*!
 * This is a convenience function which copies the entire contents of the given
 * source structure to the given destination structure. This is a wapper for
 * calling s_copy_raw_audio_window with an offset of 0 and a window size equal
 * to the length of the entire source structure.
 *
 * \param dst The destination for the copy.
 * \param src The s_raw_audio_t structure to copy.
 * \return 0 on success, or an error number otherwise.
 */
int s_copy_raw_audio(s_raw_audio_t **dst, const s_raw_audio_t *src)
{
	return s_copy_raw_audio_window(dst, src, 0, src->samples_length);
}

/*!
 * This function copies a portion of the given source structure into the given
 * destination structure. Only the samples in the range [o, o + w - w] will be
 * copied. Note, though, that the stat structure will be copied in its entirety
 * from the source structure.
 *
 * \param dst The destination for the copy.
 * \param src The s_raw_audio_t structure to copy.
 * \param o The offset to start copying samples from.
 * \param w The size of the window - i.e., the number of samples to copy.
 * \return 0 on success, or an error number otherwise.
 */
int s_copy_raw_audio_window(s_raw_audio_t **dst, const s_raw_audio_t *src,
	size_t o, size_t w)
{
	int r;
	size_t i;

	// Make sure the destinatino is a newly-initialized s_raw_audio_t.

	s_free_raw_audio(dst);

	r = s_init_raw_audio(dst);

	if(r < 0)
		return r;

	// Copy the properties from the source structure.

	(*dst)->stat = src->stat;

	// Allocate space for the copy.

	(*dst)->samples_length = w;

	(*dst)->samples = malloc(sizeof(s_stereo_sample_t) * w);

	if((*dst)->samples == NULL)
	{
		s_free_raw_audio(dst);
		return r;
	}

	// Copy the values from the source structure.

	for(i = o; i < w; ++i)
		(*dst)->samples[i] = src->samples[i + o];

	// We're done!

	return 0;
}

/*!
 * This function decodes the audio in the given file to raw PCM format, and
 * then processes that raw data to populate the given s_raw_audio_t structure.
 *
 * \param raw The raw audio structure to store the decoded data inside.
 * \param f The path to the input file to read.
 * \return 0 on success, or an error number of something goes wrong.
 */
int s_decode_raw_audio(s_raw_audio_t *raw, const char *f)
{
	int ret = 0;
	int r;
	uint8_t *audio;
	size_t bytes;
	FILE *in;
	int16_t buf[1];
	size_t idx;

	// Try decoding the input file.

	r = s_decode(&audio, &bytes, f);

	if(r < 0)
	{
		ret = r;
		goto done;
	}

	in = fmemopen(audio, bytes, "r");

	if(in == NULL)
	{
		ret = -errno;
		goto err_after_decode;
	}

	r = s_audio_stat(&(raw->stat), f);

	if(r < 0)
	{
		ret = r;
		goto err_after_fmemopen;
	}

	/*
	 * Try allocating memory for the file's samples.
	 *
	 * For a bit depth of B, the left and right portions of each sample
	 * will be B / 8 bytes each (or, in other words, each entire sample
	 * will be B / 4 bytes). This means that:
	 *
	 *     - B / 4 should evenly divide the bytes in the raw output.
	 *     - bytes / (B / 4) = number of samples
	 */

	if(raw->samples != NULL)
	{
		free(raw->samples);
		raw->samples = NULL;
	}

	raw->samples_length = raw->stat.bit_depth / 4;

	if( (bytes % raw->samples_length) != 0 )
	{
		ret = -EINVAL;
		goto err_after_fmemopen;
	}

	raw->samples_length = bytes / raw->samples_length;
	raw->samples = malloc(sizeof(s_stereo_sample_t) * raw->samples_length);

	if(raw->samples == NULL)
	{
		ret = -ENOMEM;
		goto err_after_fmemopen;
	}

	// Iterate through the decoded raw data, reading in each sample.

	for(idx = 0; idx < raw->samples_length; ++idx)
	{
		fread(buf, sizeof(int16_t), 1, in);
		raw->samples[idx].l = buf[0];

		fread(buf, sizeof(int16_t), 1, in);
		raw->samples[idx].r = buf[0];
	}

err_after_fmemopen:
	fclose(in);
err_after_decode:
	free(audio);
done:
	return ret;
}
