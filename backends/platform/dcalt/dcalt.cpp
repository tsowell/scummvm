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

#define FORBIDDEN_SYMBOL_EXCEPTION_stdout
#define FORBIDDEN_SYMBOL_EXCEPTION_fputs
#define FORBIDDEN_SYMBOL_EXCEPTION_time

#include <malloc.h>

#include <arch/gdb.h>
#include <kos/mutex.h>
#include <arch/timer.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/keyboard.h>
#include <dc/maple/mouse.h>
#include <dc/sound/stream.h>
#include <kos/thread.h>

#include "backends/modular-backend.h"
#include "base/main.h"
#include "backends/plugins/dcalt/dcalt-provider.h"

#include "backends/timer/default/default-timer.h"
#include "backends/events/default/default-events.h"
#include "audio/mixer_intern.h"
#include "common/scummsys.h"
#include "common/mutex.h"

#include "backends/fs/posix/posix-fs-factory.h"

#include "backends/platform/dcalt/mutex.h"
#include "backends/platform/dcalt/graphics.h"
#include "backends/platform/dcalt/saves.h"

#include "common/config-manager.h"

#include <dc/sd.h>
#include <dc/g1ata.h>
#include <kos/blockdev.h>
#include <fat/fs_fat.h>
#include <dc/vmufs.h>
#include <dc/vmu_pkg.h>
#include <dirent.h>

#include "backends/platform/dcalt/keyboard.h"

#define CONT_RTRIG (1 << 16)
#define CONT_LTRIG (1 << 17)

class OSystem_DCAlt : public ModularBackend, Common::EventSource {
public:
	OSystem_DCAlt(bool sd_mounted, bool ata_mounted);
	virtual ~OSystem_DCAlt();

	virtual void initBackend();

	virtual Common::EventSource *getDefaultEventSource() { return this; }
	virtual bool pollEvent(Common::Event &event);

	virtual uint32 getMillis(bool skipRecord = false);
	virtual void delayMillis(uint msecs);
	virtual void getTimeAndDate(TimeDate &t) const;

	virtual void quit();

	virtual void logMessage(LogMessageType::Type type, const char *message);

	virtual bool hasFeature(Feature f);

	virtual Common::SeekableReadStream *createConfigReadStream();
	virtual Common::WriteStream *createConfigWriteStream();
private:
	static void *soundStreamCallback(
	    snd_stream_hnd_t hnd, int smp_req, int *smp_recv);
	static void *audioThreadFunction(void *arg);
	static void *timerThreadFunction(void *arg);
	static void handleButtons(
	    OSystem_DCAlt *os, uint32 changedButtons, uint32 buttons,
	    int dx, int dy);
	static void *eventThreadFunction(void *arg);
	bool _sd_mounted;
	bool _ata_mounted;
	maple_device_t *_controller;
	snd_stream_hnd_t _stream;
	uint8 *_stream_buf;
	bool _quitting;
	kthread_t *_audioThread;
	kthread_t *_timerThread;
	kthread_t *_eventThread;
	Common::Mutex *_eventMutex;
	Common::Queue<Common::Event> _eventQueue;
	Common::String _configLocation;
};

