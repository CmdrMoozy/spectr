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

#include "types.h"
#include "decoding/raw.h"
#include "rendering/render.h"
#include "transform/fourier.h"

#ifdef SPECTR_DEBUG
	#include <assert.h>
	#include <inttypes.h>
	#include <math.h>

	#include "decoding/stat.h"
	#include "util/math.h"
#endif

void s_print_error(int);

#ifdef SPECTR_DEBUG
	void s_test();
#endif

int main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;
	int r;
	s_raw_audio_t *audio = NULL;

#ifdef SPECTR_DEBUG
	uint32_t duration;
#endif

#ifdef SPECTR_DEBUG
	s_test();
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

	// Render the processed audio.

#ifdef SPECTR_DEBUG
	printf("Entering rendering loop...\n");
#endif

	r = render_loop();

	if(r < 0)
	{
		s_print_error(r);
		ret = EXIT_FAILURE;
		goto err_after_alloc;
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

#ifdef SPECTR_DEBUG
void s_test()
{
	s_raw_audio_t *test = NULL;
	s_dft_t *dft = NULL;
	s_complex_t v;
	int r;
	int32_t i;

	double expr[20] = {
		-10.0,
		-10.0,
		-10.0,
		-10.0,
		-10.0,
		-10.0,
		-10.0,
		-10.0,
		-10.0,
		-10.0,
		-10.0,
		-10.0,
		-10.0,
		-10.0,
		-10.0,
		-10.0,
		-10.0,
		-10.0,
		-10.0,
		-10.0
	};

	double expi[20] = {
		0.0,
		63.1375,
		30.7768,
		19.6261,
		13.7638,
		10.0,
		7.2654,
		5.0953,
		3.2492,
		1.5838,
		0.0,
		-1.5838,
		-3.2492,
		-5.0953,
		-7.2654,
		-10.0,
		-13.7638,
		-19.6261,
		-30.7768,
		-63.1375
	};

	printf("DEBUG: Testing DFT computation...\n");

	// Populate our test audio with our test data.

	r = s_init_raw_audio(&test);
	assert(r == 0);

	test->stat.type = FTYPE_MP3;
	test->stat.bit_depth = 32;
	test->stat.sample_rate = 44100;

	test->samples_length = 20;
	test->samples = malloc(sizeof(s_stereo_sample_t) * 20);
	assert(test->samples != NULL);

	for(i = -10; i < 10; ++i)
	{
		test->samples[i + 10].l = i;
		test->samples[i + 10].r = i;
	}

	for(i = 0; i < 20; ++i)
		assert(s_mono_sample(test->samples[i]) == i - 10);

	// Compute the DFT of the test audio.

	r = s_init_dft(&dft);
	assert(r == 0);

	r = s_naive_dft(dft, test);
	assert(r == 0);

	// Verify that the computation produced the correct output.

	for(i = 0; i < 20; ++i)
	{
		r = s_get_dft_value(&v, dft, i);
		assert(r == 0);

		printf("\tX(%d): %f + %fi\n", i, v.r, v.i);

		assert(fabs(v.r - expr[i]) < 0.0001);
		assert(fabs(v.i - expi[i]) < 0.0001);
	}

	s_free_raw_audio(&test);
	s_free_dft(&dft);

	printf("DEBUG: DFT computation verified successfully!\n\n");
}
#endif
