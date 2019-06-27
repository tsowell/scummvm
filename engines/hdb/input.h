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

#ifndef HDB_INPUT_H
#define HDB_INPUT_H

#include "common/system.h"

namespace HDB {

enum Button {
	kButtonUp		= 2 << 0,
	kButtonDown		= 2 << 1,
	kButtonLeft		= 2 << 2,
	kButtonRight	= 2 << 3,
	kButtonA		= 2 << 4,
	kButtonB		= 2 << 5,
	kButtonC		= 2 << 6,
	kButtonD		= 2 << 7,
	kButtonMouseL	= 2 << 8,
	kButtonMouseM	= 2 << 9,
	kButtonMouseR	= 2 << 10,
	kButtonExit		= 2 << 11
};

class Input {
public:

	bool init();

	void setButtons(uint16 b);
	uint16 getButtons();
	void stylusDown(int x, int y);
	void stylusUp(int x, int y);
	void stylusMove(int x, int y);

private:

	uint16 _buttons;	// Flags for buttons
	bool _stylusDown;
	int _stylusDownX, _stylusDownY;
	int _mouseX, _mouseY;
	int _lastMouseX, _lastMouseY;

	// Definable Keys
	int _keyUp, _keyDown, _keyLeft, _keyRight;
	int _keyInv, _keyUse, _keyMenu, _keyDebug;
	int _keyQuit;
};

} // End of Namespace

#endif // !HDB_INPUT_H
