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
#endif

#include "decoding/raw.h"
#include "util/math.h"
#include "util/bitwise.h"
#include "util/complex.h"

int s_fft_r(s_dft_t *, size_t, const s_raw_audio_t *, size_t, size_t, size_t,
	double (*)(int32_t, size_t), size_t, size_t);

/*!
 * This function implements the Hann windowing function. For more information,
 * see:
 *
 *     http://en.wikipedia.org/wiki/Hann_function
 *     http://en.wikipedia.org/wiki/Window_function
 *
 * \param n The index of the sample to window.
 * \param N the total number of samples in this window's range.
 * \return The Hann window coefficient for the given sample.
 */
double s_hann_function(int32_t n, size_t N)
{
	double nd = (double) n;
	double Nd = (double) N;
	double ret;

	ret = 2.0 * M_PI * nd;
	ret /= (Nd - 1.0);
	ret = cos(ret);
	ret = 1.0 - ret;
	ret = 0.5 * ret;

	return ret;
}

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

	if(*dft == NULL)
		return -ENOMEM;

	(*dft)->length = 0;
	(*dft)->dft = NULL;

	return 0;
}

/*!
 * This function frees the given s_dft_t structure, including the list of
 * values it contains (if any). Note that this function is safe against
 * double-frees.
 *
 * \param r The s_dft_t to free.
 */
void s_free_dft(s_dft_t **dft)
{
	if(*dft == NULL)
		return;

	if((*dft)->dft != NULL)
	{
		free((*dft)->dft);
		(*dft)->dft = NULL;
	}

	free(*dft);
	*dft = NULL;
}

/*!
 * This is a utility function which initializes the contents of the given DFT
 * structure to be able to store a result of the given length.
 *
 * \param dft The DFT whose contents will be initialized.
 * \param length The length of the result that is to be stored.
 * \return 0 on success, or an error number otherwise.
 */
int s_init_dft_result(s_dft_t *dft, size_t length)
{
	size_t i;

	if(length < 1)
		return -EINVAL;

	// Free the existing result, if any.

	if(dft->dft != NULL)
	{
		free(dft->dft);
		dft->dft = NULL;
	}

	// Allocate memory for the new result.

	dft->length = length;

	dft->dft = malloc(sizeof(s_complex_t) * dft->length);

	if(dft->dft == NULL)
		return -ENOMEM;

	// Initialize the result values to 0.

	for(i = 0; i < dft->length; ++i)
	{
		dft->dft[i].r = 0.0;
		dft->dft[i].i = 0.0;
	}

	// Done.

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

	s_free_dft(dst);

	r = s_init_dft(dst);

	if(r < 0)
		return r;

	// Copy the length values from the original.

	(*dst)->length = src->length;

	// Allocate memory for the list of DFT values.

	(*dst)->dft = malloc(sizeof(s_complex_t) * src->length);

	if((*dst)->dft == NULL)
	{
		s_free_dft(dst);
		return -ENOMEM;
	}

	// Copy the DFT values from the original.

	memcpy((*dst)->dft, src->dft, sizeof(s_complex_t) * src->length);

	// We're done!

	return 0;
}

/*!
 * This function computes the DFT of a part of the given raw audio data using a
 * classic fast Fourier transform algorithm, assuming that the length of the
 * raw audio data is a power of two.
 *
 * Note that this function will free any existing contents in the given result
 * destination, and will allocate memory for the new result. It is up to our
 * caller to free that memory later.
 *
 * \param dft The s_dft_t to store the result in.
 * \param raw The raw audio data to process.
 * \param o The offset to start processing the raw audio data from.
 * \param l The length of the raw audio data to process.
 * \param wfn The window function to use, or NULL.
 * \return 0 on success, or an error number otherwise.
 */
int s_fft_part(s_dft_t **dft, const s_raw_audio_t *raw, size_t o, size_t l,
	double (*wfn)(int32_t, size_t))
{
	int r;

	// The length of the input must be a power of two for the FFT.

	if(!s_is_pow_2(l))
		return -EINVAL;

	// Allocate space for the output values.

	s_free_dft(dft);

	r = s_init_dft(dft);

	if(r < 0)
		return r;

	s_init_dft_result(*dft, l);

	// Compute the DFT using our FFT algorithm.

	r = s_fft_r(*dft, o, raw, 1, o, l, wfn, o, l);

	if(r < 0)
	{
		s_free_dft(dft);
		return r;
	}

	return 0;
}

