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

#include "decoding/decode.h"

int main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;
	char *audio = NULL;
	size_t audioSize;
	int r;
	FILE *pcmout;

	if(argc < 2)
	{
		printf("Usage: spectr <file to analyze> [PCM out file]\n");

		ret = EXIT_FAILURE;
		goto done;
	}

	// Decode the input file we were given.

	r = decode(&audio, &audioSize, argv[1]);

	if(r != 0)
	{
		printf("Fatal error %d: %s\n", -r, strerror(-r));

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

err_after_decode:
	free(audio);
done:
	return ret;
}
