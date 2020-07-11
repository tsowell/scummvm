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

#include <stdint.h>
#include <malloc.h>

#include <dc/video.h>
#include <dc/pvr.h>

#include "backends/platform/dcalt/graphics.h"
#include "backends/modular-backend.h"
#include "common/config-manager.h"

#define PF_RGB565 (Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0))
#define PF_ARGB1555 (Graphics::PixelFormat(2, 5, 5, 5, 1, 10, 5, 0, 15))
#define PF_ARGB4444 (Graphics::PixelFormat(2, 4, 4, 4, 4, 8, 4, 0, 12))
#define PF_CLUT8 (Graphics::PixelFormat::createFormatCLUT8())

static int
_to_texture_dimension(int x) {
	if (x <= 8)
		return 8;
	else if (x <= 16)
		return 16;
	else if (x <= 32)
		return 32;
	else if (x <= 64)
		return 64;
	else if (x <= 128)
		return 128;
	else if (x <= 256)
		return 256;
	else if (x <= 512)
		return 512;
	else
		return 1024;
}

static int toPVRFormat(const Graphics::PixelFormat format) {
	if (format == PF_RGB565) {
		return PVR_TXRFMT_RGB565;
	}
	else if (format == PF_ARGB1555) {
		return PVR_TXRFMT_ARGB1555;
	}
	else if (format == PF_ARGB4444) {
		return PVR_TXRFMT_ARGB4444;
	}
	else {
		return PVR_TXRFMT_RGB565;
	}
}

RGBSurface::RGBSurface(int w, int h, int pixelFormat, int filteringMode) {
	size_t texture_size, pixels_size;

	_width = w;
	_height = h;
	// align stride to 32 bytes
	if (!(_width & 0x1F))
		_stride = _width;
	else
		_stride = (_width & ~0x1F) + 32;
	_texture_width = _to_texture_dimension(_stride);
	_texture_height = _to_texture_dimension(_height);

	pixels_size = _stride * _height * sizeof(uint16_t);
	_pixels = (void *)memalign(32, pixels_size);
	memset((uint8_t *)_pixels, 0, pixels_size);

	texture_size = _texture_width * _texture_height * sizeof(uint16_t);
	_texture = (pvr_ptr_t)pvr_mem_malloc(texture_size);

        pvr_poly_cxt_txr(&_cxt,
                         pixelFormat == PVR_TXRFMT_RGB565 ? PVR_LIST_OP_POLY
	                                                  : PVR_LIST_TR_POLY,
                         pixelFormat |
                         PVR_TXRFMT_NONTWIDDLED,
                         _texture_width, _texture_height,
			 _texture,
                         filteringMode);
        pvr_poly_compile(&_poly, &_cxt);
}

RGBSurface::~RGBSurface() {
	if (_pixels)
		free(_pixels);
	if (_texture)
		pvr_mem_free(_texture);
}

void RGBSurface::setFilteringMode(int filteringMode) {
	_cxt.txr.filter = filteringMode;
	pvr_poly_compile(&_poly, &_cxt);
}

void RGBSurface::fill(uint32 col) {
	sq_set((uint16_t *)_pixels, col, _stride * _height * sizeof(uint16_t));
}

void RGBSurface::clear() {
	sq_clr((uint16_t *)_pixels, _stride * _height * sizeof(uint16_t));
}

void RGBSurface::grab(void *buf, int pitch) const {
	int h = _height;
	uint16 *src = (uint16 *)_pixels;
	byte *dst = (byte *)buf;

	do {
		memcpy(dst, src, _stride * sizeof(uint16));
		src += _stride;
		dst += pitch;
	} while (--h);
}

void RGBSurface::copyRect(const void *buf,
                          int pitch, int x, int y, int w, int h) {
	const byte *src = (const byte *)buf;
	if (x < 0) {
		w += x;
		src -= x * sizeof(uint16);
		x = 0;
	}

	if (y < 0) {
		h += y;
		src -= y * pitch;
		y = 0;
	}

	if (w > _width - x)
		w = _width - x;

	if (h > _height - y)
		h = _height - y;

	if (w <= 0 || h <= 0)
		return;

	uint16 *dst = ((uint16 *)_pixels) + (y * _stride + x);
	if (_width == (uint16)w && (uint16)pitch == _stride * sizeof(uint16)) {
		memcpy(dst, src, h * pitch);
	} else {
		do {
			memcpy(dst, src, w * sizeof(uint16));
			src += pitch;
			dst += _stride;
		} while (--h);
	}
}

void RGBSurface::loadTexture() {
	int r;
	for (r = 0; r < _height; r++) {
		sq_cpy((uint16_t *)_texture + (r * _texture_width),
		       ((uint16_t *)_pixels) + (r * _stride),
		       _width * sizeof(uint16_t));
	}
}

