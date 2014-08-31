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

#include "mp3.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include <mad.h>

#include "defines.h"
#include "util/bitwise.h"

/*!
 * \brief This structure stores the input and output buffers for MAD decoding.
 */
typedef struct s_mad_buffer
{
	const uint8_t *in;
	size_t length;
	FILE *out;
} s_mad_buffer_t;

int s_is_mp3_frame_header(const uint8_t *, size_t);
enum mad_flow s_mad_input(void *, struct mad_stream *);
int s_mad_scale(mad_fixed_t);
enum mad_flow s_mad_output(void *,
	struct mad_header const *, struct mad_pcm *);
enum mad_flow s_mad_error(void *, struct mad_stream *, struct mad_frame *);
int s_decode_mp3_data(FILE *, const uint8_t *, size_t);

/*!
 * This function attemps to locate the first valid MP3 frame header in the
 * given file. If an ID3v2 tag is present, we try to use the information it
 * contains to find a valid header. Otherwise, the file is simply searched
 * sequentially.
 *
 * \param o This will receive the offset of the first valid MP3 frame header.
 * \param f The path to the file to examine.
 * \return 0 on success, or an error number if no headers could be found.
 */
int s_get_mp3_frame_header_offset(size_t *o, const char *f)
{
	int ret = 0;
	int r;
	int fd;
	struct stat stat;
	uint8_t *file;
	size_t off = 0;
	size_t i;

	// mmap the file, since our reads will be rather random.

	fd = open(f, O_RDONLY);

	if(fd < 0)
	{
		ret = -errno;
		goto done;
	}

	r = fstat(fd, &stat);

	if(r < 0)
	{
		ret = -errno;
		goto err_after_open;
	}

	if(stat.st_size < 10)
	{
		ret = -EIO;
		goto err_after_open;
	}

	file = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

	if(file == MAP_FAILED)
	{
		ret = -errno;
		goto err_after_open;
	}

	/*
	 * If the file has an ID3v2 header, find the offset to skip it. ID3v2
	 * headers start with the bytes 0x49 0x44 0x33 ("ID3").
	 */

	if( (file[0] == 0x49) && (file[0] == 0x44) && (file[0] == 0x33) )
	{
		/*
		 * The 10-byte ID3v2 header is formatted as follows:
		 *
		 *     49 44 33 yy yy xx zz zz zz zz
		 *
		 * Where yy is the ID3v2 version (and is guaranteed to be less
		 * than 0xFF), xx is the "flags" byte and the zz bytes are the
		 * ID3v2 tag length (represented as a 32-bit synchsafe integer).
		 */

		off = (size_t) s_from_synchsafe_int32(file, 6);

		/*
		 * The tag size field excludes the length of the header, so add
		 * that value in.
		 */

		off += 10;

		/*
		 * The tag size field also excludes the length of the footer,
		 * if one is present. Check if it is present by examining the
		 * "flags" field, and then adding the length of the footer if
		 * one is found.
		 *
		 * See the ID3v2 standard for more information:
		 *
		 *     http://id3.org/id3v2.4.0-structure
		 */

		if(file[5] & 0x10)
			off += 10;
	}

	// Verify that there's a valid MP3 header at the offset we found.

	if(!s_is_mp3_frame_header(file, off))
	{
		/*
		 * Looks like we didn't find a valid MP3 frame header where we
		 * expected to, based upon the ID3v2 tag. Let's just assume the
		 * ID3v2 tag is corrupt, and its size field is incorrect. Try
		 * searching the file for the first valid MP3 frame header it
		 * contains.
		 *
		 * Note that this is not all that reliable, but can still be
		 * useful in the interest of supporting files tha are only
		 * somewhat malformed.
		 */

		for(i = 0; i < (size_t) (stat.st_size - 1); ++i)
		{
			if(s_is_mp3_frame_header(file, i))
			{
				off = i;
				break;
			}
		}
	}

	// Verify that we eventually did find an MP3 frame header, and finish.

	if(!s_is_mp3_frame_header(file, off))
	{
		ret = -EINVAL;
		goto err_after_mmap;
	}

	*o = off;

err_after_mmap:
	munmap(file, stat.st_size);
err_after_open:
	close(fd);
done:
	return ret;
}

/*!
 * This function decodes a given MP3 file to raw 16-bit signed little-endian
 * PCM samples, placing the output in the given buffer.
 *
 * NOTE: It is up to the caller to ensure that the given buffer hasn't already
 * been allocated, and to free it when done.
 *
 * \param buf The buffer to write decoded audio data to.
 * \param bufSize This will receive the size of the resulting audio.
 * \param f The path to the MP3 file to decode.
 */
int s_decode_mp3(char **buf, size_t *bufSize, const char *f)
{
	int ret = 0;
	int r;
	struct stat s;
	int fd;
	void *in;
	FILE *out;

	// Map the file into memory.

	fd = open(f, O_RDONLY);

	if(fd < 0)
	{
		ret = -errno;
		goto done;
	}

	r = stat(f, &s);

	if(r < 0)
	{
		ret = -errno;
		goto err_after_open;
	}

	in = mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, fd, 0);

	if(in == MAP_FAILED)
	{
		ret = -errno;
		goto err_after_open;
	}

	// Open a memory stream on the given buffer.

	out = open_memstream(buf, bufSize);

	if(out == NULL)
	{
		ret = -EIO;
		goto err_after_mmap;
	}

	// Decode the mapped file.

	r = s_decode_mp3_data(out, in, s.st_size);

	if(r < 0)
	{
		ret = r;
		goto err_after_memstream;
	}

	// Clean up and we're done!

