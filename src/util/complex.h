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

#ifndef INCLUDE_SPECTR_UTIL_COMPLEX_H
#define INCLUDE_SPECTR_UTIL_COMPLEX_H

#include "types.h"

#include <stdio.h>
#include <stddef.h>

extern void s_cadd(s_complex_t *, const s_complex_t *, const s_complex_t *);
extern void s_csub(s_complex_t *, const s_complex_t *, const s_complex_t *);
extern void s_cmul(s_complex_t *, const s_complex_t *, const s_complex_t *);

extern void s_cmul_r(s_complex_t *, const s_complex_t *, double);

extern void s_cexp(s_complex_t *, double x);

extern int s_cprintf(const s_complex_t *);
extern int s_cfprintf(FILE *, const s_complex_t *);

#endif
