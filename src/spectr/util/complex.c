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

#include "complex.h"

#include <math.h>

#ifdef SPECTR_DEBUG
	#include <assert.h>
#endif

/*!
 * This function adds the two given complex values together, placing the result
 * in the given location.
 *
 * \param r This will receive the result of the computation.
 * \param a The first value to add.
 * \param b The second value to add.
 */
void s_cadd(s_complex_t *r, const s_complex_t *a, const s_complex_t *b)
{
	r->r = a->r + b->r;
	r->i = a->i + b->i;
}

/*!
 * This function subtracts the second given value from the first, placing the
 * result in the given location.
 *
 * \param r This will receive the result of the computation.
 * \param a The value being subtracted from.
 * \param b The value to subtract from the first value.
 */
void s_csub(s_complex_t *r, const s_complex_t *a, const s_complex_t *b)
{
	r->r = a->r - b->r;
	r->i = a->i - b->i;
}

/*!
 * This function multiplies the two given complex values together, placing the
 * result in the given location.
 *
 * We use the following formula, if a = m + ni and b = x + yi:
 *
 *     (m * x - n * y) + (n * x + m * y)i
 *
 * \param r This will receive the result of the computation.
 * \param a The first value to multiply.
 * \param b The second value to multiply.
 */
void s_cmul(s_complex_t *r, const s_complex_t *a, const s_complex_t *b)
{
	double real = a->r * b->r - a->i * b->i;
	double imag = a->i * b->r + a->r * b->i;

	r->r = real;
	r->i = imag;
}

/*!
 * This function multiplies the given complex value by the given real value,
 * placing the result in the given location. We use the following trivial
 * formula, where a = x + yi:
 *
 *     (x + yi) * b = b * x + (b * y)i
 *
 * \param r This will receive the result of the computation.
 * \param a The complex value to multiply.
 * \param b The real value to multiply.
 */
void s_cmul_r(s_complex_t *r, const s_complex_t *a, double b)
{
	r->r = a->r * b;
	r->i = a->i * b;
}

/*!
 * This function computes the value of e^(xi), where e is the base of the
 * natural logarithm, and x is a given real value. We use Euler's formula,
 * which gives us that:
 *
 *     e ^ (xi) = cos(x) + sin(x)i
 *
 * \param r This will receive the result of the computation.
 * \param x The real value to use in the exponent.
 */
void s_cexp(s_complex_t *r, double x)
{
	r->r = cos(x);
	r->i = sin(x);
}

/*!
 * This is a very basic function which returns the magnitude of the given
 * complex number. This is equivalent to the concept of the magnitude of a
 * 2-vector, with the real and imaginary parts of the given value making up the
 * vector components.
 *
 * \param c The complex number to compute the magnitude of.
 * \return The magnitude of the given complex number.
 */
double s_magnitude(const s_complex_t *c)
{
	double r = sqrt(c->r * c->r + c->i * c->i);

#ifdef SPECTR_DEBUG
	// Assert that we didn't just overflow or do anything illegal.

	assert(!isnan(r));
	assert(!isinf(r));
#endif

	return r;
}

/*!
 * This is a convenience function which prints the given complex value to
 * standard output, using s_cfprintf.
 *
 * \param v The value to print.
 * \return The number of characters printed, or a negative value on error.
 */
int s_cprintf(const s_complex_t *v)
{
	return s_cfprintf(stdout, v);
}

/*!
 * This function prints the given complex value to the given stream, as a
 * human-readable string.
 *
 * \param s The stream to write the rendered value to.
 * \param v The value to print.
 * \return The number of characters printed, or a negative value on error.
 */
int s_cfprintf(FILE *s, const s_complex_t *v)
{
	char sign = '+';

	if(v->i < 0.0)
		sign = '-';

	return fprintf(s, "(%f%c%fj)", v->r, sign, fabs(v->i));
}
