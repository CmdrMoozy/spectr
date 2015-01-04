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

#include "ftype.h"

#include "spectr/decoding/quirks/mp3.h"

/*!
 * This function determines the file type of the file denoted by the given path
 * f, placing the result in the variable pointed to by t.
 *
 * If the given file's type can't be determined (or it is a format we don't
 * support), then t will receive FTYPE_INVALID, and we will return EINVAL.
 *
 * \param t This will receive the computed file type.
 * \param f The path to the file whose type will be determined.
 * \return 0 on success, or an error number otherwise.
 */
int s_ftype(s_ftype_t *t, const char *f)
{
	size_t off;

	*t = FTYPE_INVALID;

	if(s_get_mp3_frame_header_offset(&off, f) == 0)
	{
		*t = FTYPE_MP3;
		return 0;
	}

	return -EINVAL;
}

/*!
 * This function returns the ffmpeg AVCodecID value which can be used to decode
 * the given file type. If no codec exists, or something else goes wrong, then
 * AV_CODEC_ID_NONE is returned instead.
 *
 * \param t The file type to find a codec for.
 * \return The ffmpeg codec which can be used to decode the given file type.
 */
enum AVCodecID s_codec_for_ftype(s_ftype_t t)
{
	switch(t)
	{
		case FTYPE_MP3: return AV_CODEC_ID_MP3;
		case FTYPE_FLAC: return AV_CODEC_ID_FLAC;
		case FTYPE_OGG: return AV_CODEC_ID_VORBIS;
		case FTYPE_AAC: return AV_CODEC_ID_AAC;

		default:
			return AV_CODEC_ID_NONE;
	}
}
