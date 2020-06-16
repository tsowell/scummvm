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

#include "common/mutex.h"

#include "backends/platform/dcalt/mutex.h"

OSystem::MutexRef DCAltMutexManager::createMutex()
{
	mutex_t *mutex;
	int ret;

	mutex = (mutex_t *)malloc(sizeof(mutex_t));
	if (mutex == NULL) {
		perror("malloc");
		return NULL;
	}

	ret = mutex_init(mutex, MUTEX_TYPE_RECURSIVE);
	if (ret != 0) {
		perror("mutex_init");
		return NULL;
	}

	return (OSystem::MutexRef)mutex;
}

void DCAltMutexManager::lockMutex(OSystem::MutexRef mutex)
{
	int ret;

	ret = mutex_lock((mutex_t *)mutex);
	if (ret != 0) {
		perror("mutex_lock");
	}
}

void DCAltMutexManager::unlockMutex(OSystem::MutexRef mutex)
{
	int ret;

	ret = mutex_unlock((mutex_t *)mutex);
	if (ret != 0) {
		perror("mutex_unlock");
	}
}

void DCAltMutexManager::deleteMutex(OSystem::MutexRef mutex)
{
	int ret;

	ret = mutex_destroy((mutex_t *)mutex);
	if (ret != 0) {
		perror("mutex_destroy");
	}

	free(mutex);
}