static Common::SeekableReadStream *_createReadStreamForFile(
    const Common::String &path) {
	Common::FSNode node(path);
	if (node.exists()) {
		return node.createReadStream();
	}
	else {
		return NULL;
	}
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

Common::SeekableReadStream *OSystem_DCAlt::createConfigReadStream() {
	Common::SeekableReadStream *stream;
	maple_device_t *dev;
	vmu_pkg_t pkg;
	byte *buf;
	int icon_size, ec_size, hdr_size;
	int size;
	int ret;

	stream = _createReadStreamForFile("/sd/scummvm/scummvm.ini");
	if (stream) {
		_configLocation = "/sd/scummvm/scummvm.ini";
		return stream;
	}
	stream = _createReadStreamForFile("/ata/scummvm/scummvm.ini");
	if (stream) {
		_configLocation = "/ata/scummvm/scummvm.ini";
		return stream;
	}

	dev = maple_enum_type(0, MAPLE_FUNC_MEMCARD);
	if (dev && (dev->info.functions & MAPLE_FUNC_MEMCARD)) {
		ret = vmufs_read(dev, "scummvm.ini", (void **)&buf, &size);
		if (ret == 0) {
			vmu_pkg_parse((uint8 *)buf, &pkg);
			_configLocation = 
				Common::String("/vmu/") +
				Common::String(dev->port + 'a') +
				Common::String(dev->unit + '0') +
				Common::String("/scummvm.ini");
			icon_size = 512 * pkg.icon_cnt;
			ec_size = vmu_eyecatch_size(pkg.eyecatch_type);
			hdr_size = sizeof(struct vmu_hdr) + icon_size + ec_size;
			// Include the VMU package so that MemoryReadStream can
			// free it when it finishes.
			stream = new Common::MemoryReadStream(
			    buf, hdr_size + pkg.data_len,
			    DisposeAfterUse::YES);
			// But seek past the VMU package to the actual file
			// contents.
			stream->seek(pkg.data - buf);
			return stream;
		}
	}

	stream = _createReadStreamForFile("/cd/scummvm/scummvm.ini");
	if (stream)
		return stream;

	return NULL;
}

Common::WriteStream *OSystem_DCAlt::createConfigWriteStream() {
	Common::WriteStream *stream;
	maple_device_t *dev;
	int port, unit;
	void *buf;
	int size;
	int ret;

	if (_configLocation.hasPrefix("/vmu/")) {
		port = _configLocation[5] - 'a';
		unit = _configLocation[6] - '0';
		dev = maple_enum_dev(port, unit);
		if (dev && (dev->info.functions & MAPLE_FUNC_MEMCARD)) {
			// get the existing size so that
			// VMUConfigFileWriteStream can compute remaining space
			ret = vmufs_read(dev, "scummvm.ini",
					 &buf, &size);
			if (ret < 0) {
				free(buf);
			}
			return new VMUConfigFileWriteStream(
			    dev, "scummvm.ini", size);
		}
	}
	else if (!_configLocation.empty()) {
		Common::FSNode node(_configLocation);
		return node.createWriteStream();
	}

	Common::FSNode("/sd/scummvm").createDirectory();
	stream = Common::FSNode("/sd/scummvm/scummvm.ini").createWriteStream();
	if (stream) {
		_configLocation = "/sd/scummvm/scummvm.ini";
		return stream;
	}

	Common::FSNode("/ata/scummvm").createDirectory();
	stream = Common::FSNode("/ata/scummvm/scummvm.ini").createWriteStream();
	if (stream) {
		_configLocation = "/ata/scummvm/scummvm.ini";
		return stream;
	}

	for (port = 0; port < 4; port++) {
		for (unit = 0; unit < 6; unit++) {
			dev = maple_enum_dev(port, unit);
			if (dev && (dev->info.functions & MAPLE_FUNC_MEMCARD)) {
				// get the existing size so that
				// VMUConfigFileWriteStream can compute
				// remaining space
				ret = vmufs_read(dev, "scummvm.ini",
						 &buf, &size);
				if (ret < 0) {
					free(buf);
				}
				return new VMUConfigFileWriteStream(
				    dev, "scummvm.ini", size);
			}
		}
	}

	return NULL;
}

OSystem_DCAlt::OSystem_DCAlt(bool sd_mounted, bool ata_mounted) :
    _controller(NULL),
    _quitting(false),
    _audioThread(NULL),
    _timerThread(NULL),
    _eventThread(NULL),
    _eventMutex(NULL),
    _sd_mounted(sd_mounted),
    _ata_mounted(ata_mounted) {
	_fsFactory = new POSIXFilesystemFactory();
}

OSystem_DCAlt::~OSystem_DCAlt() {
	free(_stream_buf);
	if (_eventMutex) {
		delete _eventMutex;
		_eventMutex = 0;
	}

	if (_timerManager) {
		delete _timerManager;
		_timerManager = 0;
	}

	if (_eventManager) {
		delete _eventManager;
		_eventManager = 0;
	}

	if (_savefileManager) {
		delete _savefileManager;
		_savefileManager = 0;
	}
}

void *OSystem_DCAlt::audioThreadFunction(void *arg) {
	OSystem_DCAlt *os = (OSystem_DCAlt *)g_system;
	while (!os->_quitting) {
		snd_stream_poll(os->_stream);
		thd_sleep(5);
	}

	return NULL;
}

void *OSystem_DCAlt::timerThreadFunction(void *arg) {
	OSystem_DCAlt *os = (OSystem_DCAlt *)g_system;
	while (!os->_quitting) {
		DefaultTimerManager *tm =
			(DefaultTimerManager *)g_system->getTimerManager();
		tm->handler();
		thd_sleep(10);
	}

	return NULL;
}

static void keyboardEvent(
    int region, int shift_keys, int scancode, Common::Event &event) {
	event.kbd.keycode =
	    ScancodeToOSystemKeycode(region, shift_keys, scancode);
	event.kbd.ascii =
	    OSystemKeycodeToAscii(event.kbd.keycode, shift_keys);

	if (event.type == Common::EVENT_KEYUP) {
		if (event.kbd.keycode == Common::KEYCODE_NUMLOCK) {
			event.kbd.flags ^= Common::KBD_NUM;
		}
		else if (event.kbd.keycode == Common::KEYCODE_CAPSLOCK) {
			event.kbd.flags ^= Common::KBD_CAPS;
		}
		else if (event.kbd.keycode == Common::KEYCODE_CAPSLOCK) {
			event.kbd.flags ^= Common::KBD_SCRL;
		}
	}
}

void OSystem_DCAlt::handleButtons(
    OSystem_DCAlt *os, uint32 changedButtons, uint32 buttons, int dx, int dy) {
	using namespace Common;
	Common::Event event;
	bool buttonUp;
	int i;
	struct ControllerEventMapping {
		int button;
		EventType normalType;
		KeyState normalKeyState;
		EventType modifierType;
		KeyState modifierKeyState;
	};

	static const ControllerEventMapping mapping[] = {
		// CONT_A: Left mouse button (+R_trigger: Virtual keyboard
		{ CONT_A,
		  EVENT_LBUTTONDOWN, KeyState(),
#ifdef ENABLE_VKEYBD
		  EVENT_VIRTUAL_KEYBOARD, KeyState() },
#else
		  EVENT_LBUTTONDOWN, KeyState() },
#endif
		// CONT_B: Right mouse button
		//         (+R_trigger: Predictive Input Dialog)
		{ CONT_B,
		  EVENT_RBUTTONDOWN, KeyState(),
		  EVENT_PREDICTIVE_DIALOG, KeyState() },
		// CONT_X: Period (+R_trigger: Space)
		{ CONT_X,
		  EVENT_KEYDOWN, KeyState(KEYCODE_PERIOD, '.'),
		  EVENT_KEYDOWN, KeyState(KEYCODE_SPACE, ASCII_SPACE) },
		// CONT_Y: Escape (+R_trigger: Return)
		{ CONT_Y,
		  EVENT_KEYDOWN, KeyState(KEYCODE_ESCAPE, ASCII_ESCAPE),
		  EVENT_KEYDOWN, KeyState(KEYCODE_RETURN, ASCII_RETURN) },
		// CONT_START: ScummVM in game menu
		{ CONT_START,
		  EVENT_MAINMENU, KeyState(),
		  EVENT_MAINMENU, KeyState() },
		// CONT_LTRIG: Game menu
		{ CONT_LTRIG,
		  EVENT_KEYDOWN, KeyState(KEYCODE_F5, ASCII_F5),
		  EVENT_KEYDOWN, KeyState(KEYCODE_F5, ASCII_F5) },
		// CONT_RTRIG: Modifier + Shift
		{ CONT_RTRIG,
		  EVENT_KEYDOWN, KeyState(KEYCODE_INVALID, 0, KBD_SHIFT),
		  EVENT_KEYDOWN, KeyState(KEYCODE_INVALID, 0, 0) },
		// CONT_DPAD_UP: Up (+R_trigger: Up+Right)
		{ CONT_DPAD_UP,
		  EVENT_KEYDOWN, KeyState(KEYCODE_KP8, 0),
		  EVENT_KEYDOWN, KeyState(KEYCODE_KP9, 0) },
		// CONT_DPAD_DOWN: Down (+R_trigger: Down+Left)
		{ CONT_DPAD_DOWN,
		  EVENT_KEYDOWN, KeyState(KEYCODE_KP2, 0),
		  EVENT_KEYDOWN, KeyState(KEYCODE_KP1, 0) },
		// CONT_DPAD_UP: Left (+R_trigger: Up+Left)
		{ CONT_DPAD_LEFT,
		  EVENT_KEYDOWN, KeyState(KEYCODE_KP4, 0),
		  EVENT_KEYDOWN, KeyState(KEYCODE_KP7, 0) },
		// CONT_DPAD_UP: Right (+R_trigger: Down+Right)
		{ CONT_DPAD_RIGHT,
		  EVENT_KEYDOWN, KeyState(KEYCODE_KP6, 0),
		  EVENT_KEYDOWN, KeyState(KEYCODE_KP3, 0) },
	};

	for (i = 0; i < (int)(sizeof(mapping) / sizeof(mapping[0])); i++) {
		if (changedButtons & mapping[i].button) {
			buttonUp = !(buttons & mapping[i].button);
			if (buttons & CONT_RTRIG) {
				event.type = mapping[i].modifierType;
				event.kbd = mapping[i].modifierKeyState;
			}
			else {
				event.type = mapping[i].normalType;
				event.kbd = mapping[i].normalKeyState;
			}
			if (buttonUp) {
				if (event.type == EVENT_KEYDOWN) {
					event.type = EVENT_KEYUP;
				}
				else if (event.type == EVENT_LBUTTONDOWN) {
					event.type = EVENT_LBUTTONUP;
				}
				else if (event.type == EVENT_RBUTTONDOWN) {
					event.type = EVENT_RBUTTONUP;
				}
				else {
					event.type = EVENT_INVALID;
				}
			}
			os->_eventMutex->lock();
			os->_eventQueue.push(event);
			os->_eventMutex->unlock();
		}
	}
}

