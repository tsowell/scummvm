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

// This define lets us use the system function remove() on Symbian, which
// is disabled by default due to a macro conflict.
// See backends/platform/symbian/src/portdefs.h .

#include "common/scummsys.h"

#include "common/file.h"
#include "common/system.h"

#include "backends/platform/dcalt/saves.h"

#include "common/savefile.h"
#include "common/util.h"
#include "common/fs.h"
#include "common/archive.h"
#include "common/config-manager.h"
#include "common/zlib.h"
#include "common/memstream.h"
#include "common/stream.h"
#include "common/md5.h"

#include <errno.h>	// for removeSavefile()
#include <dirent.h>

#include <dc/maple.h>
#include <dc/maple/vmu.h>
#include <dc/vmu_pkg.h>
#include <dc/vmufs.h>

#include "path.h"

#define MAX(x, y) ((x >= y) ? x : y)
#define MIN(x, y) ((x <= y) ? x : y)

// Copied from KallistiOS/kernel/arch/dreamcast/fs/fs_vmu.c
/* Take a VMUFS path and return the requested address */
static maple_device_t * vmu_path_to_addr(const char *p) {
    char port;

    if(p[0] != '/') return NULL;            /* Only absolute paths */

    port = p[1] | 32;               /* Lowercase the port */

    if(port < 'a' || port > 'd') return NULL;   /* Unit A-D, device 0-5 */

    if(p[2] < '0' || p[2] > '5') return NULL;

    return maple_enum_dev(port - 'a', p[2] - '0');
}

// Copied from KallistiOS/kernel/arch/dreamcast/util/vmu_pkg.c
static int vmu_eyecatch_size(int eyecatch_type) {
    switch(eyecatch_type) {
        case VMUPKG_EC_NONE:
            return 0;
        case VMUPKG_EC_16BIT:
            return 72 * 56 * 2;
        case VMUPKG_EC_256COL:
            return 512 + 72 * 56;
        case VMUPKG_EC_16COL:
            return 32 + 72 * 56 / 2;
        default:
            return -1;
    }
}

#include "icon.c"

VMUSaveFileWriteStream::VMUSaveFileWriteStream(
    DCAltSaveFileManager *savefileManager,
    maple_device_t *dev,
    const Common::String &shortFilename,
    const Common::String &longFilename,
    int existingSize) {
	int icon_size, ec_size, free_bytes;
	uint32 save_offset;

	_savefileManager = savefileManager;
	_shortFilename = shortFilename;
	_longFilename = longFilename;
	_dev = dev;

	free_bytes = vmufs_free_blocks(_dev) * 512 + existingSize;

	memset(&_pkg, 0, sizeof(struct vmu_pkg));
	memset(_pkg.desc_short, ' ', 16);
	memcpy(_pkg.desc_short, "ScummVM", strlen("ScummVM"));
	memset(_pkg.desc_long, ' ', 32);
	memcpy(_pkg.desc_long,
	       _longFilename.c_str(),
	       MAX(_longFilename.size(), 32));
	memset(_pkg.app_id, '\0', 16);
	strcpy(_pkg.app_id, "ScummVM");
	_pkg.icon_cnt = 1;
	_pkg.icon_anim_speed = 0;
	_pkg.eyecatch_type = VMUPKG_EC_NONE;
	_pkg.data_len = 0;
	memcpy(&_pkg.icon_pal, &icon_palette, sizeof(uint16) * 16);
	_pkg.icon_data = (const uint8 *)&icon_pixels;
	_pkg.eyecatch_data = NULL;

	icon_size = 512 * _pkg.icon_cnt;
	ec_size = vmu_eyecatch_size(_pkg.eyecatch_type);
	_hdr_size = sizeof(struct vmu_hdr) + icon_size + ec_size;

	_data = (uint8 *)malloc(free_bytes - _hdr_size);

	// Save preceded by offset and long filename with null terminator
	save_offset = sizeof(uint32) + _longFilename.size() + 1;

	memcpy(_data, &save_offset, sizeof(uint32));
	strcpy(((char *)_data) + sizeof(uint32), _longFilename.c_str());

	_saveBuf = _data + save_offset;

	_pkg.data = _data;
	_pkg.data_len = save_offset;

	_stream = new Common::MemoryWriteStream(
	    _saveBuf, free_bytes - _hdr_size - _pkg.data_len);
}

