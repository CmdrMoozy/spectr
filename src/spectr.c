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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "spectr_types.h"
#include "decoding/raw.h"

#ifdef SPECTR_DEBUG
	#include <inttypes.h>

	#include "decoding/stat.h"
#endif

void s_print_error(int);

int main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;
	int r;
	s_raw_audio_t *audio = NULL;

#ifdef SPECTR_DEBUG
	uint32_t duration;
#endif

	if(argc < 2)
	{
		printf("Usage: spectr <file to analyze> [PCM out file]\n");

		ret = EXIT_FAILURE;
		goto done;
	}

	// Decode the input file we were given.

	r = s_init_raw_audio(&audio);

	if(r < 0)
	{
		s_print_error(r);
		ret = EXIT_FAILURE;
		goto done;
	}

	r = s_decode_raw_audio(audio, argv[1]);

	if(r < 0)
	{
		s_print_error(r);
		ret = EXIT_FAILURE;
		goto err_after_alloc;
	}

#ifdef SPECTR_DEBUG
	printf("Loaded input file - %" PRIu32 " Hz / %" PRIu32 "-bit.\n",
		audio->stat.sample_rate, audio->stat.bit_depth);

	duration = s_audio_duration_sec(&(audio->stat), audio->samples_length);

	printf("\t%zu samples yields duration of %" PRIu32 "m %" PRIu32 "s\n",
		audio->samples_length, duration / 60, duration % 60);
#endif

	// If applicable, dump the raw data back to a file (for debugging).

	if(argc >= 3)
	{
		FILE *out = fopen(argv[2], "wb");

		if(out == NULL)
		{
			s_print_error(-errno);
			ret = EXIT_FAILURE;
			goto err_after_alloc;
		}

		r = s_write_raw_audio(out, audio);

		fclose(out);

		if(r < 0)
		{
			s_print_error(r);
			ret = EXIT_FAILURE;
			goto err_after_alloc;
		}
	}

err_after_alloc:
	s_free_raw_audio(&audio);
done:
	return ret;
}

void s_print_error(int error)
{
	printf("Fatal error %d: %s\n", -error, strerror(-error));
}
