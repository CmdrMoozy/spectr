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

#ifndef INCLUDE_SPECTR_CONFIG_H
#define INCLUDE_SPECTR_CONFIG_H

/*
 * This defines the number of frames we're willing to render each second.
 */
#define S_WINDOW_FPS 20

/*
 * This is the size of the window we'll plot our result in.
 */
#define S_WINDOW_W 800
#define S_WINDOW_H 250

/*
 * These values define some properties of our spectrogram legend.
 */
#define S_SPEC_LGND_TICK_SIZE 7

#endif