VMUSaveFileWriteStream::~VMUSaveFileWriteStream() {
	uint8 *buf;
	int bufSize;
	int ret;

	if (_stream->err()) {
		free(_data);
		free(_stream);
		return;
	}

	_pkg.data_len += _stream->pos();

	ret = vmu_pkg_build(&_pkg, &buf, &bufSize);
	if (ret < 0) {
		free(_data);
		free(_stream);
		return;
	}

	vmufs_write(_dev, _shortFilename.c_str(),
	            buf, bufSize, VMUFS_OVERWRITE);

	free(_data);
	free(_stream);

	VMUSaveFile *save = new VMUSaveFile(buf, bufSize, _shortFilename);
	_savefileManager->cacheNewSave(_longFilename, save);

	free(buf);
}

VMUSaveFile::VMUSaveFile() {
	_buf = NULL;
	_size = 0;
}

VMUSaveFile::VMUSaveFile(
    maple_device_t *dev, const Common::String &shortFilename) {
	uint8 *buf;
	int size;

	vmufs_read(dev, shortFilename.c_str(), (void **)&buf, &size);
	init(buf, size, shortFilename);

	free(buf);
}

VMUSaveFile::VMUSaveFile(
    uint8 *buf, int size, const Common::String &shortFilename) {
	init(buf, size, shortFilename);
}

void VMUSaveFile::init(
    uint8 *buf, int size, const Common::String &shortFilename) {
	const uint32 *save_offset;

	_shortFilename = shortFilename;

	_size = size;
	_buf = (uint8 *)malloc(_size);
	memcpy(_buf, buf, _size);

	vmu_pkg_parse(_buf, &_pkg);

	save_offset = (const uint32 *)_pkg.data;
	_saveBuf = _pkg.data + *save_offset;

	_saveSize = _pkg.data_len - (_saveBuf - _pkg.data);
}

VMUSaveFile::~VMUSaveFile() {
	if (_buf)
		free(_buf);
}

Common::String VMUSaveFile::getShortFilename() const {
	return _shortFilename;
}

Common::String VMUSaveFile::getLongFilename() const {
	return Common::String(((const char *)_pkg.data) + sizeof(uint32));
}

Common::SeekableReadStream *VMUSaveFile::createReadStream() const {
	return new Common::MemoryReadStream(_saveBuf, _saveSize);
}

Common::WriteStream *VMUSaveFile::createWriteStream(
    DCAltSaveFileManager *savefileManager, maple_device_t *dev) const {
	return new VMUSaveFileWriteStream(
	    savefileManager, dev, getShortFilename(), getLongFilename(), _size);
}

DCAltSaveFileManager::DCAltSaveFileManager() :
	DefaultSaveFileManager(),
	_rnd("DCAltSaveFileManager") {
}

Common::String DCAltSaveFileManager::makeSavefileName(
    const Common::String &savePathName) {
	const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	Common::String name;
	do {
		name = "SCUMMVM_";
		name += chars[_rnd.getRandomNumber(sizeof(chars) - 2)];
		name += chars[_rnd.getRandomNumber(sizeof(chars) - 2)];
		name += chars[_rnd.getRandomNumber(sizeof(chars) - 2)];
		name += chars[_rnd.getRandomNumber(sizeof(chars) - 2)];
	} while (_saveFileCache.find(name) != _saveFileCache.end());
	return name;
}

Common::InSaveFile *DCAltSaveFileManager::openRawFile(
    const Common::String &filename) {
	if (!(strncmp(getSavePath().c_str(), "/vmu", 4) == 0))
		return DefaultSaveFileManager::openRawFile(filename);

	assureCached(getSavePath());
        if (getError().getCode() != Common::kNoError)
                return nullptr;

        SaveFileCache::const_iterator file = _saveFileCache.find(filename);
        if (file == _saveFileCache.end()) {
                return nullptr;
        } else {
                // Open the file for loading.
                return file->_value.createReadStream();
        }
}

