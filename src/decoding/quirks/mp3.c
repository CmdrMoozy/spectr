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

#include "mp3.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "util/bitwise.h"

int s_is_mp3_frame_header(const uint8_t *, size_t);

/*!
 * This function attemps to locate the first valid MP3 frame header in the
 * given file. If an ID3v2 tag is present, we try to use the information it
 * contains to find a valid header. Otherwise, the file is simply searched
 * sequentially.
 *
 * \param o This will receive the offset of the first valid MP3 frame header.
 * \param f The path to the file to examine.
 * \return 0 on success, or an error number if no headers could be found.
 */
int s_get_mp3_frame_header_offset(size_t *o, const char *f)
{
	int ret = 0;
	int r;
	int fd;
	struct stat stat;
	uint8_t *file;
	size_t off = 0;
	size_t i;

	// mmap the file, since our reads will be rather random.

	fd = open(f, O_RDONLY);

	if(fd < 0)
	{
		ret = -errno;
		goto done;
	}

	r = fstat(fd, &stat);

	if(r < 0)
	{
		ret = -errno;
		goto err_after_open;
	}

	if(stat.st_size < 10)
	{
		ret = -EIO;
		goto err_after_open;
	}

	file = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

	if(file == MAP_FAILED)
	{
		ret = -errno;
		goto err_after_open;
	}

	/*
	 * If the file has an ID3v2 header, find the offset to skip it. ID3v2
	 * headers start with the bytes 0x49 0x44 0x33 ("ID3").
	 */

	if( (file[0] == 0x49) && (file[0] == 0x44) && (file[0] == 0x33) )
	{
		/*
		 * The 10-byte ID3v2 header is formatted as follows:
		 *
		 *     49 44 33 yy yy xx zz zz zz zz
		 *
		 * Where yy is the ID3v2 version (and is guaranteed to be less
		 * than 0xFF), xx is the "flags" byte and the zz bytes are the
		 * ID3v2 tag length (represented as a 32-bit synchsafe integer).
		 */

		off = (size_t) s_from_synchsafe_int32(file, 6);

		/*
		 * The tag size field excludes the length of the header, so add
		 * that value in.
		 */

		off += 10;

		/*
		 * The tag size field also excludes the length of the footer,
		 * if one is present. Check if it is present by examining the
		 * "flags" field, and then adding the length of the footer if
		 * one is found.
		 *
		 * See the ID3v2 standard for more information:
		 *
		 *     http://id3.org/id3v2.4.0-structure
		 */

		if(file[5] & 0x10)
			off += 10;
	}

	// Verify that there's a valid MP3 header at the offset we found.

	if(!s_is_mp3_frame_header(file, off))
	{
		/*
		 * Looks like we didn't find a valid MP3 frame header where we
		 * expected to, based upon the ID3v2 tag. Let's just assume the
		 * ID3v2 tag is corrupt, and its size field is incorrect. Try
		 * searching the file for the first valid MP3 frame header it
		 * contains.
		 *
		 * Note that this is not all that reliable, but can still be
		 * useful in the interest of supporting files tha are only
		 * somewhat malformed.
		 */

		for(i = 0; i < (size_t) (stat.st_size - 1); ++i)
		{
			if(s_is_mp3_frame_header(file, i))
			{
				off = i;
				break;
			}
		}
	}

	// Verify that we eventually did find an MP3 frame header, and finish.

	if(!s_is_mp3_frame_header(file, off))
	{
		ret = -EINVAL;
		goto err_after_mmap;
	}

	*o = off;

err_after_mmap:
	munmap(file, stat.st_size);
err_after_open:
	close(fd);
done:
	return ret;
}

/*!
 * This is a very simple function which tests if, at the given offset in the
 * given buffer, there is a valid MP3 frame header. MP3 frame headers always
 * start with the bytes 0xFF 0xFB -or- 0xFF 0xFA.
 *
 * \param buf The buffer to examine.
 * \param off The offset in the buffer to examine.
 * \return Whether or not an MP3 frame header is presen at the given offset.
 */
int s_is_mp3_frame_header(const uint8_t *buf, size_t off)
{
	return (buf[off] == 0xFF) &&
		( (buf[off + 1] == 0xFB) || (buf[off + 1] == 0xFA) );
}
