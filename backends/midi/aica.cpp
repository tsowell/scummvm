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

// Disable symbol overrides so that we can use system headers.
#define FORBIDDEN_SYMBOL_ALLOW_ALL

#include "common/scummsys.h"

#if defined(__DCALT__)

#include <dc/sound/sound.h>
#include <dc/sound/midi.h>

#include "common/error.h"
#include "common/textconsole.h"
#include "common/util.h"
#include "audio/musicplugin.h"
#include "audio/mpu401.h"

////////////////////////////////////////
//
// Dreamcast AICA MIDI driver
//
////////////////////////////////////////

class MidiDriver_AICA : public MidiDriver_MPU401 {
public:
	MidiDriver_AICA();
	int open();
	bool isOpen() const { return _isOpen; }
	void close();
	void send(uint32 b);
	void sysEx(const byte *msg, uint16 length);

private:
	bool _isOpen;
};

MidiDriver_AICA::MidiDriver_AICA() {
	_isOpen = false;
}

int MidiDriver_AICA::open() {
	if (_isOpen)
		return MERR_ALREADY_OPEN;
	snd_init();
	snd_sh4_to_aica_start();
	_isOpen = true;
	return 0;
}

void MidiDriver_AICA::close() {
	MidiDriver_MPU401::close();
	_isOpen = false;
}

void MidiDriver_AICA::send(uint32 b) {
	switch (b & 0xF0) {
	case 0x80:
	case 0x90:
	case 0xA0:
	case 0xB0:
	case 0xE0:
		snd_midi((uint8)b);
		snd_midi((uint8)((b >> 8) & 0x7F));
		snd_midi((uint8)((b >> 16) & 0x7F));
		break;
	case 0xC0:
	case 0xD0:
		snd_midi((uint8)b);
		snd_midi((uint8)((b >> 8) & 0x7F));
		break;
	default:
		warning("MidiDriver_AICA::send: unknown: %08x", (int)b);
		break;
	}
}

void MidiDriver_AICA::sysEx(const byte *msg, uint16 length) {
	const byte *chr = msg;

	assert(length + 2 <= 266);

	snd_midi(0xF0);
	for (; length; --length, ++chr) {
		snd_midi((uint8) *chr & 0x7F);
	}
	snd_midi(0xF7);
}


// Plugin interface

class AicaMusicPlugin : public MusicPluginObject {
public:
	const char *getName() const {
		return "Dreamcast AICA";
	}

	const char *getId() const {
		return "aica";
	}

	MusicDevices getDevices() const;
	Common::Error createInstance(MidiDriver **mididriver, MidiDriver::DeviceHandle = 0) const;
};

MusicDevices AicaMusicPlugin::getDevices() const {
	MusicDevices devices;
	// TODO: Return a different music type depending on the configuration
	// TODO: List the available devices
	devices.push_back(MusicDevice(this, "", MT_GM));
	return devices;
}

Common::Error AicaMusicPlugin::createInstance(MidiDriver **mididriver, MidiDriver::DeviceHandle) const {
	*mididriver = new MidiDriver_AICA();

	return Common::kNoError;
}

//#if PLUGIN_ENABLED_DYNAMIC(AICA)
	//REGISTER_PLUGIN_DYNAMIC(AICA, PLUGIN_TYPE_MUSIC, AicaMusicPlugin);
//#else
	REGISTER_PLUGIN_STATIC(AICA, PLUGIN_TYPE_MUSIC, AicaMusicPlugin);
//#endif

#endif