void RGBSurface::draw(int x, int y, float z,
		      float scale_x, float scale_y) {
        pvr_vertex_t vert;

        pvr_prim(&_poly, sizeof(pvr_poly_hdr_t));

        vert.flags = PVR_CMD_VERTEX;
        vert.x = x;
        vert.y = y + (_height * scale_y);
        vert.z = z;
        vert.u = 0.0f;
        vert.v = _height / (float)_texture_height;
        vert.argb = 0xffffffff;
        vert.oargb = 0;
        pvr_prim(&vert, sizeof(vert));

        vert.y = y;
        vert.v = 0.0f;
        pvr_prim(&vert, sizeof(vert));

        vert.x = x + (_width * scale_y);
        vert.y = y + (_height * scale_y);
        vert.u = _width / (float)_texture_width;
        vert.v = _height / (float)_texture_height;
        pvr_prim(&vert, sizeof(vert));

        vert.flags = PVR_CMD_VERTEX_EOL;
        vert.y = y;
        vert.v = 0.0f;
        pvr_prim(&vert, sizeof(vert));
}

static int
_to_vq_width(int x) {
	if (x <= 32)
                return 32;
        else if (x <= 64)
                return 64;
        else if (x <= 128)
                return 128;
        else
                return 256;
}

static int
_align32(int x)
{
	if ((x % 32) == 0)
		return x;
	else
		return ((x / 32) * 32) + 32;
}

VQSurface::VQSurface(int w, int h, int format, int filteringMode) {
	int i;

	memset((uint8_t *)_palette, 0, 1024 * sizeof(uint16_t));

	_width = w;
	_height = h;
	_stride = _align32(w);
	_texture_height = _to_texture_dimension(h);

	_pixels = (uint8_t *)memalign(32, _stride * _height);
	memset((uint8_t *)_pixels, 0, _stride * _height);

	_tiles_count = (_stride % 256) ? (_stride / 256) : (_stride / 256 - 1);
	_last_tile_stride = (_stride % 256) ? _stride % 256 : 256;
	_last_tile_width = (_width % 256) ? _width % 256 : 256;
	_last_tile_texture_width = _to_vq_width(_last_tile_stride);

	_tiles = (struct vqtile *)malloc(sizeof(struct vqtile) * _tiles_count);
	for (i = 0; i < _tiles_count; i++) {
		_tiles[i].texture =
		    pvr_mem_malloc(2048 + 256 * _texture_height);
		pvr_poly_cxt_txr(&_tiles[i].cxt,
		                 PVR_LIST_OP_POLY,
		                 format |
				 PVR_TXRFMT_VQ_ENABLE |
				 PVR_TXRFMT_NONTWIDDLED,
		                 256 * 4, _texture_height,
		                 _tiles[i].texture,
		                 filteringMode);
		pvr_poly_compile(&_tiles[i].poly, &_tiles[i].cxt);
	}

	_last_tile.texture =
	    pvr_mem_malloc(2048 + _last_tile_texture_width * _texture_height);
	pvr_poly_cxt_txr(&_last_tile.cxt,
	                 PVR_LIST_OP_POLY,
			 format |
			 PVR_TXRFMT_VQ_ENABLE |
			 PVR_TXRFMT_NONTWIDDLED,
	                 _last_tile_texture_width * 4, _texture_height,
	                 _last_tile.texture,
	                 filteringMode);
	pvr_poly_compile(&_last_tile.poly, &_last_tile.cxt);
}

void VQSurface::setFilteringMode(int filteringMode) {
	int i;
	for (i = 0; i < _tiles_count; i++) {
		_tiles[i].cxt.txr.filter = filteringMode;
		pvr_poly_compile(&_tiles[i].poly, &_tiles[i].cxt);
	}
	_last_tile.cxt.txr.filter = filteringMode;
	pvr_poly_compile(&_last_tile.poly, &_last_tile.cxt);
}

void VQSurface::fill(uint32 col) {
	sq_set((uint8_t *)_pixels, col, _stride * _height);
}

void VQSurface::clear() {
	sq_clr((uint8_t *)_pixels, _stride * _height);
}

void VQSurface::grab(void *buf, int pitch) const {
	int h = _height;
        uint8 *src = (uint8 *)_pixels;
        byte *dst = (byte *)buf;

        do {
                memcpy(dst, src, _width * sizeof(uint8));
                src += _stride;
                dst += pitch;
        } while (--h);
}

void VQSurface::copyRect(const void *buf, int pitch,
                         int x, int y, int w, int h) {
        const byte *src = (const byte *)buf;
        if (x < 0) {
                w += x;
                src -= x * sizeof(uint16);
                x = 0;
        }

        if (y < 0) {
                h += y;
                src -= y * pitch;
                y = 0;
        }

        if (w > _width - x)
                w = _width - x;

        if (h > _height - y)
                h = _height - y;

        if (w <= 0 || h <= 0)
                return;

        uint8 *dst = ((uint8 *)_pixels) + (y * _stride + x);
	if (_width == (uint16)w && (uint16)pitch == _stride * sizeof(uint8)) {
                memcpy(dst, src, h * pitch);
        } else {
                do {
                        memcpy(dst, src, w * sizeof(uint8));
                        src += pitch;
                        dst += _stride;
                } while (--h);
        }
}

