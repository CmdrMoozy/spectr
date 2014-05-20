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
#include <inttypes.h>

#include "spectr_types.h"
#include "decoding/decode.h"
#include "decoding/stat.h"

void s_print_error(int);

int main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;
	char *audio = NULL;
	size_t audioSize;
	int r;
	FILE *pcmout;
	s_audio_stat_t stat;

	if(argc < 2)
	{
		printf("Usage: spectr <file to analyze> [PCM out file]\n");

		ret = EXIT_FAILURE;
		goto done;
	}

	// Decode the input file we were given.

	r = s_decode(&audio, &audioSize, argv[1]);

	if(r != 0)
	{
		s_print_error(r);
		ret = EXIT_FAILURE;
		goto err_after_decode;
	}

	// If we were given an output file, write our decoded input.

	if(argc > 2)
	{
		pcmout = fopen(argv[2], "wb");

		if(pcmout)
		{
			fwrite(audio, audioSize, sizeof(char), pcmout);

			fflush(pcmout);
			fclose(pcmout);
		}
	}

	// Get the properties of the input file.

	r = s_audio_stat(&stat, argv[1]);

	if(r < 0)
	{
		s_print_error(r);
		ret = EXIT_FAILURE;
		goto err_after_decode;
	}

#ifdef SPECTR_DEBUG
	printf("Loaded input file - %" PRIu32 " Hz / %" PRIu32 "-bit\n",
		stat.sample_rate, stat.bit_depth);
#endif

err_after_decode:
	free(audio);
done:
	return ret;
}

void s_print_error(int error)
{
	printf("Fatal error %d: %s\n", -error, strerror(-error));
}
