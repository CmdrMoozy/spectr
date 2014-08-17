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

#include "path.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>

/*!
 * This function places the absolute path to our own executable in the given
 * buffer. This is implemented by examining the /proc/self/exe symlink, and as
 * such only works on Linux.
 *
 * \param buf The buf to store our path in.
 * \param bufsiz The size of the given buffer.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_get_own_path(char *buf, size_t bufsiz)
{
	ssize_t l;

	if(bufsiz == 0)
		return -EINVAL;

	l = readlink("/proc/self/exe", buf, bufsiz - 1);

	if(l == -1)
		return -errno;

	buf[l] = '\0';

	return 0;
}

/*!
 * This function places the absolute path to the directory containing our own
 * executable in the given buffer. This is implemented by examining the
 * /proc/self/exe symlink, and as such only works on Linux.
 *
 * \param buf The buf to store the path to our parent directory in.
 * \param bufsiz The size of the given buffer.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_get_own_dir(char *buf, size_t bufsiz)
{
	int r;
	size_t i;
	char *last;

	// Get the path to our own process.

	r = s_get_own_path(buf, bufsiz);

	if(r < 0)
		return r;

	// Strip any trailing slashes.

	for(i = strlen(buf); i > 0; --i)
	{
		if(buf[i - 1] != '/')
			break;

		buf[i - 1] = '\0';
	}

	// Truncate the string at the last '/' character.

	last = strrchr(buf, '/');

	if(last == NULL)
		return -EIO;

	*last = '\0';

	// Done!

	return 0;
}
