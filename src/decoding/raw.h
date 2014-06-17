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

#ifndef INCLUDE_SPECTR_DECODING_RAW_H
#define INCLUDE_SPECTR_DECODING_RAW_H

#include <stdio.h>

#include "types.h"

extern int s_init_raw_audio(s_raw_audio_t **);
extern void s_free_raw_audio(s_raw_audio_t **);

extern int s_copy_raw_audio(s_raw_audio_t **, const s_raw_audio_t *);
extern int s_copy_raw_audio_window(s_raw_audio_t **, const s_raw_audio_t *,
	size_t, size_t);
extern int s_decode_raw_audio(s_raw_audio_t *, const char *);

extern int s_write_raw_audio(FILE *, const s_raw_audio_t *);

#endif