void VQSurface::draw(int x, int y, float z,
		     float scale_x, float scale_y) {
	int i;

	pvr_vertex_t vert;

	for (i = 0; i < _tiles_count; i++) {
		pvr_prim(&_tiles[i].poly, sizeof(pvr_poly_hdr_t));

		vert.flags = PVR_CMD_VERTEX;
		vert.x = x + (i * 256) * scale_x;
		vert.y = y + (_height) * scale_y;
		vert.z = z;
		vert.u = 0.0f;
		vert.v = _height / (float)_texture_height;
		vert.argb = 0xffffffff;
		vert.oargb = 0;
		pvr_prim(&vert, sizeof(vert));

		vert.y = y;
		vert.v = 0.0f;
		pvr_prim(&vert, sizeof(vert));

		vert.x = x + ((i * 256) + 256) * scale_x;
		vert.y = y + (_height) * scale_y;
		vert.u = 1.0f;
		vert.v = _height / (float)_texture_height;
		pvr_prim(&vert, sizeof(vert));

		vert.flags = PVR_CMD_VERTEX_EOL;
		vert.y = y;
		vert.v = 0.0f;
		pvr_prim(&vert, sizeof(vert));
	}
	pvr_prim(&_last_tile.poly, sizeof(pvr_poly_hdr_t));

	vert.flags = PVR_CMD_VERTEX;
	vert.x = x + ((_tiles_count * 256)) * scale_x;
	vert.y = y + (_height) * scale_y;
	vert.z = z;
	vert.u = 0.0f;
	vert.v = _height / (float)_texture_height;
	vert.argb = 0xffffffff;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));

	vert.y = y;
	vert.v = 0.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x + ((_tiles_count * 256) + _last_tile_width) * scale_x;
	vert.y = y + (_height) * scale_y;
	vert.u = _last_tile_width / (float)_last_tile_texture_width;
	vert.v = _height / (float)_texture_height;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.y = y;
	vert.v = 0.0f;
	pvr_prim(&vert, sizeof(vert));
}

void VQSurface::setPalette(const byte *colors, uint start, uint num) {
        const byte *s = colors;
        uint16_t color;
        int i;
        for (i = start; i < (int)(start + num); i++, s += 3) {
                color = ((s[0] << 8) & 0xf800) |
                        ((s[1] << 3) & 0x07e0) |
                        ((s[2] >> 3) & 0x001f);
                (_palette)[i*4+0] = color;
                (_palette)[i*4+1] = color;
                (_palette)[i*4+2] = color;
                (_palette)[i*4+3] = color;
        }
}

void VQSurface::grabPalette(byte *colors, uint start, uint num) {
        byte *s = colors;
	int i;
        for (i = start; i < (int)(start + num); i++, s += 3) {
		s[0] = (_palette[i*4+0] & 0xf800) >> 8;
		s[1] = (_palette[i*4+0] & 0x07e0) >> 3;
		s[2] = (_palette[i*4+0] & 0x001f) << 3;
	}
}

void VQSurface::loadTexture() {
	int i, r;
	uint8_t *dst, *src;
	for (i = 0; i < _tiles_count; i++) {
		src = ((uint8_t *)_pixels) + (i * 256);
		dst = ((uint8_t *)_tiles[i].texture) + 2048;
		for (r = 0; r < _height; r++) {
			sq_cpy(dst, src, 256);
			src += _stride;
			dst += 256;
		}
	}
	src = ((uint8_t *)_pixels) + (_tiles_count * 256);
	dst = ((uint8_t *)_last_tile.texture) + 2048;
	for (r = 0; r < _height; r++) {
		sq_cpy(dst, src, _last_tile_stride);
		src += _stride;
		dst += _last_tile_texture_width;
	}
}

void VQSurface::loadPalette() {
	int i;
        uint8_t *dst, *src;
	src = (uint8_t *)_palette;
        for (i = 0; i < _tiles_count; i++) {
                dst = (uint8_t *)_tiles[i].texture;
                sq_cpy(dst, src, 2048);
        }
	dst = (uint8_t *)_last_tile.texture;
	sq_cpy(dst, src, 2048);
}

VQSurface::~VQSurface() {
	int i;

	if (_pixels)
		free(_pixels);
	if (_last_tile.texture)
		pvr_mem_free(_last_tile.texture);
	for (i = 0; i < _tiles_count; i++) {
		if (_tiles[i].texture)
			pvr_mem_free(_tiles[i].texture);
	}
	if (_tiles)
		free(_tiles);
}

Mouse::Mouse() :
    _visible(false),
    _x(0),
    _y(0),
    _w(0),
    _h(0),
    _texture_w(0),
    _texture_h(0),
    _dontScale(false),
    _format(PF_CLUT8),
    _cursorDirty(false),
    _paletteDirty(false),
    _palette(NULL),
    _keycolor(0),
    _pixels(NULL),
    _texture(NULL),
    _cursorPaletteDisabled(true),
    _filteringMode(PVR_FILTER_NONE) {
	_palette = (uint16_t *)memalign(32, 2048);
	_screenPalette = (uint16_t *)memalign(32, 2048);
	changeFormat(16, 16, PF_CLUT8);
}

Mouse::~Mouse() {
	if (_pixels)
		free(_pixels);
	if (_texture)
		pvr_mem_free(_texture);
	if (_palette)
		free(_palette);
	if (_screenPalette)
		free(_screenPalette);
}

