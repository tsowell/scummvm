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

#include <string.h>

#include "dc/maple.h"

#include "path.h"

int is_vmu(const char *path)
{
	if (strncmp(path, "/vmu/", 5) == 0) {
		if (strlen(path) >= 7) {
			if ((path[5] >= 'a' &&
			     path[5] < ('a' + MAPLE_PORT_COUNT)) &&
			    (path[6] >= '0' &&
			     path[6] < ('0' + MAPLE_UNIT_COUNT)) &&
			    (path[7] == '/')) {
				return 1;
			}
		}
	}
	return 0;
}
