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

#include "fonts.h"

#include <stdlib.h>
#include <linux/limits.h>
#include <string.h>
#include <errno.h>

#include "spectr/util/path.h"

/*!
 * This function places the absolute path to our monospace TTF font file in the
 * given buffer. It is assumed that this file can be found in a "fonts"
 * directory, in the same directory as our executable.
 *
 * Note that this function does NOT check if the font file actually exists; it
 * is up to our caller to handle this case.
 *
 * \param buf The buffer to place the path in.
 * \param bufsiz The size of the given buffer.
 * \return 0 on success, or an error number if something goes wrong.
 */
int s_get_mono_font_path(char *buf, size_t bufsiz)
{
	int r;
	char fontdir[PATH_MAX];
	const char *relfont = "/fonts/DroidSansMono.ttf";

	// Get the directory our executable is inside of.

	r = s_get_own_dir(fontdir, PATH_MAX);

	if(r < 0)
		return r;

	// Add the expected font path onto the end of the string.

	if(strlen(fontdir) + strlen(relfont) + 1 > PATH_MAX)
		return -ENOMEM;

	strcpy(fontdir + strlen(fontdir), relfont);

	// Copy the final path into the buffer the caller gave us.

	if(strlen(fontdir) >= bufsiz)
		return -EINVAL;

	strcpy(buf, fontdir);

	return 0;
}