void Mouse::setFilteringMode(int filteringMode) {
	_filteringMode = filteringMode;
	_cxt.txr.filter = _filteringMode;
	pvr_poly_compile(&_poly, &_cxt);
}

bool Mouse::show(bool visible) {
	bool last = _visible;

	_visible = visible;

	return last;
}

void Mouse::warp(int x, int y) {
	_x = x;
	_y = y;
}

void Mouse::load() {
	int i, j;
	uint16_t *dst, *src;

	if (!_visible || (_w == 0) || (_h == 0)) {
		return;
	}

	if (_pixels && _texture) {
		if (_paletteDirty && (_format == PF_CLUT8)) {
			if (!_cursorPaletteDisabled) {
				pvr_txr_load(_palette, _texture, 2048);
			}
			else {
				pvr_txr_load(_screenPalette, _texture, 2048);
			}
			((uint16_t *)_texture)[_keycolor * 4 + 0] = 0x0000;
			((uint16_t *)_texture)[_keycolor * 4 + 1] = 0x0000;
			((uint16_t *)_texture)[_keycolor * 4 + 2] = 0x0000;
			((uint16_t *)_texture)[_keycolor * 4 + 3] = 0x0000;
		}

		// We also have to reload the cursor in 16-bit color mode if
		// the palette is dirty because it indicates that the key color
		// has changed.
		if (_cursorDirty || (_paletteDirty && (_format != PF_CLUT8))) {
			src = (uint16_t *)_pixels;
			if (_format.bytesPerPixel == 2) {
				dst = (uint16_t *)_texture;
				for (i = 0; i < _texture_h; i++) {
					for (j = 0; j < _texture_w; j++) {
						if (*dst != _keycolor)
							*dst = *src;
						else
							*dst = 0;
						dst++;
						src++;
					}
				}
			}
			else {
				dst = (uint16_t *)(((uint8_t *)_texture) + 2048);
				memcpy(dst, src, _texture_w * _texture_h);
			}
		}
	}
}

void Mouse::draw(int offset_x, int offset_y, float scale_x, float scale_y,
                 float cursor_scale) {
	int draw_w, draw_h;
	int draw_x, draw_y;

	if (!_visible || (_w == 0) || (_h == 0)) {
		return;
	}

	if (_dontScale) {
		draw_w = _w;
		draw_h = _h;
		draw_x = (_x - _hotspotX) * scale_x + offset_x;
		draw_y = (_y - _hotspotY) * scale_y + offset_y;
	}
	else {
		draw_w = _w * scale_x * cursor_scale;
		draw_h = _h * scale_y * cursor_scale;
		draw_x = (_x - _hotspotX * cursor_scale) * scale_x + offset_x;
		draw_y = (_y - _hotspotY * cursor_scale) * scale_y + offset_y;
	}

	pvr_vertex_t vert;

	pvr_prim(&_poly, sizeof(pvr_poly_hdr_t));

	vert.flags = PVR_CMD_VERTEX;
	vert.x = draw_x + 0.0f;
	vert.y = draw_y + draw_h;
	vert.z = 15.0f;
	vert.u = 0.0f;
	vert.v = _h / (float)_texture_h;
	vert.argb = 0xffffffff;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));

	vert.y = draw_y + 0.0f;
	vert.v = 0.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.x = draw_x + draw_w;
	vert.y = draw_y + draw_h;
	vert.u = _w / (float)_texture_w;
	vert.v = _h / (float)_texture_h;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.y = draw_y + 0.0f;
	vert.v = 0.0f;
	pvr_prim(&vert, sizeof(vert));
}

void Mouse::changeFormat(uint w, uint h,
                         const Graphics::PixelFormat format) {
	if (_pixels) {
		free(_pixels);
		_pixels = NULL;
	}
	if (_texture) {
		pvr_mem_free(_texture);
		_texture = NULL;
	}


	_w = w;
	_h = h;

	_texture_w = _to_texture_dimension(_w);
	_texture_h = _to_texture_dimension(_h);
	_format = format;

	if (_texture_w == 0 || _texture_h == 0) {
		return;
	}

	// align stride to 32 bytes
	if (_texture_w & 0x1F)
		_texture_w = (_texture_w & ~0x1F) + 32;

	if (format == PF_CLUT8) {
		_pixels = (uint8_t *)memalign(32, _texture_w * _texture_h);
		_texture = pvr_mem_malloc(2048 + _texture_w * _texture_h);

		pvr_poly_cxt_txr(&_cxt,
				 PVR_LIST_TR_POLY,
				 PVR_TXRFMT_ARGB4444 |
				 PVR_TXRFMT_VQ_ENABLE |
				 PVR_TXRFMT_NONTWIDDLED,
				 _texture_w * 4, _texture_h,
				 _texture,
				 _filteringMode);
	}
	else {
		_pixels = (uint8_t *)memalign(32,
			_texture_w * _texture_h * format.bytesPerPixel);
		_texture = pvr_mem_malloc(
			_texture_w * _texture_h * format.bytesPerPixel);

		pvr_poly_cxt_txr(&_cxt,
				 PVR_LIST_TR_POLY,
				 toPVRFormat(_format) |
				 PVR_TXRFMT_NONTWIDDLED,
				 _texture_w, _texture_h,
				 _texture,
				 _filteringMode);
	}
	pvr_poly_compile(&_poly, &_cxt);
}

