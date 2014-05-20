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

#include "bitwise.h"

/*!
 * This function converts a "32-bit synchsafe integer" found e.g. in MP3
 * headers, and converts it to a standard 32-bit integer type.
 *
 * These "synchsafe integers" are more or less 32-bit integers, except every
 * 8th bit is ignored. So:
 *
 *     1111 1111 1111 1111 (0xFFFF) =>  0011 0111 1111 0111 1111 (0x37F7F)
 *
 * \param buf The buffer containing the raw data.
 * \param o The offset in the buffer to start at.
 * \return The value given as a normal 32-bit integer.
 */
uint32_t s_from_synchsafe_int32(const uint8_t *buf, size_t o)
{
	uint32_t result = 0;

	result |= ((uint32_t) (buf[o + 0] & 0x7F)) << 21;
	result |= ((uint32_t) (buf[o + 1] & 0x7F)) << 14;
	result |= ((uint32_t) (buf[o + 2] & 0x7F)) << 7;
	result |= (uint32_t) (buf[o + 3] & 0x7F);

	return result;
}
