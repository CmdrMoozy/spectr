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

#include "decoding/ftype.h"
#include "decoding/quirks/mp3.h"

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
int s_decode(char **buf, size_t *bufSize, const char *f)
{
	int r;
	s_ftype_t type;

	r = s_ftype(&type, f);

	if(r != 0)
		return r;

	switch(type)
	{
		case FTYPE_MP3: return s_decode_mp3(buf, bufSize, f);
		default: return -EINVAL;
	}
}