void Mouse::setCursor(const void *buf, uint w, uint h,
                      int hotspotX, int hotspotY, uint32 keycolor,
                      bool dontScale, const Graphics::PixelFormat *format) {
	uint8_t *dst;
	const uint8_t *src;
	Graphics::PixelFormat newFormat = (format == NULL)?PF_CLUT8:*format;
	int bytesPerPixel;

	if ((w != (uint)_w) || (h != (uint)_h) || (newFormat != _format)) {
		changeFormat(w, h, newFormat);
	}

	if ((w == 0) || (h == 0))
		return;

	bytesPerPixel = (format == 0)?1:_format.bytesPerPixel;

	if (w > 0 && h > 0 && buf) {
		src = (const uint8_t *)buf;
		dst = (uint8_t *)_pixels;
		do {
			memcpy(dst, src, w * bytesPerPixel);
			src += w * bytesPerPixel;
			dst += _texture_w * bytesPerPixel;
		} while (--h);
	}

	_hotspotX = hotspotX;
	_hotspotY = hotspotY;

	_cursorDirty = true;

	_dontScale = dontScale;

	if (_keycolor != keycolor)
		_paletteDirty = true;

	_keycolor = keycolor;
}

void Mouse::setCursorPalette(const byte *colors, uint start, uint num) {
	const byte *s = colors;
	uint16_t color;
	int i;
	for (i = start; i < (int)(start + num); i++, s += 3) {
		color = 0xf000 |
		        ((s[0] << 4) & 0x0f00) |
		        ((s[1] << 0) & 0x00f0) |
		        ((s[2] >> 4) & 0x000f);
		_palette[i*4+0] = color;
		_palette[i*4+1] = color;
		_palette[i*4+2] = color;
		_palette[i*4+3] = color;
	}

	_paletteDirty = true;
	_cursorPaletteDisabled = false;
}

void Mouse::setScreenPalette(const byte *colors, uint start, uint num) {
	const byte *s = colors;
	uint16_t color;
	int i;
	for (i = start; i < (int)(start + num); i++, s += 3) {
		color = 0xf000 |
		        ((s[0] << 4) & 0x0f00) |
		        ((s[1] << 0) & 0x00f0) |
		        ((s[2] >> 4) & 0x000f);
		_screenPalette[i*4+0] = color;
		_screenPalette[i*4+1] = color;
		_screenPalette[i*4+2] = color;
		_screenPalette[i*4+3] = color;
	}

	_paletteDirty = true;
}

void Mouse::setCursorPaletteDisabled(bool disable) {
	if (disable != _cursorPaletteDisabled)
		_paletteDirty = true;
	_cursorPaletteDisabled = disable;
}

bool Mouse::getCursorPaletteDisabled() const {
	return _cursorPaletteDisabled;
}

DCAltGraphicsManager::DCAltGraphicsManager() :
    GraphicsManager(),
    _screenChangeCount(0),
    _overlayHidden(false),
    _overlayDirty(false),
    _screenDirty(false),
    _shakeXOffset(0),
    _shakeYOffset(0),
    _screen(NULL),
    _overlay(NULL),
    _aspectRatioCorrection(false),
    _activeDomain(0),
    _screenFormat(PF_CLUT8),
    _filteringMode(PVR_FILTER_NONE)
{
	_vga = vid_check_cable() == CT_VGA;
	initOverlay(640, 480);
	_mouse = new Mouse();
}

#ifdef USE_RGB_COLOR
Graphics::PixelFormat DCAltGraphicsManager::getScreenFormat() const {
	return _screenFormat;
}

Common::List<Graphics::PixelFormat>
    DCAltGraphicsManager::getSupportedFormats() const {
	Common::List<Graphics::PixelFormat> list;
	list.push_back(PF_RGB565);
	list.push_back(PF_ARGB1555);
	list.push_back(PF_ARGB4444);
	list.push_back(PF_CLUT8);
	return list;
}

#endif

DCAltGraphicsManager::~DCAltGraphicsManager() {
	if (_overlay)
		delete _overlay;
	if (_screen)
		delete _screen;
	if (_mouse)
		delete _mouse;
}

bool DCAltGraphicsManager::hasFeature(OSystem::Feature f) const {
	return (f == OSystem::kFeatureOverlaySupportsAlpha) ||
	       (f == OSystem::kFeatureCursorPalette) ||
	       (f == OSystem::kFeatureAspectRatioCorrection) ||
	       (f == OSystem::kFeatureFilteringMode);
}