void *OSystem_DCAlt::eventThreadFunction(void *arg) {
	static uint32 lastButtons[MAPLE_PORT_COUNT] = { 0 };
	static uint16 lastMouseButtons[MAPLE_PORT_COUNT] = { 0 };
	static uint8 lastMatrix[MAPLE_PORT_COUNT][MAX_KBD_KEYS] = { { 0 } };
	static int lastFlags[MAPLE_PORT_COUNT] = { 0 };
	OSystem_DCAlt *os = (OSystem_DCAlt *)g_system;
	int dx = 0, dy = 0;
	Common::Event event;
	uint32 changedButtons;
	int width;
	int i;

	event.mouse.x = 0;
	event.mouse.y = 0;

	while (!os->_quitting) {
		thd_sleep(10);

		width = os->_graphicsManager->getOverlayWidth();

		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)
			int port = __dev->port;

			uint32 buttons = st->buttons;
			// Simulate digital trigger buttons
			if (st->rtrig)
				buttons |= CONT_RTRIG;
			else
				buttons &= ~CONT_RTRIG;
			if (st->ltrig)
				buttons |= CONT_LTRIG;
			else
				buttons &= ~CONT_LTRIG;

			if (width == 640) {
				dx = st->joyx >> 3;
				dy = st->joyy >> 3;
			}
			else {
				dx = st->joyx >> 4;
				dy = st->joyy >> 4;
			}

			if ((dx != 0) || (dy != 0)) {
				event.type = Common::EVENT_MOUSEMOVE;
				// These will be translated to absolute virtual
				// coords by pollEvent
				event.mouse.x = dx;
				event.mouse.y = dy;
				os->_eventMutex->lock();
				os->_eventQueue.push(event);
				os->_eventMutex->unlock();
			}

			changedButtons = lastButtons[port] ^ buttons;
			handleButtons(os, changedButtons, buttons, dx, dy);
			lastButtons[port] = buttons;
		MAPLE_FOREACH_END()

		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_MOUSE, mouse_state_t, st)
			int port = __dev->port;

			if (width == 640) {
				dx = st->dx >> 0;
				dy = st->dy >> 0;
			}
			else {
				dx = st->dx >> 1;
				dy = st->dy >> 1;
			}

			if ((dx != 0) || (dy != 0)) {
				event.type = Common::EVENT_MOUSEMOVE;
				// These will be translated to absolute virtual
				// coords by pollEvent
				event.mouse.x = dx;
				event.mouse.y = dy;
				os->_eventMutex->lock();
				os->_eventQueue.push(event);
				os->_eventMutex->unlock();
			}

			if (st->dz < 0) {
				event.type = Common::EVENT_WHEELUP;
				os->_eventMutex->lock();
				os->_eventQueue.push(event);
				os->_eventMutex->unlock();
			}
			else if (st->dz > 0) {
				event.type = Common::EVENT_WHEELDOWN;
				os->_eventMutex->lock();
				os->_eventQueue.push(event);
				os->_eventMutex->unlock();
			}

			changedButtons = lastMouseButtons[port] ^ st->buttons;
			if (changedButtons & MOUSE_LEFTBUTTON) {
				if (st->buttons & MOUSE_LEFTBUTTON)
					event.type = Common::EVENT_LBUTTONDOWN;
				else
					event.type = Common::EVENT_LBUTTONUP;
				os->_eventMutex->lock();
				os->_eventQueue.push(event);
				os->_eventMutex->unlock();
			}
			if (changedButtons & MOUSE_SIDEBUTTON) {
				if (st->buttons & MOUSE_SIDEBUTTON)
					event.type = Common::EVENT_MAINMENU;
				else
					event.type = Common::EVENT_MAINMENU;
				os->_eventMutex->lock();
				os->_eventQueue.push(event);
				os->_eventMutex->unlock();
			}
			if (changedButtons & MOUSE_RIGHTBUTTON) {
				if (st->buttons & MOUSE_RIGHTBUTTON)
					event.type = Common::EVENT_RBUTTONDOWN;
				else
					event.type = Common::EVENT_RBUTTONUP;
				os->_eventMutex->lock();
				os->_eventQueue.push(event);
				os->_eventMutex->unlock();
			}

			lastMouseButtons[port] = st->buttons;
		MAPLE_FOREACH_END()

		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_KEYBOARD, kbd_state_t, st)
			int port = __dev->port;

			event.kbd.flags = lastFlags[port];

			if (st->shift_keys &
			    (KBD_MOD_LCTRL | KBD_MOD_RCTRL))
				event.kbd.flags |= Common::KBD_CTRL;
			if (st->shift_keys &
			    (KBD_MOD_LALT | KBD_MOD_RALT))
				event.kbd.flags |= Common::KBD_ALT;
			if (st->shift_keys &
			    (KBD_MOD_LSHIFT | KBD_MOD_RSHIFT))
				event.kbd.flags |= Common::KBD_SHIFT;

			for (i = 0; i < MAX_KBD_KEYS; i++) {
				if (lastMatrix[port][i] == st->matrix[i])
					continue;
				else if (st->matrix[i])
					event.type = Common::EVENT_KEYDOWN;
				else
					event.type = Common::EVENT_KEYUP;

				keyboardEvent(st->region, st->shift_keys, i,
				              event);

				os->_eventMutex->lock();
				os->_eventQueue.push(event);
				os->_eventMutex->unlock();

				lastFlags[port] = event.kbd.flags;
			}

			memcpy(&lastMatrix[port], st->matrix,
			       sizeof(uint8) * MAX_KBD_KEYS);
		MAPLE_FOREACH_END()
	}
	return NULL;
}

