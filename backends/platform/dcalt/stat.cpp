/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <sys/stat.h>
#include <kos/fs.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

#include "path.h"

extern "C"

// Workaround for KallistiOS's stat stubs
int stat(const char *path, struct stat *buf) {
	char dir[PATH_MAX];
	const char *filename;
	DIR *dirp;
	struct dirent *dp;
	char *slash;
	int err = errno, rv;
	file_t fp;
	mode_t md;

	// KallistiOS stat breaks on / and on top-level directories
	if (path[0] == '/' && !index(&path[1], '/')) {
		buf->st_mode = S_IFDIR;
		return 0;
	}

	// KOS doesn't implement stat for VMU files, but we also want to avoid
	// opening them because it loads the entire file on open.
	if (is_vmu(path)) {
		slash = rindex(path, '/');
		strncpy(dir, path, (slash - path));
		dir[slash - path] = '\0';
		filename = &slash[1]; 

		dirp = opendir(dir);
		if (dirp == NULL) {
			return -1;
		}

		do {
			dp = readdir(dirp);
			if ((dp != NULL) &&
			    (strcmp(filename, dp->d_name) == 0)) {
				closedir(dirp);
				return 0;
			}
		} while (dp != NULL);
		closedir(dirp);
		return -1;
	}

	// The rest is from kernel/libc/newlib/newlib_stat.c

	/* Try to use the native stat function first... */
	if(!(rv = fs_stat(path, buf, 0)) || errno != ENOSYS)
		return rv;

	/* If this filesystem doesn't implement stat, then fake it to get a few
	   important pieces... */
	errno = err;
	fp = fs_open(path, O_RDONLY);
	md = S_IFREG;

	/* If we couldn't get it as a file, try as a directory */
	if(fp == FILEHND_INVALID) {
		fp = fs_open(path, O_RDONLY | O_DIR);
		md = S_IFDIR;
	}

	/* If we still don't have it, then we're not going to get it. */
	if(fp == FILEHND_INVALID) {
		errno = ENOENT;
		return -1;
	}

	/* This really doesn't convey all that much information, but it should help
	   with at least some uses of stat. */
	buf->st_mode = md;
	buf->st_size = (off_t)fs_total(fp);

	/* Clean up after ourselves. */
	fs_close(fp);

	return 0;
}
