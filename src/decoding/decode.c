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

#include "decode.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include "decoding/ftype.h"
#include "decoding/quirks/mp3.h"

#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

int decode_with_codec(char **, size_t *, const char *,
	ftype_t, enum AVCodecID);
int decode_with_context(FILE *, size_t, FILE *, AVCodecContext *);

/*!
 * This function decodes the contents of the file denoted the the path f,
 * storing the decoded audio data in the given buffer. The resulting buffer
 * will be null-terminated, and its size (excluding the null terminator) will
 * be stored in bufSize.
 *
 * The buffer is allocated to be the proper size to store the decoded audio
 * data. It is up to the caller to free() this buffer.
 *
 * \param buf The buffer to store the decoded audio data in.
 * \param bufSize Receives the size of the decoded data, in bytes.
 * \param f The path to the file to decode.
 * \return 0 on success, or an error number if decoding fails.
 */
int decode(char **buf, size_t *bufSize, const char *f)
{
	int r;
	ftype_t type;
	enum AVCodecID codecID;

	r = ftype(&type, f);

	if(r != 0)
		return r;

	codecID = codec_for_ftype(type);

	if(codecID == AV_CODEC_ID_NONE)
		return -EINVAL;

	return decode_with_codec(buf, bufSize, f, type, codecID);
}

/*!
 * This function attemps to decode a given audio file, using the given codec.
 * See decode() for more information.
 *
 * \param buf The buffer to store the decoded audio data in.
 * \param bufSize Receives the size of the decoded data, in bytes.
 * \param f The path to the file to decode.
 * \param t The file type being decoded.
 * \param c The ffmpeg codec to use to decode the file.
 * \return 0 on success, or an error number if decoding fails.
 */
int decode_with_codec(char **buf, size_t *bufSize, const char *f,
	ftype_t t, enum AVCodecID c)
{
	AVCodec *codec;
	AVCodecContext *ctxt = NULL;
	FILE *in;
	FILE *out;
	int r;
	size_t off = 0;

	// Setup the ffmpeg context.

	av_register_all();

	codec = avcodec_find_decoder(c);

	if(!codec)
		return -EINVAL;

	ctxt = avcodec_alloc_context3(codec);

	if(!ctxt)
		return -EINVAL;

	if(avcodec_open2(ctxt, codec, NULL) < 0)
		return -EINVAL;

	// Deal with any file forma quirks, so ffmpeg can decode successfully.

	if(t == FTYPE_MP3)
	{
		/*
		 * For MP3 files, the ID3v2 tag needs to be skipped, or else
		 * ffmpeg will refuse to decode the file. Find the offset of
		 * the first valid MP3 frame.
		 */

		r = get_mp3_frame_header_offset(&off, f);

		if(r < 0)
			return r;
	}

	// Decode the given input file.

	in = fopen(f, "rb");

	if(!in)
		return -EIO;

	out = open_memstream(buf, bufSize);

	if(!out)
		return -EIO;

	r = decode_with_context(in, off, out, ctxt);

	fclose(out);

	fclose(in);

	avcodec_close(ctxt);
	av_free(ctxt);

	return r;
}

/*!
 * This function attempts to decode the contents of one file, writing the
 * decoded data t another file, using the given pre-allocated ffmpeg context.
 *
 * \param in The input file to read from. Should be opened in "rb" mode.
 * \param off The byte offset in the input file where the audio data starts.
 * \param out The output file to write to. Should be opened in "wb" mode.
 * \param ctxt The ffmpeg context to use to decode the input file.
 * \return 0 on success, or an error number if decoding fails.
 */
int decode_with_context(FILE *in, size_t off, FILE *out, AVCodecContext *ctxt)
{
	int r;
	uint8_t inbuf[AUDIO_INBUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];
	AVPacket avpkt;
	AVFrame *decodedFrame = NULL;
	int gotFrame = 0;
	int len;
	int dataSize;

	r = fseek(in, off, 0);

	if(r < 0)
		return -errno;

	av_init_packet(&avpkt);

	avpkt.data = inbuf;
	avpkt.size = fread(inbuf, sizeof(uint8_t), AUDIO_INBUF_SIZE, in);

	while(avpkt.size > 0)
	{
		gotFrame = 0;

		if(!decodedFrame)
			if(!(decodedFrame = avcodec_alloc_frame()))
				return -EINVAL;

		len = avcodec_decode_audio4(ctxt, decodedFrame,
			&gotFrame, &avpkt);

		if(len < 0)
			return -EIO;

		if(gotFrame)
		{
			dataSize = av_samples_get_buffer_size(NULL,
				ctxt->channels, decodedFrame->nb_samples,
				ctxt->sample_fmt, 1);

			if(dataSize < 0)
				return -EINVAL;

			fwrite(decodedFrame->data[0], sizeof(uint8_t),
				dataSize, out);
		}

		avpkt.size -= len;
		avpkt.data += len;
		avpkt.dts = avpkt.pts = AV_NOPTS_VALUE;

		if(avpkt.size < AUDIO_REFILL_THRESH)
		{
			memmove(inbuf, avpkt.data, avpkt.size);
			avpkt.data = inbuf;

			len = fread(avpkt.data + avpkt.size, sizeof(uint8_t),
				AUDIO_INBUF_SIZE - avpkt.size, in);

			if(len > 0)
				avpkt.size += len;
		}
	}

	avcodec_free_frame(&decodedFrame);

	fflush(out);

	return 0;
}