void *OSystem_DCAlt::soundStreamCallback(
    snd_stream_hnd_t hnd, int smp_req, int *smp_recv) {
	OSystem_DCAlt *os = (OSystem_DCAlt *)g_system;
	Audio::MixerImpl *mixer = (Audio::MixerImpl *)os->_mixer;
	mixer->mixCallback(os->_stream_buf, smp_req);
	*smp_recv = smp_req;
	return (void *)(os->_stream_buf);
}

void OSystem_DCAlt::initBackend() {
	ConfMan.registerDefault("dcalt_vga_mode_aspect_ratio", false);
	ConfMan.registerDefault("dcalt_vga_25175", false);
	ConfMan.registerDefault("dcalt_vga_polarity", false);
	if (!ConfMan.hasKey("savepath", 0))
		ConfMan.set("savepath", "/vmu/a1", 0);
	if (!ConfMan.hasKey("vkeybdpath", 0))
		ConfMan.set("vkeybdpath", "/cd/vkeybd");
	if (!ConfMan.hasKey("themepath", 0))
		ConfMan.set("themepath", "/cd/themes");
	if (!ConfMan.hasKey("extrapath", 0))
		ConfMan.set("extrapath", "/cd/extra");
	if (!ConfMan.hasKey("pluginspath", 0))
		ConfMan.set("pluginspath", "/cd/plugins");

	_mutexManager = new DCAltMutexManager();
	_timerManager = new DefaultTimerManager();
	_eventManager = new DefaultEventManager(this);
	_savefileManager = new DCAltSaveFileManager();
	_graphicsManager = new DCAltGraphicsManager();

	_timerThread = thd_create(0, timerThreadFunction, NULL);

	_mixer = new Audio::MixerImpl(32000);
	((Audio::MixerImpl *)_mixer)->setReady(false);

	// Not entirely sure what the ratio between snd_stream_alloc and
	// memalign's sizes should be...
	_stream_buf = (uint8 *)memalign(32, SND_STREAM_BUFFER_MAX);
	snd_stream_init();
	_stream = snd_stream_alloc(soundStreamCallback, SND_STREAM_BUFFER_MAX / 4);
	((Audio::MixerImpl *)_mixer)->setReady(true);
	snd_stream_start(_stream, 32000, 1);
	_audioThread = thd_create(0, audioThreadFunction, NULL);

	_eventMutex = new Common::Mutex();
	_eventThread = thd_create(0, eventThreadFunction, NULL);

	ModularBackend::initBackend();
}

