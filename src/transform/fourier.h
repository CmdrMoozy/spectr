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

#ifndef INCLUDE_SPECTR_TRANSFORM_FOURIER_H
#define INCLUDE_SPECTR_TRANSFORM_FOURIER_H

#include "types.h"

extern int s_init_dft(s_dft_t **);
extern int s_free_dft(s_dft_t **);

#ifdef SPECTR_DEBUG
extern int s_naive_dft(s_dft_t *, const s_raw_audio_t *);
#endif

#endif