Common::InSaveFile *DCAltSaveFileManager::openForLoading(
    const Common::String &filename) {
	if (!(strncmp(getSavePath().c_str(), "/vmu", 4) == 0))
		return DefaultSaveFileManager::openForLoading(filename);

	assureCached(getSavePath());
        if (getError().getCode() != Common::kNoError)
                return nullptr;

        SaveFileCache::const_iterator file = _saveFileCache.find(filename);
        if (file == _saveFileCache.end()) {
                return nullptr;
        } else {
                // Open the file for loading.
                Common::SeekableReadStream *sf =
		    file->_value.createReadStream();
                return Common::wrapCompressedReadStream(sf);
        }
}

Common::OutSaveFile *DCAltSaveFileManager::openForSaving(
    const Common::String &filename, bool compress) {
	if (!(strncmp(getSavePath().c_str(), "/vmu", 4) == 0))
		return DefaultSaveFileManager::openForSaving(filename);

	assureCached(getSavePath());
        if (getError().getCode() != Common::kNoError)
                return nullptr;

	maple_device_t *dev = vmu_path_to_addr(&getSavePath().c_str()[4]);

        SaveFileCache::const_iterator file = _saveFileCache.find(filename);
        if (file == _saveFileCache.end()) {
		Common::String shortFilename;

		shortFilename = makeSavefileName(getSavePath());

		VMUSaveFileWriteStream *sf;

		sf = new VMUSaveFileWriteStream(
		             this, dev, shortFilename, filename);
		return (Common::OutSaveFile *)
		    Common::wrapCompressedWriteStream(sf);
        } else {
                VMUSaveFileWriteStream *sf = (VMUSaveFileWriteStream *)
		    file->_value.createWriteStream(this, dev);
                return (Common::OutSaveFile *)
		    Common::wrapCompressedWriteStream(sf);
        }
}

bool DCAltSaveFileManager::removeSavefile(const Common::String &filename) {
	if (!(strncmp(getSavePath().c_str(), "/vmu", 4) == 0))
		return DefaultSaveFileManager::removeSavefile(filename);

	maple_device_t *dev;
	int ret;

	assureCached(getSavePath());
        if (getError().getCode() != Common::kNoError)
                return false;

	// Obtain node if exists.
        SaveFileCache::const_iterator file = _saveFileCache.find(filename);
        if (file == _saveFileCache.end()) {
                return false;
        } else {
		const Common::String shortFilename = file->_value.getShortFilename();
                // Remove from cache, this invalidates the 'file' iterator.
                _saveFileCache.erase(file);
                file = _saveFileCache.end();

		dev = vmu_path_to_addr(&getSavePath().c_str()[4]);

		ret = vmufs_delete(dev, shortFilename.c_str());
		_fingerprint = fingerprint(getSavePath());
		return (ret == 0);
	}
}

void DCAltSaveFileManager::updateSavefilesList(
    Common::StringArray &lockedFiles) {
	if (!(strncmp(getSavePath().c_str(), "/vmu", 4) == 0))
		DefaultSaveFileManager::updateSavefilesList(lockedFiles);
}

Common::StringArray DCAltSaveFileManager::listSavefiles(
    const Common::String &pattern) {
	if (!(strncmp(getSavePath().c_str(), "/vmu", 4) == 0))
		return DefaultSaveFileManager::listSavefiles(pattern);

	assureCached(getSavePath());
        if (getError().getCode() != Common::kNoError)
                return Common::StringArray();

	Common::StringArray results;
	for (SaveFileCache::const_iterator file = _saveFileCache.begin(),
	                                   end = _saveFileCache.end();
	     file != end;
	     ++file) {
		if (file->_key.matchString(pattern, true))
			results.push_back(file->_key);
	}
        return results;
}

void DCAltSaveFileManager::assureCached(const Common::String &savePathName) {
	clearError();
	Common::String currentFingerprint = fingerprint(savePathName);

	if (_fingerprint == currentFingerprint)
		return;

	DIR *dirp;
        struct dirent *dp;
	maple_device_t *dev;
	VMUSaveFile *saveFile;

	_saveFileCache.clear();

	dev = vmu_path_to_addr(&savePathName.c_str()[4]);
	if (!dev) {
		setError(Common::kPathDoesNotExist,
		         "Invalid VMU path " + savePathName);
		return;
	}

	dirp = opendir(savePathName.c_str());
	if (dirp == NULL) {
		setError(Common::kPathDoesNotExist,
		         "Could not read VMU at " + savePathName);
		return;
	}

	do {
		dp = readdir(dirp);
		if ((dp != NULL) && (strncmp("SCUMMVM_", dp->d_name, 8) == 0)) {
			saveFile = new VMUSaveFile(dev, dp->d_name);
			_saveFileCache[saveFile->getLongFilename()] = *saveFile;
		}
	} while (dp != NULL);
	closedir(dirp);

	_fingerprint = currentFingerprint;
}

