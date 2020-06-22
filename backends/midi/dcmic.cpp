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

#include <dc/scif.h>

#include "common/error.h"
#include "common/textconsole.h"
#include "common/util.h"
#include "audio/musicplugin.h"
#include "audio/mpu401.h"

////////////////////////////////////////
//
// Dreamcast MIDI Interface Cable MIDI driver
//
////////////////////////////////////////


class MidiDriver_DCMIC : public MidiDriver_MPU401 {
public:
	MidiDriver_DCMIC();
	int open();
	bool isOpen() const { return _isOpen; }
	void close();
	void send(uint32 b);
	void sysEx(const byte *msg, uint16 length);

private:
	bool _isOpen;
};

// We can't use scif_write because KallistiOS may have been using the SCIF for
// dbgio output in which case the SCIF API may have already disabled itself
// after timing out.  There should really be a MIDI Interface Cable API for
// this.
#define SCFSR2 (*((volatile uint16 *) 0xffe80010)) 
#define SCFTDR2 (*((volatile uint8 *) 0xffe8000c))
#define SCFCR2 (*((volatile uint16 *) 0xffe80018))
static int dcmic_write(int c) {
	int timeout = 800000;													   
																				
	/* Wait until the transmit buffer has space. Too long of a failure		  
	   is indicative of no serial cable. */
	while(!(SCFSR2 & 0x20) && timeout > 0)
		timeout--;															  
																				
	if(timeout <= 0) {
		return -1;
	}																		   
																				
	/* Send the char */														 
	SCFTDR2 = c;																
																				
	/* Clear status */														  
	SCFSR2 &= 0xff9f;														   
																				
	return 1;
}

MidiDriver_DCMIC::MidiDriver_DCMIC() {
	_isOpen = false;
}

int MidiDriver_DCMIC::open() {
	if (_isOpen)
		return MERR_ALREADY_OPEN;
	// Disable dbgio if it is using the SCIF port.
	if (strcmp(dbgio_dev_get(), "scif") == 0) {                                 
		dbgio_disable();                                                        
	} 
	scif_set_parameters(31250, 0);
	scif_init();
	// XXX: Disable hardware flow control in SCFCR2.  This should really be
	// done by KallistiOS.
	SCFCR2 = 0x0000;
	_isOpen = true;
	return 0;
}

void MidiDriver_DCMIC::close() {
	MidiDriver_MPU401::close();
	_isOpen = false;
}

void MidiDriver_DCMIC::send(uint32 b) {
	switch (b & 0xF0) {
	case 0x80:
	case 0x90:
	case 0xA0:
	case 0xB0:
	case 0xE0:
		dcmic_write((uint8)b);
		dcmic_write((uint8)((b >> 8) & 0x7F));
		dcmic_write((uint8)((b >> 16) & 0x7F));
		break;
	case 0xC0:
	case 0xD0:
		dcmic_write((uint8)b);
		dcmic_write((uint8)((b >> 8) & 0x7F));
		break;
	default:
		warning("MidiDriver_DCMIC::send: unknown: %08x", (int)b);
		break;
	}
}

void MidiDriver_DCMIC::sysEx(const byte *msg, uint16 length) {
	const byte *chr = msg;

	assert(length + 2 <= 266);

	dcmic_write(0xF0);
	for (; length; --length, ++chr) {
		dcmic_write((uint8) *chr & 0x7F);
	}
	dcmic_write(0xF7);
}


// Plugin interface

class DcmicMusicPlugin : public MusicPluginObject {
public:
	const char *getName() const {
		return "Dreamcast MIDI Interface Cable";
	}

	const char *getId() const {
		return "dcmic";
	}

	MusicDevices getDevices() const;
	Common::Error createInstance(MidiDriver **mididriver, MidiDriver::DeviceHandle = 0) const;
};

MusicDevices DcmicMusicPlugin::getDevices() const {
	MusicDevices devices;
	// TODO: Return a different music type depending on the configuration
	// TODO: List the available devices
	devices.push_back(MusicDevice(this, "", MT_GM));
	return devices;
}

Common::Error DcmicMusicPlugin::createInstance(MidiDriver **mididriver, MidiDriver::DeviceHandle) const {
	*mididriver = new MidiDriver_DCMIC();

	return Common::kNoError;
}

//#if PLUGIN_ENABLED_DYNAMIC(DCMIC)
	//REGISTER_PLUGIN_DYNAMIC(DCMIC, PLUGIN_TYPE_MUSIC, DcmicMusicPlugin);
//#else
	REGISTER_PLUGIN_STATIC(DCMIC, PLUGIN_TYPE_MUSIC, DcmicMusicPlugin);
//#endif

#endif
