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
#include <string.h>

#ifdef SPECTR_DEBUG
	#include <assert.h>

	#include "util/math.h"
	#include "util/bitwise.h"
	#include "util/complex.h"
#endif

#ifdef SPECTR_DEBUG
int s_naive_fft_r(s_dft_t *, const s_raw_audio_t *, size_t, size_t, size_t);
#endif

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
 * This is a utility function which creates a duplicate of an existing s_dft_t.
 *
 * The given destination will be freed (if it was previously initialized), and
 * then re-initialized to be equal to the given other s_dft_t.
 *
 * \param dst The destination for the copy.
 * \param src The s_dft_t structure to copy.
 * \return 0 on success, or an error number otherwise.
 */
int s_copy_dft(s_dft_t **dst, const s_dft_t *src)
{
	int r;

	// Make sure the destination is a newly-initialized s_dft_t.

	r = s_free_dft(dst);

	if(r < 0)
		return r;

	r = s_init_dft(dst);

	if(r < 0)
		return r;

	// Copy the length values from the original.

	(*dst)->raw_length = src->raw_length;
	(*dst)->dft_length = src->dft_length;

	// Allocate memory for the list of DFT values.

	(*dst)->dft = malloc(sizeof(s_complex_t) * src->dft_length);

	if((*dst)->dft == NULL)
	{
		s_free_dft(dst);
		return -ENOMEM;
	}

	// Copy the DFT values from the original.

	memcpy((*dst)->dft, src->dft, sizeof(s_complex_t) * src->dft_length);

	// We're done!

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
	s_complex_t v;

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
			exp = - 2.0 * M_PI * ((double) k) *
				((double) n);
			exp /= (double) N;

			s_cexp(&v, exp);
			s_cmul_r(&v, &v,
				(double) s_mono_sample(raw->samples[n]));

			dft->dft[k].r += v.r;
			dft->dft[k].i += v.i;
		}
	}

	return 0;
}

/*!
 * This function computes the DFT of the given raw audio data using a classic
 * fast Fourier transform algorithm, assuming that the length of the raw audio
 * data is a power of two.
 *
 * \param dft The s_dft_t to store the result in.
 * \param raw The raw audio data to process.
 * \return 0 on success, or an error number otherwise.
 */
int s_naive_fft(s_dft_t *dft, const s_raw_audio_t *raw)
{
	size_t i;

	// The length of the input must be a power of two for the FFT.

	if(!s_is_pow_2(raw->samples_length))
		return -EINVAL;

	// Allocate space for the output values.

	if(dft->dft != NULL)
	{
		free(dft->dft);
		dft->dft = NULL;
	}

	dft->raw_length = raw->samples_length;
	dft->dft_length = raw->samples_length;

	dft->dft = malloc(sizeof(s_complex_t) * dft->dft_length);

	if(dft->dft == NULL)
		return -ENOMEM;

	// Initialize all of the values in the DFT to 0.

	for(i = 0; i < dft->dft_length; ++i)
	{
		dft->dft[i].r = 0.0;
		dft->dft[i].i = 0.0;
	}

	// Compute the DFT using our FFT algorithm.

	return s_naive_fft_r(dft, raw, 1, 0, dft->dft_length);
}

/*!
 * This function uses the Danielson-Lanczos lemma to compute the Fourier
 * transform of the even and odd sections of the set of items in the list whose
 * index is denoted by s * k + o, where s and o are the given stride and
 * offset, and k is an integer starting from 0.
 *
 * This function is recursive, and assumes that its inputs have already been
 * setup for it in advance. One should call s_naive_fft() first, to ensure that
 * this function is called correctly.
 *
 * \param dft The dft to store the result in.
 * \param raw The raw data to transform.
 * \param s The stride to iterate over the list with.
 * \param o The offset to iterate over the list with.
 * \param N the length of the list being iterated over.
 * \return 0 on success, or an error number otherwise.
 */
int s_naive_fft_r(s_dft_t *dft, const s_raw_audio_t *raw,
	size_t s, size_t o, size_t bign)
{
	int r;

	size_t es;
	size_t eo;
	size_t os;
	size_t oo;

	size_t half;

	s_dft_t *dftprime = NULL;
	size_t idx;

	double bigw;
	s_complex_t bigwl;
	s_complex_t bigwu;

	const s_complex_t *even = NULL;
	const s_complex_t *odd = NULL;

	s_complex_t *dstl = NULL;
	s_complex_t *dstu = NULL;

	/*
	 * Noting that fft(x) = x when x is of length 1, so if we've divided to
	 * the point where we have a list of length 1, just copy the value from
	 * our raw list.
	 */

	if(bign == 1)
	{
		dft->dft[o].r = (double) s_mono_sample(raw->samples[o]);
		dft->dft[o].i = 0.0;

		return 0;
	}

	// Compute the stride and offset for the even and odd elements.

	es = 2 * s;
	eo = o;

	os = 2 * s;
	oo = o + s;

	// Compute the DFT of the even elements.

	r = s_naive_fft_r(dft, raw, es, eo, bign >> 1);

	if(r < 0)
		return r;

	// Compute the DFT of the odd elements.

	r = s_naive_fft_r(dft, raw, os, oo, bign >> 1);

	if(r < 0)
		return r;

	half = bign >> 1;

	/*
	 * We need to create a copy of the DFT values as-is, since we need to
	 * use them to combine the even and odd DFT's so that we'll have
	 * computed the correct DFT for this level.
	 */

	r = s_copy_dft(&dftprime, dft);

	if(r < 0)
		return r;

	/*
	 * Per the Danielson-Lanczos lemma, we have that:
	 *
	 *     $F_n = F^e_n + W^n F^o_n$
	 *
	 * Where:
	 *
	 *     $W = e^{-2\pi i/N}$
	 *
	 * Also, because our inputs are real, the DFT is symmetric, so the
	 * result values in the lower and upper halves are based upon the same
	 * values of $F^e_N$ and $F^o_n$. Because of this, we set 2 values per
	 * iteration, using different values of $W$ for each.
	 */

	for(idx = 0; idx < half; ++idx)
	{
		/*
		 * Compute the two $W$ values we'll use, according to the
		 * aforementioned formula.
		 */

		bigw = -2.0 * M_PI / ((double) bign);

		s_cexp(&bigwl, bigw * ((double) idx));
		s_cexp(&bigwu, bigw * ((double) (idx + half)));

		/*
		 * For brevity later on, get pointers to the even and odd
		 * values we will use in our computation.
		 */

		even = &(dftprime->dft[es * idx + eo]);
		odd = &(dftprime->dft[os * idx + oo]);

		// Compute the lower-half result value.

		dstl = &(dft->dft[s * idx + o]);
		s_cmul(dstl, &bigwl, odd);
		s_cadd(dstl, even, dstl);

		// Compute the upper-half result value.

		dstu = &(dft->dft[s * (idx + half) + o]);
		s_cmul(dstu, &bigwu, odd);
		s_cadd(dstu, even, dstu);
	}

	// Clean up our copy, and we're done.

	s_free_dft(&dftprime);

	return 0;
}
#endif
