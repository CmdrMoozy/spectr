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

int s_process_next_sample(s_stereo_sample_t *, size_t *,
	const char *, size_t *, size_t, uint32_t);

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
	char *audio;
	size_t bytes;
	size_t soff;
	size_t doff;

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

	raw->samples_length = raw->stat.bit_depth >> 2;

	if( (bytes % raw->samples_length) != 0 )
	{
		ret = -EINVAL;
		goto err_after_decode;
	}

	raw->samples_length = bytes / raw->samples_length;
	raw->samples = malloc(sizeof(s_stereo_sample_t) * raw->samples_length);

	if(raw->samples == NULL)
	{
		ret = -ENOMEM;
		goto err_after_decode;
	}

	// Iterate through the decoded raw data, reading in each sample.

	soff = 0;
	doff = 0;

	while(s_process_next_sample(raw->samples, &soff,
		audio, &doff, bytes, raw->stat.bit_depth))
	{
		/*
		 * We continue processing samples until we reach the end of the
		 * raw data buffer.
		 */
	}

	if(soff + 1 != raw->samples_length)
	{
		ret = -EINVAL;

		free(raw->samples);
		raw->samples_length = 0;
		raw->samples = NULL;

		goto err_after_decode;
	}

err_after_decode:
	free(audio);
done:
	return ret;
}

/*!
 * This function writes the contents of the given s_raw_audio_t structure to
 * the given file, in raw PCM format.
 *
 * \param out The file to write the data to.
 * \param raw The raw audio object whose data will be written.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_write_raw_audio(FILE *out, const s_raw_audio_t *raw)
{
	size_t i;
	uint32_t byte;
	uint8_t buf[4];
	size_t bufidx;
	uint32_t shift;
	size_t written;

	uint32_t byted = raw->stat.bit_depth >> 3;

	for(i = 0; i < raw->samples_length; ++i)
	{
		// Write the left sample.

		bufidx = 0;

		for(byte = byted; byte > 0; --byte)
		{
			shift = (byte - 1) << 3;
			buf[bufidx++] = (raw->samples[i].l >> shift) & 0xFF;
		}

		written = fwrite(buf, sizeof(uint8_t), byted, out);

		if(written != byted)
			return -EINVAL;

		// Write the right sample.

		bufidx = 0;

		for(byte = byted; byte > 0; --byte)
		{
			shift = (byte - 1) << 3;
			buf[bufidx++] = (raw->samples[i].r >> shift) & 0xFF;
		}

		written = fwrite(buf, sizeof(uint8_t), byted, out);

		if(written != byted)
			return -EINVAL;
	}

	fflush(out);

	return 0;
}

/*!
 * This function reads and interprets the next sample from the given byte
 * buffer. We return true if there are more samples to be read, or false if
 * we've reached the end of the buffer.
 *
 * \param s The array of samples which will be populated.
 * \param soff The offset of the sample to populate. This will be incremented.
 * \param d The byte buffer to read sample data from.
 * \param doff The offset to read from the buffer. This will be incremented.
 * \param bytes The length of the given buffer, in bytes.
 * \param bits The bit depth of the samples to read.
 * \return True if there are more samples to read, or false if we're done.
 */
int s_process_next_sample(s_stereo_sample_t *s, size_t *soff,
	const char *d, size_t *doff, size_t bytes, uint32_t bits)
{
	size_t o;

	/*
	 * We expect that the raw PCM data is stereo samples. For example, for
	 * data with a bit depth of 16, we would expect each sample to look
	 * like this (where each block is an 8-bit byte):
	 *
	 *     | Left MSB | Left LSB | Right MSB | Right LSB |
	 */

	// Clear both channels of the sample initially.

	s[*soff].l = 0;
	s[*soff].r = 0;

	// Make sure the bytes for these samples aren't out of bounds.

	if( (*doff + (bits >> 2)) >= bytes )
		return 0;

	// Read the bytes for the left channel.

	for(o = 0; o < (bits >> 3); ++o)
	{
		s[*soff].l <<= 8;
		s[*soff].l |= (uint32_t) d[*doff + o];
	}

	*doff += (bits >> 3);

	// Read the bytes for the right channel.

	for(o = 0; o < (bits >> 3); ++o)
	{
		s[*soff].r <<= 8;
		s[*soff].r |= (uint32_t) d[*doff + o];
	}

	*doff += (bits >> 3);

	// Increase the sample index, and then we're done.

	++(*soff);

	return 1;
}
