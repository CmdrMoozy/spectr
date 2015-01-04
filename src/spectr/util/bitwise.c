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

/*!
 * This function turns the right-most one bit of the given value off, returning
 * the result.
 *
 * \param v The value to manipulate.
 * \return The given value, with the right-most one bit turned off.
 */
uint64_t s_rmo_off(uint64_t v)
{
	return (v & (v - 1));
}

/*!
 * This function returns whether or not the given value is a power of two.
 *
 * \param v The value to examine.
 * \return Whether or not v is a power of two.
 */
int s_is_pow_2(uint64_t v)
{
	return (v && !s_rmo_off(v));
}

/*!
 * This function returns the largest power of two less than or equal to the
 * given value. More specifically:
 *
 *   s_flp2(0) = 0
 *   s_flp2(x) = 2 ^ floor(lg(x))
 *
 * This algorithm propagates the leftmost 1-bit in order to turn off all bits
 * beside that one. This algorithm comes from "Hacker's Delight" Chapter 3 -
 * Power-of-2 Boundaries.
 *
 * \param v The value to power-of-two floor.
 * \return The floored value.
 */
uint64_t s_flp2(uint64_t v)
{
	v = v | (v >> 1);
	v = v | (v >> 2);
	v = v | (v >> 4);
	v = v | (v >> 8);
	v = v | (v >> 16);
	v = v | (v >> 32);

	return v - (v >> 1);
}