void DCAltGraphicsManager::setFeatureState(OSystem::Feature f, bool enable) {
	switch (f) {
	case OSystem::kFeatureCursorPalette:
		_mouse->setCursorPaletteDisabled(!enable);
		break;
	case OSystem::kFeatureAspectRatioCorrection:
		_aspectRatioCorrection = enable;
		break;
	case OSystem::kFeatureFilteringMode:
		if (enable)
			_filteringMode = PVR_FILTER_BILINEAR;
		else
			_filteringMode = PVR_FILTER_NONE;
		if (_screen)
			_screen->setFilteringMode(_filteringMode);
		break;
	default:
		break;
	}
}

bool DCAltGraphicsManager::getFeatureState(OSystem::Feature f) const {
	switch (f) {
	case OSystem::kFeatureCursorPalette:
		return _mouse->getCursorPaletteDisabled();
		break;
	case OSystem::kFeatureAspectRatioCorrection:
		return _aspectRatioCorrection;
		break;
	case OSystem::kFeatureFilteringMode:
		return (_filteringMode != PVR_FILTER_NONE);
		break;
	default:
		return false;
		break;
	}
}

void DCAltGraphicsManager::showOverlay() {
	int mouse_offset_y = \
	    (_vid_height - _screen->getHeight() * getScaleY()) / 2;

	_mouse->setFilteringMode(PVR_FILTER_NONE);

	warpMouse(_mouse->getX() * getScaleX(),
	          _mouse->getY() * getScaleY() + mouse_offset_y);

	_overlayHidden = false;
}

void DCAltGraphicsManager::hideOverlay() {
	int mouse_offset_y = \
	    (_vid_height - _screen->getHeight() * getScaleY()) / 2;

	_mouse->setFilteringMode(_filteringMode);

	warpMouse(_mouse->getX() / getScaleX(),
	          (_mouse->getY() - mouse_offset_y) / getScaleY());

	_overlayHidden = true;
}

void DCAltGraphicsManager::setPalette(
    const byte *colors, uint start, uint num) {
	assert(_screen);
	_screen->setPalette(colors, start, num);
	_mouse->setScreenPalette(colors, start, num);
	_screenDirty = true;
}

void DCAltGraphicsManager::grabPalette(
    byte *colors, uint start, uint num) const {
	assert(_screen);
	_screen->grabPalette(colors, start, num);
}

void DCAltGraphicsManager::copyRectToScreen(
    const void *buf, int pitch, int x, int y, int w, int h) {
	assert(_screen);
	_screen->copyRect(buf, pitch, x, y, w, h);
	_screenDirty = true;
}

void DCAltGraphicsManager::initSize(
    uint width, uint height, const Graphics::PixelFormat *format) {
	if (format == 0) {
		_screenFormat = PF_CLUT8;
	}
	else if (*format == Graphics::PixelFormat(PF_RGB565)) {
		_screenFormat = *format;
	}
	else if (*format == Graphics::PixelFormat(PF_ARGB1555)) {
		_screenFormat = *format;
	}
	else if (*format == Graphics::PixelFormat(PF_ARGB4444)) {
		_screenFormat = *format;
	}
	else {
		_screenFormat = PF_CLUT8;
	}

	_activeDomain = ConfMan.getActiveDomain();

	_screenChangeCount++;

	// Tear everything down and reinitialize graphics because we might be
	// changing graphics modes.
	if (_overlay) {
		delete _overlay;
	}
	if (_screen) {
		delete _screen;
	}
	if (_mouse) {
		delete _mouse;
	}

	pvr_shutdown();
	initOverlay(width, height);

	if (_screenFormat == PF_CLUT8) {
		_screen = new VQSurface(width, height,
		                        PVR_TXRFMT_RGB565,
		                        _filteringMode);
	}
	else {
		_screen = new RGBSurface(width, height,
					 toPVRFormat(_screenFormat),
					 _filteringMode);
	}
	_screenDirty = true;

	_mouse = new Mouse;
}

void DCAltGraphicsManager::initVideo640x480() {
	// 640x480 timing for HOLLY clocked at 50.350 MHz.  Please check my
	// work before using this with a CRT monitor!  Certain very old CRTs
	// can be permanently damaged by bad video timing.
        vid_mode_t vga_640x480_25175 =
        {
                DM_640x480,
                640, 480,
                VID_INTERLACE,
                CT_VGA,
                PM_RGB565,
                525, 800, // scanlines, clocks
                121, 35, // bitmapx, bitmapy
                2, 260, // scanline int 1, 2 (2 doubled for VGA?)
                121, 777, // border x start, border x end
                2, 525, // border y start, border y end
                0, 1,
                { 0, 0, 0, 0 }
        };

	if (_vga && ConfMan.getBool("dcalt_vga_25175")) {
		vid_set_mode_ex(&vga_640x480_25175);
	}
	else {
		vid_init(DM_640x480, PM_RGB565);
	}

	if (_vga && ConfMan.getBool("dcalt_vga_polarity")) {
		// set hsync- vsync-
		static volatile uint32 *regs = (uint32*)0xA05F8000;
		regs[0x34] &= 0xfffc;
	}
}

