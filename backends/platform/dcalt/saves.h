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

#ifndef DCALT_SAVES_H
#define DCALT_SAVES_H

#include <dc/maple.h>
#include <dc/vmu_pkg.h>

#include "common/scummsys.h"
#include "common/savefile.h"
#include "common/str.h"
#include "common/str-array.h"
#include "common/fs.h"
#include "common/hash-str.h"
#include "common/random.h"
#include "common/stream.h"
#include "common/memstream.h"
#include <limits.h>

#include "backends/saves/default/default-saves.h"

/**
 * Provides a savefile manager implementation that can save to VMU or other
 * storage.
 *
 * VMU saves require special treatment.  A) VMU files should be wrapped in a
 * header in order to be visible to the Dreamcast BIOS.  B) The VMU filesystem
 * has a maximum filename length of 12 characters which is frequently exceeded
 * by ScummVM engines.  C) Reading from the VMU is slow: it can take a couple
 * of seconds to read a ~20 block save, which can add up to a 30 second wait
 * just to show the save/load dialog.  D) VMUs are small and can only fit a few
 * ScummVM savefiles.
 *
 * An attempt has been made to address these shortcomings, but it's still
 * preferable to use an SD card for saving.
 *
 * For VMUs, this save manager uses a random SCUMMVM_XXXX short filename for
 * saves and stores, in order, the 32-bit offset of the save data within the
 * file, followed by a null-terminated long filename, and then the actual save
 * data.  It caches the long filename to short filename mapping and all ScummVM
 * save data, using a checksum of the root dir and fat to uniquely identify a
 * VMU.  The first time the save/load dialog displays can take a while, but
 * subsequent uses of the same VMU should be very quick.
 *
 * Save paths outside of /vmu are passed to the default savefile manager.
 */

class DCAltSaveFileManager;

class VMUSaveFileWriteStream : public Common::SeekableWriteStream {
public:
	VMUSaveFileWriteStream(
	    DCAltSaveFileManager *savefileManager,
	    maple_device_t *dev,
	    const Common::String &shortFilename,
	    const Common::String &longFilename,
	    int existingSize = 0);
	~VMUSaveFileWriteStream();

	uint32 write(const void *dataPtr, uint32 dataSize) override {
		return _stream->write(dataPtr, dataSize);
	}
	virtual int32 pos() const override { return _stream->pos(); }
	virtual int32 size() const override { return _stream->size(); }
	virtual bool err() const override { return _stream->err(); }
	virtual void clearErr() override { _stream->clearErr(); }
	virtual bool seek(int32 offset, int whence = SEEK_SET) override {
		return _stream->seek(offset, whence);
	}

private:
	DCAltSaveFileManager *_savefileManager;
	Common::MemoryWriteStream *_stream;
	Common::String _shortFilename;
	Common::String _longFilename;
	maple_device_t *_dev;
	vmu_pkg_t _pkg;
	uint8 *_data;
	int _hdr_size;
	uint8 *_saveBuf;
};

class VMUSaveFile {
public:
	VMUSaveFile();
	VMUSaveFile(maple_device_t *dev, const Common::String &shortFilename);
	VMUSaveFile(uint8 *buf, int size, const Common::String &shortFilename);
	~VMUSaveFile();

	Common::String getShortFilename() const;
	Common::String getLongFilename() const;

	Common::SeekableReadStream *createReadStream() const;
	Common::WriteStream *createWriteStream(
	    DCAltSaveFileManager *savefileManager, maple_device_t *dev) const;
private:
	void init(uint8 *buf, int size, const Common::String &shortFilename);

	uint8 *_buf;
	int _size;
	vmu_pkg_t _pkg;

	const uint8 *_saveBuf;
	int _saveSize;

	Common::String _shortFilename;
};

class DCAltSaveFileManager : public DefaultSaveFileManager {
public:
	DCAltSaveFileManager();

	virtual Common::StringArray listSavefiles(
	    const Common::String &pattern) override;
	virtual Common::InSaveFile *openRawFile(
	    const Common::String &filename) override;
	virtual Common::InSaveFile *openForLoading(
	    const Common::String &filename) override;
	virtual Common::OutSaveFile *openForSaving(
	    const Common::String &filename, bool compress = true) override;
	virtual bool removeSavefile(const Common::String &filename) override;
	virtual void updateSavefilesList(
	    Common::StringArray &lockedFiles) override;

	virtual void cacheNewSave(
	    const Common::String &longFilename, const VMUSaveFile *);
private:
	void assureCached(const Common::String &savePathName);
	typedef Common::HashMap<Common::String, VMUSaveFile, Common::IgnoreCase_Hash, Common::IgnoreCase_EqualTo> SaveFileCache;

	SaveFileCache _saveFileCache;

	Common::String fingerprint(const Common::String &savePathName);
	Common::String _fingerprint;

	Common::String makeSavefileName(const Common::String &savePath);
	bool find(const Common::String &filename, Common::FSNode &fsNode);

	Common::RandomSource _rnd;
};

class VMUConfigFileWriteStream : public Common::SeekableWriteStream {
public:
	VMUConfigFileWriteStream(
	    maple_device_t *dev,
	    const Common::String &filename,
	    int existingSize = 0);
	~VMUConfigFileWriteStream();

	uint32 write(const void *dataPtr, uint32 dataSize) override {
		return _stream->write(dataPtr, dataSize);
	}
	virtual int32 pos() const override { return _stream->pos(); }
	virtual int32 size() const override { return _stream->size(); }
	virtual bool err() const override { return _stream->err(); }
	virtual void clearErr() override { _stream->clearErr(); }
	virtual bool seek(int32 offset, int whence = SEEK_SET) override {
		return _stream->seek(offset, whence);
	}

private:
	Common::MemoryWriteStream *_stream;
	Common::String _filename;
	maple_device_t *_dev;
	vmu_pkg_t _pkg;
	uint8 *_data;
	int _hdr_size;
	uint8 *_saveBuf;
};

#endif
