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

#ifndef INCLUDE_SPECTR_DECODING_STAT_H
#define INCLUDE_SPECTR_DECODING_STAT_H

#include <stdint.h>
#include <stddef.h>

#include "spectr/types.h"

extern int s_audio_stat(s_audio_stat_t *, const char *);

extern uint32_t s_audio_duration_sec(const s_audio_stat_t *, size_t);
extern int s_audio_duration_str(char *, size_t, const s_audio_stat_t *, size_t);

extern uint32_t s_nyquist_frequency(const s_audio_stat_t *);
extern int s_nyquist_frequency_str(char *, size_t, const s_audio_stat_t *);

#endif