void DCAltGraphicsManager::initVideo640x400() {
	// 640x400 timing.  Please check my work before using any this with a
	// CRT monitor!  Certain very old CRTs can be permanently damaged by
	// bad video timing.
        vid_mode_t vga_640x400 = {
                DM_640x480,
                640, 400,
                VID_INTERLACE,
                CT_VGA,
                PM_RGB565,
                450, 857, // scanlines, clocks
                172, 40, // bitmapx, bitmapy
                21, 220, // scanline int 1, 2 (2 doubled for VGA?)
                126, 837, // border x start, border x end
                30, 430, // border y start, border y end
                0, 1,
                { 0, 0, 0, 0 }
        };
	// 640x400 timing for HOLLY clocked at 50.350 MHz.  Please check my
	// work before using any of this with a CRT monitor!  Certain very old
	// CRTs can be permanently damaged by bad video timing.
        vid_mode_t vga_640x400_25175 =
        {
                DM_640x480,
                640, 400,
                VID_INTERLACE,
                CT_VGA,
                PM_RGB565,
                449, 800, // scanlines, clocks
                121, 37, // bitmapx, bitmapy
                2, 220, // scanline int 1, 2 (2 doubled for VGA?)
                121, 777, // border x start, border x end
                2, 449, // border y start, border y end
                0, 1,
                { 0, 0, 0, 0 }
        };

	if (ConfMan.getBool("dcalt_vga_25175")) {
		vid_set_mode_ex(&vga_640x400_25175);
	}
	else {
		vid_set_mode_ex(&vga_640x400);
	}
	if (ConfMan.getBool("dcalt_vga_polarity")) {
		// set hsync- vsync+
		static volatile uint32 *regs = (uint32*)0xA05F8000;
		regs[0x34] |= 0x02;
	}
}

void DCAltGraphicsManager::initVideo320x240() {
	vid_init(DM_320x240, PM_RGB565);
}

void DCAltGraphicsManager::initOverlay(int width, int height) {
	if (!_vga) {
		// Not on VGA.  Use 320x240 for menu and games except for games
		// that run at 640x480.
		if (_activeDomain != 0 && width == 640 && height == 480) {
			initVideo640x480();
			_vid_width = 640;
			_vid_height = 480;

			pvr_init_defaults();
			pvr_set_bg_color(0.0f, 0.0f, 0.0f);

			_overlay = new RGBSurface(_vid_width, _vid_height,
			                          PVR_TXRFMT_ARGB4444,
						  _filteringMode);
		}
		else {
			initVideo320x240();
			_vid_width = 320;
			_vid_height = 240;

			pvr_init_defaults();
			pvr_set_bg_color(0.0f, 0.0f, 0.0f);

			_overlay = new RGBSurface(_vid_width, _vid_height,
			                          PVR_TXRFMT_ARGB4444,
						  _filteringMode);
		}
	}
	else if (_activeDomain == 0) {
		// On VGA, use 640x480 for the menu.
		initVideo640x480();
		_vid_width = 640;
		_vid_height = 480;

		pvr_init_defaults();
		pvr_set_bg_color(0.0f, 0.0f, 0.0f);

		_overlay = new RGBSurface(_vid_width, _vid_height,
		                          PVR_TXRFMT_ARGB4444,
					  _filteringMode);
	}
	else {
		// If VGA mode aspect ratio correction is enabled, use 640x400
		// when games request 320x200.  This will probably only work
		// reliably on multisync CRTs because the Dreamcast RAMDAC only
		// works with negative sync pulses. */
		if (_aspectRatioCorrection &&
		    vgaModeAspectRatioCorrection() &&
		    ((width == 320) && (height == 200))) {
			initVideo640x400();
                        _vid_width = 640;
                        _vid_height = 400;
			pvr_init_defaults();
			pvr_set_bg_color(0.0f, 0.0f, 0.0f);

                        _overlay = new RGBSurface(_vid_width, _vid_height,
			                          PVR_TXRFMT_ARGB4444,
						  _filteringMode);
		}
		else {
			initVideo640x480();
			_vid_width = 640;
			_vid_height = 480;
			pvr_init_defaults();
			pvr_set_bg_color(0.0f, 0.0f, 0.0f);

			_overlay = new RGBSurface(_vid_width, _vid_height,
			                          PVR_TXRFMT_ARGB4444,
						  _filteringMode);
		}
	}
	_overlayDirty = true;
}

int16 DCAltGraphicsManager::getHeight() const {
	if (_screen)
		return _screen->getHeight();
	else
		return 0;
}

int16 DCAltGraphicsManager::getWidth() const {
	if (_screen)
		return _screen->getWidth();
	else
		return 0;
}

void DCAltGraphicsManager::fillScreen(uint32 col) {
	if (_screen)
		_screen->fill(col);
}

void DCAltGraphicsManager::setShakePos(int shakeXOffset, int shakeYOffset) {
	_shakeXOffset = shakeXOffset;
	_shakeYOffset = shakeYOffset;
}

