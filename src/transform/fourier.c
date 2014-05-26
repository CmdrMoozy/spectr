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

#include "fourier.h"

#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include "util/math.h"

/*!
 * This function initializes (allocates) a s_dft_t variable. If the pointer is
 * non-NULL, we will not allocate a new value on top of it.
 *
 * \param dft The s_dft_t to allocate.
 * \return 0 on success, or an error number otherwise.
 */
int s_init_dft(s_dft_t **dft)
{
	if(*dft != NULL)
		return -EINVAL;

	*dft = malloc(sizeof(s_dft_t));

	if(dft == NULL)
		return -ENOMEM;

	(*dft)->dft_length = 0;
	(*dft)->dft = NULL;

	return 0;
}

/*!
 * This function frees the given s_dft_t structure, including the list of
 * values it contains (if any). Note that this function is safe against
 * double-frees.
 *
 * \param r The s_dft_t to free.
 * \return 0 on success, or an error number otherwise.
 */
int s_free_dft(s_dft_t **dft)
{
	if(*dft == NULL)
		return 0;

	if((*dft)->dft != NULL)
	{
		free((*dft)->dft);
		(*dft)->dft = NULL;
	}

	free(*dft);
	*dft = NULL;

	return 0;
}

/*!
 * This function returns the value X(n) of the given DFT output. Note that,
 * because we compute DFT's from real inputs, the DFT has the following
 * symmetry condition:
 *
 *     X(k) = X(N-k)*, k = 0, 1, ..., N-1
 *
 * Where * is the complex conjugate, i.e.:
 *
 *     (a + bi)* = a - bi
 *
 * This function takes advantage of this symmetry automatically, returning the
 * appropriate value, even though our DFT stuctures only store the first
 * N/2 + 1 values explicitly.
 *
 * \param v The complex variable whose value will be set.
 * \param dft The DFT result to get the value from.
 * \param n The index of the value to get.
 * \return 0 on success, or an error number otherwise.
 */
int s_get_dft_value(s_complex_t *v, const s_dft_t *dft, size_t n)
{
	size_t src;
	double conj = 1.0;

	if(dft->dft_length == 0)
		return -EINVAL;

	if(dft->dft_length != (dft->raw_length >> 1) + 1)
		return -EINVAL;

	if(n > dft->raw_length)
		return -EINVAL;

	src = n;

	if(src >= dft->dft_length)
	{
		src = dft->raw_length - src;
		conj = -1.0;
	}

	v->r = dft->dft[src].r;
	v->i = dft->dft[src].i;
	v->i *= conj;

	return 0;
}

#ifdef SPECTR_DEBUG
/*!
 * This function computes the discrete Fourier transform of the given raw audio
 * data, using the most basic, naive implementation possible. This is primarily
 * useful for testing purposes (e.g., to validate the computations of more
 * efficient, more complex functions).
 *
 * \param dft The pre-initialized DFT structure to store the result in.
 * \param raw The raw audio data to compute the DFT of.
 * \return 0 on success, or an error number otherwise.
 */
int s_naive_dft(s_dft_t *dft, const s_raw_audio_t *raw)
{
	size_t n;
	size_t k;
	size_t N;
	double exp;
	double r;
	double i;
	double xn;

	// We require an even number of input samples.

	if(raw->samples_length & 1)
		return -EINVAL;

	// Allocate space for the output values.

	if(dft->dft != NULL)
	{
		free(dft->dft);
		dft->dft = NULL;
	}

	dft->raw_length = raw->samples_length;
	dft->dft_length = (raw->samples_length >> 1) + 1;

	dft->dft = malloc(sizeof(s_complex_t) * dft->dft_length);

	if(dft->dft == NULL)
		return -ENOMEM;

	// Compute the first N/2 + 1 members of the DFT.

	N = raw->samples_length;

	for(k = 0; k <= (N >> 1); ++k)
	{
		dft->dft[k].r = 0.0;
		dft->dft[k].i = 0.0;

		for(n = 0; n < N; ++n)
		{
			xn = (double) s_mono_sample(raw->samples[n]);

			exp = - 2.0 * M_PI * ((double) k) *
				((double) n);
			exp /= (double) N;

			r = cos(exp) * xn;
			i = sin(exp) * xn;

			dft->dft[k].r += r;
			dft->dft[k].i += i;
		}
	}

	return 0;
}
#endif