/*!
 * This function computes the DFT of the given raw audio data. This is a
 * convenience function, and it simply calls s_fft_part with an offset of 0,
 * and the total length of the given raw audio data.
 *
 * \param dft The s_dft_t to store the result in.
 * \param raw The raw audio data to process.
 * \return 0 on success, or an error number otherwise.
 */
int s_fft(s_dft_t **dft, const s_raw_audio_t *raw)
{
	return s_fft_part(dft, raw, 0, raw->samples_length, NULL);
}

/*!
 * This function initializes (allocates) a s_stft_t variable. If the pointer
 * is non-NULL, we will not allocate a new value on top of it.
 *
 * \param dft The s_stft_t to allocate.
 * \return 0 on success, or an error number otherwise.
 */
int s_init_stft(s_stft_t **stft)
{
	if(*stft != NULL)
		return -EINVAL;

	*stft = malloc(sizeof(s_stft_t));

	if(*stft == NULL)
		return -ENOMEM;

	(*stft)->raw_length = 0;

	(*stft)->raw_stat.type = FTYPE_INVALID;
	(*stft)->raw_stat.bit_depth = 0;
	(*stft)->raw_stat.sample_rate = 0;

	(*stft)->window = 0;
	(*stft)->length = 0;
	(*stft)->dfts = NULL;

	return 0;
}

/*!
 * This function frees the given s_stft_t structure, including the list of DFT
 * values it contains (if any). Note that this function is safe against
 * double-frees.
 *
 * \param r The s_stft_t to free.
 */
void s_free_stft(s_stft_t **stft)
{
	if(*stft == NULL)
		return;

	s_free_stft_result(*stft);

	free(*stft);
	*stft = NULL;
}

/*!
 * This is a utility function which initializes the contents of the given STFT
 * structure to be able to store a result computed from the given raw audio
 * structure, and using the given window size.
 *
 * \param stft The STFT whose contents will be initialized.
 * \param raw The raw audio structure to be processed.
 * \param w The size of the STFT window. Must be a power of two.
 * \param o The amount of overlap of each window.
 * \return 0 on success, or an error number otherwise.
 */
int s_init_stft_result(s_stft_t *stft, const s_raw_audio_t *raw,
	size_t w, size_t o)
{
	int r;
	size_t i;
	s_dft_t **dft;

	// The length of the window must be a power of two for the FFT.

	if(!s_is_pow_2(w))
		return -EINVAL;

	// Allocate memory for the list of DFT results.

	stft->raw_length = raw->samples_length;
	stft->raw_stat = raw->stat;

	stft->window = w;
	stft->length = raw->samples_length / (w - o);

	stft->dfts = malloc(sizeof(s_dft_t *) * stft->length);

	if(stft->dfts == NULL)
		return -ENOMEM;

	for(i = 0; i < stft->length; ++i)
		stft->dfts[i] = NULL;

	// Allocate each of the DFT result structures.

	for(i = 0; i < stft->length; ++i)
	{
		dft = &(stft->dfts[i]);

		r = s_init_dft(dft);

		if(r < 0)
		{
			s_free_stft_result(stft);
			return r;
		}

		r = s_init_dft_result(*dft, w);

		if(r < 0)
		{
			s_free_stft_result(stft);
			return r;
		}
	}

	return 0;
}

/*!
 * This is a utility function which frees memory allocated for the contents of
 * the given STFT structure.
 *
 * \param stft The STFT whose contents were previously initialized.
 */
void s_free_stft_result(s_stft_t *stft)
{
	size_t i;

	if(stft->dfts != NULL)
	{
		for(i = 0; i < stft->length; ++i)
			if(stft->dfts[i] != NULL)
				s_free_dft(&(stft->dfts[i]));

		free(stft->dfts);
		stft->dfts = NULL;
	}

	stft->raw_length = 0;

	stft->raw_stat.type = FTYPE_INVALID;
	stft->raw_stat.bit_depth = 0;
	stft->raw_stat.sample_rate = 0;

	stft->length = 0;
}