err_after_memstream:
	fclose(out);
err_after_mmap:
	munmap(in, s.st_size);
err_after_open:
	close(fd);
done:
	return ret;
}

/*!
 * This is a very simple function which tests if, at the given offset in the
 * given buffer, there is a valid MP3 frame header. MP3 frame headers always
 * start with the bytes 0xFF 0xFB -or- 0xFF 0xFA.
 *
 * \param buf The buffer to examine.
 * \param off The offset in the buffer to examine.
 * \return Whether or not an MP3 frame header is presen at the given offset.
 */
int s_is_mp3_frame_header(const uint8_t *buf, size_t off)
{
	return (buf[off] == 0xFF) &&
		( (buf[off + 1] == 0xFB) || (buf[off + 1] == 0xFA) );
}

/*!
 * This function is a callback for libmad which deals with populating MAD's
 * input buffer.
 *
 * \param data The s_mad_buffer_t used to initialize the decoder.
 * \param stream The stream buffer which is to be decoed.
 * \return How / whether to proceed with decoding.
 */
enum mad_flow s_mad_input(void *data, struct mad_stream *stream)
{
	s_mad_buffer_t *buffer = data;

	if(!buffer->length)
		return MAD_FLOW_STOP;

	mad_stream_buffer(stream, buffer->in, buffer->length);

	buffer->length = 0;

	return MAD_FLOW_CONTINUE;
}

/*!
 * This is an extremely basic function which scales MAD's high-resolution
 * samples down to 16-bit PCM. Its output is not particularly high-quality,
 * but it's good enough for rendering a spectrogram.
 *
 * This implementation is taken from `minimad.c` in the official libmad source
 * tree.
 *
 * \param sample The sample to scale.
 * \return The sample, scaled down to 16-bits.
 */
int s_mad_scale(mad_fixed_t sample)
{
	sample += (1L << (MAD_F_FRACBITS - 16));

	if(sample >= MAD_F_ONE)
		sample = MAD_F_ONE - 1;
	else if(sample < -MAD_F_ONE)
		sample = -MAD_F_ONE;

	return sample >> (MAD_F_FRACBITS + 1 - 16);
}

/*!
 * This function is a callback for libmad  which handles successfully decoded
 * output from the decoder. In particular, we write the sample(s) to our output
 * buffer.
 *
 * \param data The s_mad_buffer_t used to initialize the decoder.
 * \param header The MPEG frame header information.
 * \param pcm The decoded raw PCM data.
 * \return How / whether to proceed with decoding.
 */
enum mad_flow s_mad_output(void *data, struct mad_header const *UNUSED(header),
	struct mad_pcm *pcm)
{
	uint8_t outbuf[2];
	s_mad_buffer_t *buffer = data;
	unsigned int nchannels;
	unsigned int nsamples;
	mad_fixed_t const *left_ch;
	mad_fixed_t const *right_ch;
	int sample;

	nchannels = pcm->channels;
	nsamples = pcm->length;
	left_ch = pcm->samples[0];
	right_ch = pcm->samples[1];

	while(nsamples--)
	{
		sample = s_mad_scale(*left_ch++);

		outbuf[0] = sample & 0xFF;
		outbuf[1] = (sample >> 8) & 0xFF;

		fwrite(outbuf, sizeof(uint8_t), 2, buffer->out);

		if(nchannels == 2)
		{
			sample = s_mad_scale(*right_ch++);

			outbuf[0] = sample & 0xFF;
			outbuf[1] = (sample >> 8) & 0xFF;

			fwrite(outbuf, sizeof(uint8_t), 2, buffer->out);
		}
	}

	fflush(buffer->out);

	return MAD_FLOW_CONTINUE;
}

/*!
 * This function is a callback for libmad which handles any decoding errors
 * encountered by libmad. This particular implementation is very forgiving, and
 * simply carries on decoding.
 *
 * \param data The s_mad_buffer_t used to initialize the decoder.
 * \param stream The stream being decoded.
 * \param frame The frame at which the error occured.
 * \return How / whether to proceed with decoding.
 */
enum mad_flow s_mad_error(void *UNUSED(data),
	struct mad_stream *UNUSED(stream), struct mad_frame *UNUSED(frame))
{
	return MAD_FLOW_CONTINUE;
}

/*!
 * This is a very basic utility function which initializes an instance of the
 * MAD decoder, and uses it to decode the data in the given input buffer.
 *
 * \param out The FILE * to write decoded data to.
 * \param in The input file's contents.
 * \param inl The length of the given input buffer.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_decode_mp3_data(FILE *out, const uint8_t *in, size_t inl)
{
	s_mad_buffer_t buffer;

	buffer.in = in;
	buffer.length = inl;
	buffer.out = out;

	struct mad_decoder decoder;

	mad_decoder_init(&decoder, &buffer, s_mad_input, NULL, NULL,
		s_mad_output, s_mad_error, NULL);

	mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

	mad_decoder_finish(&decoder);

	return 0;
}
