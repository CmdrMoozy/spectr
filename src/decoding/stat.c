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

#include "stat.h"

#include <stdio.h>
#include <errno.h>

#include "decoding/ftype.h"
#include "decoding/quirks/mp3.h"

int s_audio_stat_mp3(s_audio_stat_t *, const char *);
int s_interpret_mp3_rate(s_audio_stat_t *, uint8_t, uint8_t);

/*!
 * This function will populate an audio_stat_t instance's values with the
 * proper stats of the given input audio file.
 *
 * \param stat The audio_stat_t instance which will be populated.
 * \param f The path to the file to inspect.
 * \return 0 on success, or an error number of something goes wrong.
 */
int s_audio_stat(s_audio_stat_t *stat, const char *f)
{
	int r;
	s_ftype_t type;

	r = s_ftype(&type, f);

	if(r < 0)
		return r;

	switch(type)
	{
		case FTYPE_MP3: return s_audio_stat_mp3(stat, f);
		default: return -EINVAL;
	}
}

/*!
 * This is the function that performs the actual actions for audio_stat() for
 * MP3 files in particular. If the given file is not in MP3 format, we will
 * return an error.
 *
 * \param stat The audio_stat_t instance which will be populated.
 * \param f The path to the file to inspect.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_audio_stat_mp3(s_audio_stat_t *stat, const char *f)
{
	int ret = 0;
	int r;
	size_t off;
	FILE *file;
	uint8_t header[4];
	size_t read;
	uint8_t version;
	uint8_t rate;

	stat->type = FTYPE_MP3;
	stat->bit_depth = 16; // ffmpeg decodes MP3's to 16-bit signed PCM.

	// Get the offset of the first MP3 frame header, and open the file.

	r = s_get_mp3_frame_header_offset(&off, f);

	if(r < 0)
	{
		ret = r;
		goto done;
	}

	file = fopen(f, "rb");

	if(!file)
	{
		ret = -EIO;
		goto done;
	}

	r = fseek(file, off, 0);

	if(r < 0)
	{
		ret = -EIO;
		goto err_after_fopen;
	}

	/*
	 * Read the MP3 frame header, and extract the information we want from
	 * it. Note that this code assumes that all MP3 frames in the file
	 * share the same MPEG version and sample rate (this is true for any
	 * sane MP3 file).
	 *
	 * The header is four bytes long, with the following fields:
	 *
	 *     AAAAAAAA AAABBCCD EEEEFFGH IIJJKLMM
	 *
	 *     A (11 bits) - Frame sync (all bits set)
	 *     B (2 bits)  - MPEG Audio version ID
	 *     C (2 bits)  - Layer description
	 *     D (1 bit)   - Protection bit
	 *     E (4 bits)  - Bitrate index
	 *     F (2 bits)  - Sampling rate frequency index
	 *     G (1 bit)   - Padding bit
	 *     H (1 bit)   - Private bit
	 *     I (2 bits)  - Channel mode
	 *
	 * We extract the MPEG audio version ID as well as the sampling rate
	 * frequency index, and then pass them to another function to be
	 * interpreted.
	 */

	read = fread(header, sizeof(uint8_t), 4, file);

	if(read != 4)
	{
		ret = -EIO;
		goto err_after_fopen;
	}

	version = (header[1] & 0x18) >> 3;
	rate = (header[2] & 0x0C) >> 2;

	r = s_interpret_mp3_rate(stat, version, rate);

	if(r < 0)
	{
		ret = r;
		goto err_after_fopen;
	}

err_after_fopen:
	fclose(file);

done:
	return ret;
}

/*!
 * This function interprets an MPEG version value and a sampling rate index
 * value from an MP3 frame header in order to set the sample_rate field of the
 * given audio_stat_t instance.
 *
 * \param stat The audio_stat_t instance whose sample_rate field will be set.
 * \param version The MPEG version value from the MP3 frame header.
 * \param rate The sampling rate index value from the MP3 frame header.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_interpret_mp3_rate(s_audio_stat_t *stat, uint8_t version, uint8_t rate)
{
	int ret = 0;

	/*
	 * The following switch statements interpret the version and rate using
	 * the following table (the version and rate values are each two bits
	 * long, and the value should be in the low two bits of the parameters
	 * given):
	 *
	 *                Version 00    Version 10    Version 11
	 *
	 *     Rate 00         11025         22050         44100
	 *     Rate 01         12000         24000         48000
	 *     Rate 10          8000         16000         32000
	 *
	 * These values are defined by the MP3 frame format. More information
	 * can be found e.g. here:
	 *
	 *     http://mpgedit.org/mpgedit/mpeg_format/MP3Format.html
	 */

	switch(version)
	{
		case 0x00: // MPEG Version 2.5 (unofficial)
			switch(rate)
			{
				case 0x00:
					stat->sample_rate = 11025;
					break;

				case 0x01:
					stat->sample_rate = 12000;
					break;

				case 0x02:
					stat->sample_rate = 8000;
					break;

				default:
					ret = -EINVAL;
					break;
			}
			break;

		case 0x02: // MPEG Version 2 (ISO/IEC 13818-3)
			switch(rate)
			{
				case 0x00:
					stat->sample_rate = 22050;
					break;

				case 0x01:
					stat->sample_rate = 24000;
					break;

				case 0x02:
					stat->sample_rate = 16000;
					break;

				default:
					ret = -EINVAL;
					break;
			}
			break;

		case 0x03: // MPEG Version 1 (ISO/IEC 11172-3)
			switch(rate)
			{
				case 0x00:
					stat->sample_rate = 44100;
					break;

				case 0x01:
					stat->sample_rate = 48000;
					break;

				case 0x02:
					stat->sample_rate = 32000;
					break;

				default:
					ret = -EINVAL;
					break;
			}
			break;

		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}