/*!
 * This function computes a set of short-time Fourier transforms of the given
 * raw signal, using the given window function size.
 *
 * Note that this function will free any existing contents in the given result
 * destination, and will allocate memory for the new result. It is up to our
 * caller to free that memory later.
 *
 * \param stft This will receive the result of our computations.
 * \param raw The raw audio signal to process.
 * \param w The window function size. Must be a power of two.
 * \param o The overlap of each window.
 * \return 0 on success, or an error number otherwise.
 */
int s_stft(s_stft_t **stft, const s_raw_audio_t *raw, size_t w, size_t o)
{
	int r;
	size_t i;

	// Allocate space for the result.

	s_free_stft(stft);

	r = s_init_stft(stft);

	if(r < 0)
		return r;

	r = s_init_stft_result(*stft, raw, w, o);

	if(r < 0)
	{
		s_free_stft(stft);
		return r;
	}

	// Compute the DFT of each individual window.

	for(i = 0; i < (*stft)->length; ++i)
	{
		// Compute the DFT of this raw audio window.

		r = s_fft_part(&((*stft)->dfts[i]), raw, i * (w - o), w,
			s_hann_function);

		if(r < 0)
		{
			s_free_stft(stft);
			return r;
		}
	}

	// Done!

	return 0;
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
 * \param dfto The amount to subtract from the offset for the DFT.
 * \param raw The raw data to transform.
 * \param s The stride to iterate over the list with.
 * \param o The offset to iterate over the list with.
 * \param bign the length of the list being iterated over.
 * \param wfn The window function to use, or NULL.
 * \param orgO The offset given to the first call in this recursive chain.
 * \param orgN The N length given to the first call in this recursive chain.
 * \return 0 on success, or an error number otherwise.
 */
int s_fft_r(s_dft_t *dft, size_t dfto, const s_raw_audio_t *raw,
	size_t s, size_t o, size_t bign, double (*wfn)(int32_t, size_t),
	size_t orgO, size_t orgN)
{
	int r;

	double window;

	size_t es;
	size_t eo;
	size_t os;
	size_t oo;

	size_t half;

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
		dft->dft[o - dfto].r = (double) s_mono_sample(raw->samples[o]);
		dft->dft[o - dfto].i = 0.0;

		if(wfn != NULL)
		{
			window = wfn(o - orgO, orgN);
			dft->dft[o - dfto].r *= window;
		}

		return 0;
	}

	// Compute the stride and offset for the even and odd elements.

	es = 2 * s;
	eo = o;

	os = 2 * s;
	oo = o + s;

	// Compute the DFT of the even elements.

	r = s_fft_r(dft, dfto, raw, es, eo, bign / 2, wfn, orgO, orgN);

	if(r < 0)
		return r;

	// Compute the DFT of the odd elements.

	r = s_fft_r(dft, dfto, raw, os, oo, bign / 2, wfn, orgO, orgN);

	if(r < 0)
		return r;

	half = bign / 2;

	/*
	 * We need to create a copy of the DFT values as-is, since we need to
	 * use them to combine the even and odd DFT's so that we'll have
	 * computed the correct DFT for this level.
	 */

	s_complex_t *dftprime = malloc(sizeof(s_complex_t) * bign);

	if(dftprime == NULL)
		return -ENOMEM;

	for(idx = 0; idx < bign; ++idx)
		dftprime[idx] = dft->dft[s * idx + o - dfto];

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

		even = &(dftprime[2 * idx]);
		odd = &(dftprime[2 * idx + 1]);

		// Compute the lower-half result value.

		dstl = &(dft->dft[s * idx + o - dfto]);
		s_cmul(dstl, &bigwl, odd);
		s_cadd(dstl, even, dstl);

		// Compute the upper-half result value.

		dstu = &(dft->dft[s * (idx + half) + o - dfto]);
		s_cmul(dstu, &bigwu, odd);
		s_cadd(dstu, even, dstu);

#ifdef SPECTR_DEBUG
		// Make sure we didn't overflow or do anything wrong.

		assert(!isnan(even->r));
		assert(!isinf(even->r));

		assert(!isnan(even->i));
		assert(!isinf(even->i));

		assert(!isnan(odd->r));
		assert(!isinf(odd->r));

		assert(!isnan(odd->i));
		assert(!isinf(odd->i));
#endif
	}

	// Done!

	free(dftprime);

	return 0;
}