void DCAltSaveFileManager::cacheNewSave(
    const Common::String &longFilename, const VMUSaveFile *saveFile) {
	_saveFileCache[longFilename] = *saveFile;
	_fingerprint = fingerprint(getSavePath());
}

Common::String DCAltSaveFileManager::fingerprint(
    const Common::String &savePathName) {
	Common::ReadStream *stream;
	Common::String md5;
	maple_device_t *dev;
	uint8 buf[sizeof(vmu_root_t) + 256 * sizeof(uint16)];
	vmu_root_t *root = (vmu_root_t *)buf;
	uint16 *fat = (uint16 *)(buf + sizeof(vmu_root_t));
	int ret;

	dev = vmu_path_to_addr(&savePathName.c_str()[4]);
	if (!dev) {
		setError(Common::kPathDoesNotExist,
		         "Invalid VMU path " + savePathName);
		return Common::String();
	}

	ret = vmufs_root_read(dev, root);
	if (ret < 0) {
		setError(Common::kPathDoesNotExist,
		         "Could not read VMU at " + savePathName);
		return Common::String();
	}
	assert(root->fat_size == 1);

	ret = vmufs_fat_read(dev, root, fat);
	if (ret < 0) {
		setError(Common::kPathDoesNotExist,
		         "Could not read VMU at " + savePathName);
		return Common::String();
	}

	stream = new Common::MemoryReadStream(buf, sizeof(buf));

	md5 = Common::computeStreamMD5AsString(*stream);

	delete stream;

	return md5;
}

VMUConfigFileWriteStream::VMUConfigFileWriteStream(
    maple_device_t *dev,
    const Common::String &filename,
    int existingSize) {
	int icon_size, ec_size, free_bytes;

	_filename = filename;
	_dev = dev;

	free_bytes = vmufs_free_blocks(_dev) * 512 + existingSize;

	memset(&_pkg, 0, sizeof(struct vmu_pkg));
	memset(_pkg.desc_short, ' ', 16);
	memcpy(_pkg.desc_short, "ScummVM", strlen("ScummVM"));
	memset(_pkg.desc_long, ' ', 32);
	memcpy(_pkg.desc_long,
	       _filename.c_str(),
	       MAX(_filename.size(), 32));
	memset(_pkg.app_id, '\0', 16);
	strcpy(_pkg.app_id, "ScummVM");
	_pkg.icon_cnt = 1;
	_pkg.icon_anim_speed = 0;
	_pkg.eyecatch_type = VMUPKG_EC_NONE;
	_pkg.data_len = 0;
	memcpy(&_pkg.icon_pal, &icon_palette, sizeof(uint16) * 16);
	_pkg.icon_data = (const uint8 *)&icon_pixels;
	_pkg.eyecatch_data = NULL;

	icon_size = 512 * _pkg.icon_cnt;
	ec_size = vmu_eyecatch_size(_pkg.eyecatch_type);
	_hdr_size = sizeof(struct vmu_hdr) + icon_size + ec_size;

	_data = (uint8 *)malloc(free_bytes - _hdr_size);

	_saveBuf = _data;

	_pkg.data = _data;
	_pkg.data_len = 0;

	_stream = new Common::MemoryWriteStream(
	    _saveBuf, free_bytes - _hdr_size);
}

VMUConfigFileWriteStream::~VMUConfigFileWriteStream() {
	uint8 *buf;
	int bufSize;
	int ret;

	if (_stream->err()) {
		free(_data);
		free(_stream);
		return;
	}

	_pkg.data_len = _stream->pos();

	ret = vmu_pkg_build(&_pkg, &buf, &bufSize);
	if (ret < 0) {
		free(_data);
		free(_stream);
		return;
	}

	vmufs_write(_dev, _filename.c_str(),
	            buf, bufSize, VMUFS_OVERWRITE);

	free(_data);
	free(_stream);

	free(buf);
}