uint32 OSystem_DCAlt::getMillis(bool skipRecord) {
	return (uint32)timer_ms_gettime64();
}

void OSystem_DCAlt::delayMillis(uint msecs) {
	thd_sleep(msecs);
}

void OSystem_DCAlt::getTimeAndDate(TimeDate &t) const {
	// KallistiOS defines 32-bit time_t which can't be used with newlib's
	// localtime_r which uses 64-bit time_t by default.  Building newlib
	// with --enable-newlib-long-time_t causes even bigger problems.
	// Fortunately ScummVM doesn't use time_t anywhere else, so we can work
	// around it here.
	int64_t curTime = time(0);
        struct tm tm;
        localtime_r((const time_t *)&curTime, &tm);
        t.tm_sec = tm.tm_sec;
        t.tm_min = tm.tm_min;
        t.tm_hour = tm.tm_hour;
        t.tm_mday = tm.tm_mday;
        t.tm_mon = tm.tm_mon;
        t.tm_year = tm.tm_year;
        t.tm_wday = tm.tm_wday;
}

void OSystem_DCAlt::quit() {
	_quitting = true;
	if (_audioThread) {
		thd_join(_audioThread, NULL);
	}
	if (_timerThread) {
		thd_join(_timerThread, NULL);
	}
	if (_eventThread) {
		thd_join(_eventThread, NULL);
	}
}

