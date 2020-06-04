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

#include "backends/platform/dcalt/keyboard.h"

#include <dc/maple/keyboard.h>

struct keymap {
	Common::KeyCode base[MAX_KBD_KEYS];
	Common::KeyCode shifted[MAX_KBD_KEYS];
	Common::KeyCode alt[MAX_KBD_KEYS];
};

#define EIGHT_INVALID \
  Common::KEYCODE_INVALID, \
  Common::KEYCODE_INVALID, \
  Common::KEYCODE_INVALID, \
  Common::KEYCODE_INVALID, \
  Common::KEYCODE_INVALID, \
  Common::KEYCODE_INVALID, \
  Common::KEYCODE_INVALID, \
  Common::KEYCODE_INVALID

#define K(x) Common::KEYCODE_##x
#define K8(a, b, c, d, e, f, g, h) \
  Common::KEYCODE_##a, \
  Common::KEYCODE_##b, \
  Common::KEYCODE_##c, \
  Common::KEYCODE_##d, \
  Common::KEYCODE_##e, \
  Common::KEYCODE_##f, \
  Common::KEYCODE_##g, \
  Common::KEYCODE_##h

// Keyboard maps based on
// KallistiOS/kernel/arch/dreamcast/hardware/maple/keyboard.c and
// linux/drivers/input/keyboard/maple_keyb.c
struct keymap keymaps[8] = {
/* Japanese keyboard */
{
	/* Base values */
	{
	K8(INVALID, INVALID, INVALID, INVALID, a, b, c, d), /* 0x00 - 0x07 */
	K8(e, f, g, h, i, j, k, l), /* 0x08 - 0x0F */
	K8(m, n, o, p, q, r, s, t), /* 0x10 - 0x17 */
	K8(u, v, w, x, y, z, 1, 2), /* 0x18 - 0x1F */
	K8(3, 4, 5, 6, 7, 8, 9, 0), /* 0x20 - 0x27 */
	K8(RETURN, ESCAPE, BACKSPACE, TAB,
	   SPACE, MINUS, CARET, AT), /* 0x28 - 0x2F */
	K8(LEFTBRACKET, INVALID, RIGHTBRACKET, SEMICOLON,
	   COLON, INVALID, COMMA, PERIOD), /* 0x30 - 0x37 */
	K8(SLASH, CAPSLOCK, F1, F2, F3, F4, F5, F6), /* 0x38 - 0x3F */
	K8(F7, F8, F9, F10, F11, F12, SYSREQ, SCROLLOCK), /* 0x40 - 0x47 */
	K8(PAUSE, INSERT, HOME, PAGEUP,
	   DELETE, END, PAGEDOWN, RIGHT), /* 0x48 - 0x4F */
	K8(LEFT, UP, NUMLOCK, INVALID,
	   INVALID, INVALID, INVALID, INVALID), /* 0x50 - 0x57 */
	EIGHT_INVALID, /* 0x58 - 0x5F */
	EIGHT_INVALID, /* 0x60 - 0x67 */
	EIGHT_INVALID, /* 0x68 - 0x6F */
	EIGHT_INVALID, /* 0x70 - 0x77 */
	EIGHT_INVALID, /* 0x78 - 0x7F */
	K8(INVALID, INVALID, INVALID, INVALID,
	   INVALID, INVALID, INVALID, BACKSLASH), /* 0x80 - 0x87 */
	EIGHT_INVALID, /* 0x88 - 0x8F */
	},
	/* Shifted values */
	{
	K8(INVALID, INVALID, INVALID, INVALID, a, b, c, d), /* 0x00 - 0x07 */
	K8(e, f, g, h, i, j, k, l), /* 0x08 - 0x0F */
	K8(m, n, o, p, q, r, s, t), /* 0x10 - 0x17 */
	K8(u, v, w, x, y, z, EXCLAIM, QUOTEDBL), /* 0x18 - 0x1F */
	K8(HASH, DOLLAR, INVALID, AMPERSAND,
	   QUOTE, LEFTPAREN, RIGHTPAREN, TILDE), /* 0x20 - 0x27 */
	K8(RETURN, ESCAPE, BACKSPACE, TAB,
	   SPACE, EQUALS, INVALID, BACKQUOTE), /* 0x28 - 0x2F */
	K8(INVALID, INVALID, INVALID, PLUS,
	   ASTERISK, INVALID, LESS, GREATER), /* 0x30 - 0x37 */
	K8(QUESTION, CAPSLOCK, F1, F2, F3, F4, F5, F6), /* 0x38 - 0x3F */
	K8(F7, F8, F9, F10, F11, F12, SYSREQ, SCROLLOCK), /* 0x40 - 0x47 */
	K8(PAUSE, INSERT, HOME, PAGEUP,
	   DELETE, END, PAGEDOWN, RIGHT), /* 0x48 - 0x4F */
	K8(LEFT, UP, NUMLOCK, INVALID,
	   INVALID, INVALID, INVALID, INVALID), /* 0x50 - 0x57 */
	EIGHT_INVALID, /* 0x58 - 0x5F */
	EIGHT_INVALID, /* 0x60 - 0x67 */
	EIGHT_INVALID, /* 0x68 - 0x6F */
	EIGHT_INVALID, /* 0x70 - 0x77 */
	EIGHT_INVALID, /* 0x78 - 0x7F */
	K8(INVALID, INVALID, INVALID, INVALID,
	   INVALID, INVALID, INVALID, UNDERSCORE), /* 0x80 - 0x87 */
	EIGHT_INVALID, /* 0x88 - 0x8F */
	},
	/* Alt values */
	{
	EIGHT_INVALID,
	},
},
/* US/QWERTY keyboard */
{
	/* Base values */
	{
	K8(INVALID, INVALID, INVALID, INVALID, a, b, c, d), /* 0x00 - 0x07 */
	K8(e, f, g, h, i, j, k, l), /* 0x08 - 0x0F */
	K8(m, n, o, p, q, r, s, t), /* 0x10 - 0x17 */
	K8(u, v, w, x, y, z, 1, 2), /* 0x18 - 0x1F */
	K8(3, 4, 5, 6, 7, 8, 9, 0), /* 0x20 - 0x27 */
	K8(RETURN, ESCAPE, BACKSPACE, TAB,
	   SPACE, MINUS, EQUALS, LEFTBRACKET), /* 0x28 - 0x2F */
	K8(RIGHTBRACKET, BACKSLASH, INVALID, SEMICOLON,
	   QUOTE, BACKQUOTE, COMMA, PERIOD), /* 0x30 - 0x37 */
	K8(SLASH, CAPSLOCK, F1, F2, F3, F4, F5, F6), /* 0x38 - 0x3F */
	K8(F7, F8, F9, F10, F11, F12, SYSREQ, SCROLLOCK), /* 0x40 - 0x47 */
	K8(PAUSE, INSERT, HOME, PAGEUP,
	   DELETE, END, PAGEDOWN, RIGHT), /* 0x48 - 0x4F */
	K8(LEFT, UP, NUMLOCK, INVALID,
	   KP_DIVIDE, KP_MULTIPLY, KP_MINUS, KP_PLUS), /* 0x50 - 0x57 */
	K8(KP_ENTER, KP1, KP2, KP3, KP4, KP5, KP6, KP7), /* 0x58 - 0x5F */
	K8(KP8, KP9, KP0, KP_PERIOD,
	   INVALID, INVALID, INVALID, INVALID), /* 0x60 - 0x67 */
	},
	/* Shifted values */
	{
	K8(INVALID, INVALID, INVALID, INVALID, a, b, c, d), /* 0x00 - 0x07 */
	K8(e, f, g, h, i, j, k, l), /* 0x08 - 0x0F */
	K8(m, n, o, p, q, r, s, t), /* 0x10 - 0x17 */
	K8(u, v, w, x, y, z, EXCLAIM, AT), /* 0x18 - 0x1F */
	K8(HASH, DOLLAR, INVALID, CARET,
	   AMPERSAND, ASTERISK, LEFTPAREN, RIGHTPAREN), /* 0x20 - 0x27 */
	K8(RETURN, ESCAPE, BACKSPACE, TAB,
	   SPACE, UNDERSCORE, PLUS, INVALID), /* 0x28 - 0x2F */
	K8(INVALID, INVALID, INVALID, COLON,
	   QUOTEDBL, TILDE, LESS, GREATER), /* 0x30 - 0x37 */
	K8(QUESTION, CAPSLOCK, F1, F2, F3, F4, F5, F6), /* 0x38 - 0x3F */
	K8(F7, F8, F9, F10, F11, F12, SYSREQ, SCROLLOCK), /* 0x40 - 0x47 */
	K8(PAUSE, INSERT, HOME, PAGEUP,
	   DELETE, END, PAGEDOWN, RIGHT), /* 0x48 - 0x4F */
	K8(LEFT, UP, NUMLOCK, INVALID,
	   KP_DIVIDE, KP_MULTIPLY, KP_MINUS, KP_PLUS), /* 0x50 - 0x57 */
	K8(KP_ENTER, KP1, KP2, KP3, KP4, KP5, KP6, KP7), /* 0x58 - 0x5F */
	K8(KP8, KP9, KP0, KP_PERIOD,
	   INVALID, INVALID, INVALID, INVALID), /* 0x60 - 0x67 */
	},
	/* Alt values */
	{
	EIGHT_INVALID,
	},
},
/* UK/QWERTY keyboard */
{
	/* Base values */
	{
	K8(INVALID, INVALID, INVALID, INVALID, a, b, c, d), /* 0x00 - 0x07 */
	K8(e, f, g, h, i, j, k, l), /* 0x08 - 0x0F */
	K8(m, n, o, p, q, r, s, t), /* 0x10 - 0x17 */
	K8(u, v, w, x, y, z, 1, 2), /* 0x18 - 0x1F */
	K8(3, 4, 5, 6, 7, 8, 9, 0), /* 0x20 - 0x27 */
	K8(RETURN, ESCAPE, BACKSPACE, TAB,
	   SPACE, MINUS, EQUALS, LEFTBRACKET), /* 0x28 - 0x2F */
	K8(RIGHTBRACKET, BACKSLASH, HASH, SEMICOLON,
	   QUOTE, BACKQUOTE, COMMA, PERIOD), /* 0x30 - 0x37 */
	K8(SLASH, CAPSLOCK, F1, F2, F3, F4, F5, F6), /* 0x38 - 0x3F */
	K8(F7, F8, F9, F10, F11, F12, SYSREQ, SCROLLOCK), /* 0x40 - 0x47 */
	K8(PAUSE, INSERT, HOME, PAGEUP,
	   DELETE, END, PAGEDOWN, RIGHT), /* 0x48 - 0x4F */
	K8(LEFT, UP, NUMLOCK, INVALID,
	   KP_DIVIDE, KP_MULTIPLY, KP_MINUS, KP_PLUS), /* 0x50 - 0x57 */
	K8(KP_ENTER, KP1, KP2, KP3, KP4, KP5, KP6, KP7), /* 0x58 - 0x5F */
	K8(KP8, KP9, KP0, KP_PERIOD,
	   BACKSLASH, INVALID, INVALID, INVALID), /* 0x60 - 0x67 */
	},
	/* Shifted values */
	{
	K8(INVALID, INVALID, INVALID, INVALID, a, b, c, d), /* 0x00 - 0x07 */
	K8(e, f, g, h, i, j, k, l), /* 0x08 - 0x0F */
	K8(m, n, o, p, q, r, s, t), /* 0x10 - 0x17 */
	K8(u, v, w, x, y, z, EXCLAIM, QUOTEDBL), /* 0x18 - 0x1F */
	K8(INVALID, DOLLAR, INVALID, CARET,
	   AMPERSAND, ASTERISK, LEFTPAREN, RIGHTPAREN), /* 0x20 - 0x27 */
	K8(RETURN, ESCAPE, BACKSPACE, TAB,
	   SPACE, UNDERSCORE, PLUS, INVALID), /* 0x28 - 0x2F */
	K8(INVALID, INVALID, TILDE, COLON,
	   AT, INVALID, LESS, GREATER), /* 0x30 - 0x37 */
	K8(QUESTION, CAPSLOCK, F1, F2, F3, F4, F5, F6), /* 0x38 - 0x3F */
	K8(F7, F8, F9, F10, F11, F12, SYSREQ, SCROLLOCK), /* 0x40 - 0x47 */
	K8(PAUSE, INSERT, HOME, PAGEUP,
	   DELETE, END, PAGEDOWN, RIGHT), /* 0x48 - 0x4F */
	K8(LEFT, UP, NUMLOCK, INVALID,
	   KP_DIVIDE, KP_MULTIPLY, KP_MINUS, KP_PLUS), /* 0x50 - 0x57 */
	K8(KP_ENTER, KP1, KP2, KP3, KP4, KP5, KP6, KP7), /* 0x58 - 0x5F */
	K8(KP8, KP9, KP0, KP_PERIOD,
	   INVALID, INVALID, INVALID, INVALID), /* 0x60 - 0x67 */
	},
	/* Alt values */
	{
	EIGHT_INVALID,
	},
},
/* German/QWERTZ keyboard */
{
	/* Base values */
	{
	K8(INVALID, INVALID, INVALID, INVALID, a, b, c, d), /* 0x00 - 0x07 */
	K8(e, f, g, h, i, j, k, l), /* 0x08 - 0x0F */
	K8(m, n, o, p, q, r, s, t), /* 0x10 - 0x17 */
	K8(u, v, w, x, z, y, 1, 2), /* 0x18 - 0x1F */
	K8(3, 4, 5, 6, 7, 8, 9, 0), /* 0x20 - 0x27 */
	K8(RETURN, ESCAPE, BACKSPACE, TAB,
	   SPACE, INVALID, QUOTE, INVALID), /* 0x28 - 0x2F */
	K8(PLUS, BACKSLASH, HASH, INVALID,
	   INVALID, CARET, COMMA, PERIOD), /* 0x30 - 0x37 */
	K8(MINUS, CAPSLOCK, F1, F2, F3, F4, F5, F6), /* 0x38 - 0x3F */
	K8(F7, F8, F9, F10, F11, F12, SYSREQ, SCROLLOCK), /* 0x40 - 0x47 */
	K8(PAUSE, INSERT, HOME, PAGEUP,
	   DELETE, END, PAGEDOWN, RIGHT), /* 0x48 - 0x4F */
	K8(LEFT, UP, NUMLOCK, INVALID,
	   KP_DIVIDE, KP_MULTIPLY, KP_MINUS, KP_PLUS), /* 0x50 - 0x57 */
	K8(KP_ENTER, KP1, KP2, KP3, KP4, KP5, KP6, KP7), /* 0x58 - 0x5F */
	K8(KP8, KP9, KP0, KP_PERIOD,
	   LESS, INVALID, INVALID, INVALID), /* 0x60 - 0x67 */
	},
	/* Shifted values */
	{
	K8(INVALID, INVALID, INVALID, INVALID, a, b, c, d), /* 0x00 - 0x07 */
	K8(e, f, g, h, i, j, k, l), /* 0x08 - 0x0F */
	K8(m, n, o, p, q, r, s, t), /* 0x10 - 0x17 */
	K8(u, v, w, x, z, y, EXCLAIM, QUOTEDBL), /* 0x18 - 0x1F */
	K8(INVALID, DOLLAR, INVALID, AMPERSAND,
	   SLASH, LEFTPAREN, RIGHTPAREN, EQUALS), /* 0x20 - 0x27 */
	K8(RETURN, ESCAPE, BACKSPACE, TAB,
	   SPACE, QUESTION, BACKQUOTE, INVALID), /* 0x28 - 0x2F */
	K8(ASTERISK, INVALID, QUOTE, INVALID,
	   INVALID, INVALID, SEMICOLON, COLON), /* 0x30 - 0x37 */
	K8(UNDERSCORE, CAPSLOCK, F1, F2, F3, F4, F5, F6), /* 0x38 - 0x3F */
	K8(F7, F8, F9, F10, F11, F12, SYSREQ, SCROLLOCK), /* 0x40 - 0x47 */
	K8(PAUSE, INSERT, HOME, PAGEUP,
	   DELETE, END, PAGEDOWN, RIGHT), /* 0x48 - 0x4F */
	K8(LEFT, UP, NUMLOCK, INVALID,
	   KP_DIVIDE, KP_MULTIPLY, KP_MINUS, KP_PLUS), /* 0x50 - 0x57 */
	K8(KP_ENTER, KP1, KP2, KP3, KP4, KP5, KP6, KP7), /* 0x58 - 0x5F */
	K8(KP8, KP9, KP0, KP_PERIOD,
	   LESS, INVALID, INVALID, INVALID), /* 0x60 - 0x67 */
	},
	/* Alt values */
	{
	EIGHT_INVALID, /* 0x00 - 0x07 */
	EIGHT_INVALID, /* 0x08 - 0x0F */
	EIGHT_INVALID, /* 0x10 - 0x17 */
	EIGHT_INVALID, /* 0x18 - 0x1F */
	K8(INVALID, INVALID, INVALID, INVALID,
	   INVALID, LEFTBRACKET, RIGHTBRACKET, INVALID), /* 0x20 - 0x27 */
	K8(INVALID, INVALID, INVALID, INVALID,
	   INVALID, BACKSLASH, INVALID, INVALID), /* 0x28 - 0x2F */
	K8(TILDE, INVALID, INVALID, INVALID,
	   INVALID, INVALID, INVALID, INVALID), /* 0x30 - 0x37 */
	EIGHT_INVALID, /* 0x38 - 0x3F */
	},
},
/* French/AZERTY keyboard */
{
	{ },
	{ },
	{ },
},
/* Italian/QWERTY keyboard */
{
	{ },
	{ },
	{ },
},
/* Spanish/QWERTY keyboard */
{
	/* Base values */
	{
	K8(INVALID, INVALID, INVALID, INVALID, a, b, c, d), /* 0x00 - 0x07 */
	K8(e, f, g, h, i, j, k, l), /* 0x08 - 0x0F */
	K8(m, n, o, p, q, r, s, t), /* 0x10 - 0x17 */
	K8(u, v, w, x, y, z, 1, 2), /* 0x18 - 0x1F */
	K8(3, 4, 5, 6, 7, 8, 9, 0), /* 0x20 - 0x27 */
	K8(RETURN, ESCAPE, BACKSPACE, TAB,
	   SPACE, QUOTE, INVALID, BACKQUOTE), /* 0x28 - 0x2F */
	K8(PLUS, INVALID, INVALID, INVALID,
	   INVALID, INVALID, COMMA, PERIOD), /* 0x30 - 0x37 */
	K8(MINUS, CAPSLOCK, F1, F2, F3, F4, F5, F6), /* 0x38 - 0x3F */
	K8(F7, F8, F9, F10, F11, F12, SYSREQ, SCROLLOCK), /* 0x40 - 0x47 */
	K8(PAUSE, INSERT, HOME, PAGEUP,
	   DELETE, END, PAGEDOWN, RIGHT), /* 0x48 - 0x4F */
	K8(LEFT, UP, NUMLOCK, INVALID,
	   KP_DIVIDE, KP_MULTIPLY, KP_MINUS, KP_PLUS), /* 0x50 - 0x57 */
	K8(KP_ENTER, KP1, KP2, KP3, KP4, KP5, KP6, KP7), /* 0x58 - 0x5F */
	K8(KP8, KP9, KP0, KP_PERIOD,
	   LESS, INVALID, INVALID, INVALID), /* 0x60 - 0x67 */
	},
	/* Shifted values */
	{
	K8(INVALID, INVALID, INVALID, INVALID, a, b, c, d), /* 0x00 - 0x07 */
	K8(e, f, g, h, i, j, k, l), /* 0x08 - 0x0F */
	K8(m, n, o, p, q, r, s, t), /* 0x10 - 0x17 */
	K8(u, v, w, x, y, z, EXCLAIM, QUOTEDBL), /* 0x18 - 0x1F */
	K8(INVALID, DOLLAR, INVALID, AMPERSAND,
	   SLASH, LEFTPAREN, RIGHTPAREN, EQUALS), /* 0x20 - 0x27 */
	K8(RETURN, ESCAPE, BACKSPACE, TAB,
	   SPACE, QUESTION, INVALID, CARET), /* 0x28 - 0x2F */
	K8(ASTERISK, INVALID, INVALID, INVALID,
	   INVALID, INVALID, SEMICOLON, COLON), /* 0x30 - 0x37 */
	K8(UNDERSCORE, CAPSLOCK, F1, F2, F3, F4, F5, F6), /* 0x38 - 0x3F */
	K8(F7, F8, F9, F10, F11, F12, SYSREQ, SCROLLOCK), /* 0x40 - 0x47 */
	K8(PAUSE, INSERT, HOME, PAGEUP,
	   DELETE, END, PAGEDOWN, RIGHT), /* 0x48 - 0x4F */
	K8(LEFT, UP, NUMLOCK, INVALID,
	   KP_DIVIDE, KP_MULTIPLY, KP_MINUS, KP_PLUS), /* 0x50 - 0x57 */
	K8(KP_ENTER, KP1, KP2, KP3, KP4, KP5, KP6, KP7), /* 0x58 - 0x5F */
	K8(KP8, KP9, KP0, KP_PERIOD,
	   LESS, INVALID, INVALID, INVALID), /* 0x60 - 0x67 */
	},
	/* Alt values */
	{
	EIGHT_INVALID, /* 0x00 - 0x07 */
	EIGHT_INVALID, /* 0x08 - 0x0F */
	EIGHT_INVALID, /* 0x10 - 0x17 */
	K8(INVALID, INVALID, INVALID, INVALID,
	   INVALID, INVALID, INVALID, AT), /* 0x18 - 0x1F */
	K8(HASH, INVALID, INVALID, INVALID,
	   INVALID, INVALID, INVALID, INVALID), /* 0x20 - 0x27 */
	K8(INVALID, INVALID, INVALID, INVALID,
	   INVALID, INVALID, INVALID, LEFTBRACKET), /* 0x28 - 0x2F */
	K8(RIGHTBRACKET, INVALID, INVALID, INVALID,
	   INVALID, BACKSLASH, INVALID, INVALID), /* 0x30 - 0x37 */
	K8(MINUS, INVALID, INVALID, INVALID,
	   INVALID, INVALID, INVALID, INVALID), /* 0x38 - 0x3F */
	EIGHT_INVALID, /* 0x40 - 0x47 */
	},
},



};

Common::KeyCode ScancodeToOSystemKeycode(
    int region, int shift_keys, uint8 scancode) {
	if (shift_keys & (KBD_MOD_LSHIFT | KBD_MOD_RSHIFT))
		return keymaps[region-1].shifted[scancode];
	else if (shift_keys & (KBD_MOD_LALT | KBD_MOD_RALT))
		return keymaps[region-1].alt[scancode];
	else
		return keymaps[region-1].base[scancode];
}

int OSystemKeycodeToAscii(Common::KeyCode key, int shift_keys) {
	if (key >= Common::KEYCODE_F1 && key <= Common::KEYCODE_F12)
		return key - Common::KEYCODE_F1 + Common::ASCII_F1;
	else if (key >= Common::KEYCODE_KP0 && key <= Common::KEYCODE_KP9)
		return key - Common::KEYCODE_KP0 + '0';
	else if (key >= 'a' && key <= 'z' &&
	         shift_keys & (KBD_MOD_LSHIFT | KBD_MOD_RSHIFT))
		return key & ~0x20;
	else
		return key;
}
