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

#ifndef DCALT_GRAPHICS_H
#define DCALT_GRAPHICS_H

#include <stdint.h>

#include <dc/video.h>
#include <dc/pvr.h>

#include "backends/graphics/graphics.h"
#include "graphics/surface.h"
#include "common/events.h"

#include "graphics/colormasks.h"

class PVRSurface {
public:
	virtual ~PVRSurface() {};
	virtual void clear() = 0;
	virtual void grab(void *buf, int pitch) const = 0;
	virtual void copyRect(const void *buf, int pitch,
	                      int x, int y, int w, int h) = 0;
	virtual void loadTexture() = 0;
	virtual void loadPalette() {};
	virtual void draw(int x, int y, float z,
			  float scale_x, float scale_y) = 0;
	virtual void setPalette(const byte *colors, uint start, uint num) {};
	virtual void grabPalette(byte *colors, uint start, uint num) {};
	virtual void fill(uint32 col) = 0;
	int getWidth() const { return _width; }
	int getHeight() const { return _height; }
	int getStride() const { return _stride; }
	void *getPixels() const { return _pixels; }
protected:
	int _width;
	int _height;
	int _stride;
	void *_pixels;
};

class RGBSurface : public PVRSurface {
public:
	RGBSurface(int w, int h, int pixelFormat, int filteringMode);
	~RGBSurface();

	void fill(uint32 col);
	void clear();
	void grab(void *buf, int pitch) const;
	void copyRect(const void *buf, int pitch, int x, int y, int w, int h);
	void loadTexture();
	void draw(int x, int y, float z,
		  float scale_x, float scale_y);
private:
	int _texture_width;
	int _texture_height;
	pvr_ptr_t _texture;
	pvr_poly_cxt_t _cxt;
	pvr_poly_hdr_t _poly;
};

struct vqtile {
        pvr_ptr_t texture;
        pvr_poly_cxt_t cxt;
        pvr_poly_hdr_t poly;
};

// Use VQ texture compression to implement a paletted surface.  Described here:
// http://www.numechanix.com/blog/index.php/2015/10/03/20/
class VQSurface : public PVRSurface {
public:
	VQSurface(int w, int h, int format, int filteringMode);
	~VQSurface();
	void fill(uint32 col);
        void clear();
        void grab(void *buf, int pitch) const;
        void copyRect(const void *buf, int pitch, int x, int y, int w, int h);
        void loadTexture();
        void loadPalette();
	void draw(int x, int y, float z,
		  float scale_x, float scale_y);
        void setPalette(const byte *colors, uint start, uint num);
	void grabPalette(byte *colors, uint start, uint num);

private:
	uint16_t _palette[1024];
	int _texture_height;
	int _last_tile_texture_width;
	int _last_tile_stride;
	int _last_tile_width;
	// Number of tiles (minus the _last_tile)
	int _tiles_count;
	struct vqtile *_tiles;
	struct vqtile _last_tile;
};

class Mouse {
public:
	Mouse();
	~Mouse();
	bool show(bool visible);
	void warp(int x, int y);
	void load();
	void draw(int, int, float, float, float);
	void setCursor(const void *buf, uint w, uint h, int hotspotX, int hotspotY, uint32 keycolor, bool dontScale = false, const Graphics::PixelFormat *format = NULL);
	void setCursorPalette(const byte *colors, uint start, uint num);
	void setScreenPalette(const byte *colors, uint start, uint num);
	void setCursorPaletteDisabled(bool disable);
	bool getCursorPaletteDisabled() const;

	int getX() const { return _x; };
	int getY() const { return _y; };
private:
	void changeFormat(uint w, uint h, const Graphics::PixelFormat format);
	bool _visible;
	int _x;
	int _y;
	int _hotspotX;
	int _hotspotY;
	int _w;
	int _h;
	int _texture_w;
	int _texture_h;
	bool _dontScale;
	Graphics::PixelFormat _format;
	bool _cursorDirty;
	bool _paletteDirty;
	uint16_t *_palette;
	uint16_t *_screenPalette;
	uint32 _keycolor;
	void *_pixels;
	pvr_ptr_t _texture;
	pvr_poly_cxt_t _cxt;
	pvr_poly_hdr_t _poly;
	bool _cursorPaletteDisabled;
};

class DCAltGraphicsManager : public GraphicsManager {
public:
	DCAltGraphicsManager();
	virtual ~DCAltGraphicsManager();

	bool hasFeature(OSystem::Feature f) const;
	void setFeatureState(OSystem::Feature f, bool enable);
	bool getFeatureState(OSystem::Feature f) const;

#ifdef USE_RGB_COLOR
	Graphics::PixelFormat getScreenFormat() const override;
	Common::List<Graphics::PixelFormat> getSupportedFormats() const override;
#endif
	void initSize(uint width, uint height, const Graphics::PixelFormat *format = NULL);
	virtual	int getScreenChangeID() const override { return _screenChangeCount; }

	void beginGFXTransaction() override {}
	OSystem::TransactionError endGFXTransaction() override { return OSystem::kTransactionSuccess; }

	int16 getHeight() const;
	int16 getWidth() const;
	void setPalette(const byte *colors, uint start, uint num);
	void grabPalette(byte *colors, uint start, uint num) const;
	void copyRectToScreen(const void *buf, int pitch, int x, int y, int w, int h);
	Graphics::Surface *lockScreen();
	void unlockScreen();
	void updateScreen();
	void fillScreen(uint32 col);
	void setShakePos(int shakeXOffset, int shakeYOffset);
	void setFocusRectangle(const Common::Rect& rect) override {}
	void clearFocusRectangle() override {}

	void showOverlay();
	void hideOverlay();
	Graphics::PixelFormat getOverlayFormat() const;
	void clearOverlay();
	void grabOverlay(void *buf, int pitch) const;
	void copyRectToOverlay(const void *buf, int pitch, int x, int y, int w, int h);
	int16 getOverlayHeight() const;
	int16 getOverlayWidth() const;
	bool getOverlayHidden() { return _overlayHidden; };

	float getScaleX() const;
	float getScaleY() const;

	bool showMouse(bool visible);
	void warpMouse(int x, int y);
	void setMouseCursor(const void *buf, uint w, uint h, int hotspotX, int hotspotY, uint32 keycolor, bool dontScale = false, const Graphics::PixelFormat *format = NULL);
	void setCursorPalette(const byte *colors, uint start, uint num);

	void translateMouse(Common::Event &event, int dx, int dy);
private:
	bool vgaModeAspectRatioCorrection();
	void initOverlay(int width, int height);
	void initVideo640x480();
	void initVideo640x400();
	void initVideo320x240();
	bool _vga;
	int _screenChangeCount;
	int _activeDomain;
	bool _overlayHidden;
	bool _overlayDirty;
	bool _screenDirty;
	int _vid_width;
	int _vid_height;
	int _shakeXOffset;
	int _shakeYOffset;
	PVRSurface *_overlay;
	PVRSurface *_screen;
	Mouse *_mouse;
	Graphics::Surface _surface;
	bool _aspectRatioCorrection;
	Graphics::PixelFormat _screenFormat;
	int _filteringMode;
};

#endif