void DCAltGraphicsManager::updateScreen() {
	int align_x, align_y;

	if (_overlayDirty) {
		_overlay->loadTexture();
		_overlayDirty = false;
	}

	if (_screen && _screenDirty) {
		_screen->loadPalette();
		_screen->loadTexture();
		_screenDirty = false;
	}

	_mouse->load();

	pvr_wait_ready();
	pvr_scene_begin();

	if (_screen) {
		if (_screenFormat == PF_RGB565 || _screenFormat == PF_CLUT8)
			pvr_list_begin(PVR_LIST_OP_POLY);
		else
			pvr_list_begin(PVR_LIST_TR_POLY);

		align_x = (_overlay->getWidth() -
		           _screen->getWidth() * getScaleX()) / 2;
		align_y = (_overlay->getHeight() -
		           _screen->getHeight() * getScaleY()) / 2;

		_screen->draw(align_x + _shakeXOffset,
		              align_y + _shakeYOffset,
		              5.0f,
			      getScaleX(), getScaleY());
		if (_screenFormat == PF_RGB565 || _screenFormat == PF_CLUT8)
			pvr_list_finish();
	}

	if (_screenFormat == PF_RGB565 || _screenFormat == PF_CLUT8)
		pvr_list_begin(PVR_LIST_TR_POLY);
	if (!_overlayHidden)
		_overlay->draw(0, 0, 10.0f,
			       1.0f, 1.0f);

	if (_overlayHidden) {
		int mouse_offset_y = \
		    (_vid_height - _screen->getHeight() * getScaleY()) / 2;
		_mouse->draw(0, mouse_offset_y, getScaleX(), getScaleY(), 1.0f);
	}
	else {
		_mouse->draw(0, 0, 1.0f, 1.0f, 2.0f);
	}

	pvr_list_finish();

	pvr_scene_finish();
}

void DCAltGraphicsManager::clearOverlay() {
	_overlay->clear();

	_overlayDirty = true;
}

void DCAltGraphicsManager::grabOverlay(void *buf, int pitch) const {
	_overlay->grab(buf, pitch);
}

void DCAltGraphicsManager::copyRectToOverlay(
    const void *buf, int pitch, int x, int y, int w, int h) {
	_overlay->copyRect(buf, pitch, x, y, w, h);

	_overlayDirty = true;
}

Graphics::PixelFormat DCAltGraphicsManager::getOverlayFormat() const {
	return Graphics::PixelFormat(2,4,4,4,4,8,4,0,12);
}

int16 DCAltGraphicsManager::getOverlayWidth() const {
	return _vid_width;
}

int16 DCAltGraphicsManager::getOverlayHeight() const {
	return _vid_height;
}

float DCAltGraphicsManager::getScaleX() const {
	if (_screen)
		return _vid_width / _screen->getWidth();
	else
		return 1.0f;
}

int DCAltGraphicsManager::getMouseX() const {
	return _mouse->getX();
}
int DCAltGraphicsManager::getMouseY() const {
	return _mouse->getY();
}

float DCAltGraphicsManager::getScaleY() const {
	float scale_y, ar_scale_y;

	if (_screen)
		scale_y = _vid_height / _screen->getHeight();
	else
		scale_y = 1.0f;

	if (_aspectRatioCorrection && _screen &&
	    !vgaModeAspectRatioCorrection() &&
	    _screen->getWidth() == 320 && _screen->getHeight() == 200)
		ar_scale_y = scale_y * ((float)240 / 200);
	else
		ar_scale_y = scale_y;

	return ar_scale_y;
}

bool DCAltGraphicsManager::showMouse(bool visible) {
	return _mouse->show(visible);
}

void DCAltGraphicsManager::warpMouse(int x, int y) {
	_mouse->warp(x, y);
}

void DCAltGraphicsManager::setMouseCursor(
    const void *buf, uint w, uint h, int hotspotX, int hotspotY,
    uint32 keycolor, bool dontScale, const Graphics::PixelFormat *format) {
	_mouse->setCursor(buf, w, h, hotspotX, hotspotY,
	                  keycolor, dontScale, format);
}

void DCAltGraphicsManager::setCursorPalette(
    const byte *colors, uint start, uint num) {
	_mouse->setCursorPalette(colors, start, num);
}

Graphics::Surface *DCAltGraphicsManager::lockScreen() {
	_surface.init(_screen->getWidth(), _screen->getHeight(),
	              _screen->getStride(), _screen->getPixels(),
		      _screenFormat
	             );

	return &_surface;
}

void DCAltGraphicsManager::unlockScreen() {
	_screenDirty = true;
}

void DCAltGraphicsManager::translateMouse(Common::Event &event, int dx, int dy) {
	int width, height;

	if (_overlayHidden) {
		width =  _screen->getWidth();
		height =  _screen->getHeight();
	}
	else {
		width = _vid_width;
		height = _vid_height;
	}

	event.mouse.x = _mouse->getX() + dx;
	event.mouse.y = _mouse->getY() + dy;

	if (event.mouse.x < 0)
		event.mouse.x = 0;
	else if (event.mouse.x >= width)
		event.mouse.x = width -1 ;
	if (event.mouse.y < 0)
		event.mouse.y = 0;
	else if (event.mouse.y >= height)
		event.mouse.y = height -1 ;

	warpMouse(event.mouse.x, event.mouse.y);
}

bool DCAltGraphicsManager::vgaModeAspectRatioCorrection() const {
	return _vga && ConfMan.getBool("dcalt_vga_mode_aspect_ratio");
}