void OSystem_DCAlt::logMessage(
    LogMessageType::Type type, const char *message) {
	fputs(message, stdout);
	fflush(stdout);
}

bool OSystem_DCAlt::hasFeature(Feature f) {
	return _graphicsManager->hasFeature(f);
}

bool OSystem_DCAlt::pollEvent(Common::Event &event) {
	Common::StackLock lock(*_eventMutex);
	DCAltGraphicsManager *graphicsManager =
	    (DCAltGraphicsManager *)_graphicsManager;

	// This doesn't belong here but has problems when done in a different
	// thread.
	if (_sd_mounted) {
		fs_fat_sync("/sd");
	}
	if (_ata_mounted) {
		fs_fat_sync("/ata");
	}

	if (_eventQueue.empty())
		return false;

	event = _eventQueue.pop();

	if (event.type == Common::EVENT_MOUSEMOVE) {
		// Convert relative mouse data into absolute virtual screen
		// coordinates
		graphicsManager-> translateMouse(
		    event, event.mouse.x, event.mouse.y);
	}
	else {
		event.mouse.x = graphicsManager->getMouseX();
		event.mouse.y = graphicsManager->getMouseY();
	}

	return true;
}

int main(int argc, char *argv[]) {
	int ret;
	bool sd_mounted = false;
	bool ata_mounted = false;
	//gdb_init();

	kos_blockdev_t sd_dev, g1_ata_dev;
	uint8 sd_partition_type, g1_ata_partition_type;

	fs_fat_init();

	if (!sd_init() &&
	    !sd_blockdev_for_partition(0, &sd_dev, &sd_partition_type)) {
		ret = fs_fat_mount("/sd", &sd_dev, FS_FAT_MOUNT_READWRITE);
		sd_mounted = ret == 0;
	}

	if (!g1_ata_init() &&
	    !g1_ata_blockdev_for_partition(
	         0, 0, &g1_ata_dev, &g1_ata_partition_type)) {
		ret = fs_fat_mount("/ata", &g1_ata_dev,
		                    FS_FAT_MOUNT_READWRITE);
		ata_mounted = ret == 0;
	}

	g_system = new OSystem_DCAlt(sd_mounted, ata_mounted);
	assert(g_system);


#ifdef DYNAMIC_MODULES
	PluginManager::instance().addPluginProvider(new DCAltPluginProvider());
#endif
	// Invoke the actual ScummVM main entry point:
	int res = scummvm_main(argc, argv);

	g_system->quit();
	g_system->destroy();

	if (sd_mounted) {
		fs_fat_sync("/sd");
		fs_fat_unmount("/sd");
	}
	if (ata_mounted) {
		fs_fat_sync("/ata");
		fs_fat_unmount("/ata");
	}

	fs_fat_shutdown();

	mem_check_all();

	return res;
}
