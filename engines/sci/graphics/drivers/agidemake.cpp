/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common/config-manager.h"
#include "common/system.h"
#include "sci/sci.h"
#include "sci/graphics/drivers/gfxdriver_intern.h"

#include "sci/engine/state.h"

namespace Sci {

// Output-only filter: game logic, priority and control screens stay at 320x200.
// Each horizontal pixel pair is collapsed to one "fat" pixel (AGI style), and
// undithered colour pairs (0x10-0xFE) are snapped to the nearest single EGA colour.
class SCI0_AGIDemakeDriver final : public GfxDefaultDriver {
public:
	SCI0_AGIDemakeDriver(uint16 screenWidth, uint16 screenHeight, bool rgbRendering);
	~SCI0_AGIDemakeDriver() override;
	void copyRectToScreen(const byte *src, int srcX, int srcY, int pitch, int destX, int destY, int w, int h, const PaletteMod *palMods, const byte *palModMapping) override;
	bool agiDemake() const override { return true; }
private:
	byte _lut[256];
	byte *_demakeBuffer;
	bool _chunkyDither;	// agi_dither = 1 (Chunky dither) rather than 0 (Flat approximation)
};

SCI0_AGIDemakeDriver::SCI0_AGIDemakeDriver(uint16 screenWidth, uint16 screenHeight, bool rgbRendering) :
	GfxDefaultDriver(screenWidth, screenHeight, true, rgbRendering), _demakeBuffer(nullptr),
	_chunkyDither(ConfMan.hasKey("agi_dither") && ConfMan.getInt("agi_dither") == 1) {
	static const byte ega[16][3] = {
		{ 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0xAA }, { 0x00, 0xAA, 0x00 }, { 0x00, 0xAA, 0xAA },
		{ 0xAA, 0x00, 0x00 }, { 0xAA, 0x00, 0xAA }, { 0xAA, 0x55, 0x00 }, { 0xAA, 0xAA, 0xAA },
		{ 0x55, 0x55, 0x55 }, { 0x55, 0x55, 0xFF }, { 0x55, 0xFF, 0x55 }, { 0x55, 0xFF, 0xFF },
		{ 0xFF, 0x55, 0x55 }, { 0xFF, 0x55, 0xFF }, { 0xFF, 0xFF, 0x55 }, { 0xFF, 0xFF, 0xFF }
	};

	for (int i = 0; i < 256; ++i) {
		if (i < 16 || i == 0xFF) {
			_lut[i] = i & 0x0F;
			continue;
		}
		int c1 = i & 0x0F;
		int c2 = i >> 4;
		int r = (ega[c1][0] + ega[c2][0]) / 2;
		int g = (ega[c1][1] + ega[c2][1]) / 2;
		int b = (ega[c1][2] + ega[c2][2]) / 2;
		int best = 0;
		int bestDist = 0x7FFFFFFF;
		for (int c = 0; c < 16; ++c) {
			int dr = r - ega[c][0];
			int dg = g - ega[c][1];
			int db = b - ega[c][2];
			int dist = dr * dr + dg * dg + db * db;
			if (dist < bestDist) {
				bestDist = dist;
				best = c;
			}
		}
		_lut[i] = best;
	}

	_demakeBuffer = new byte[screenWidth * screenHeight]();
}

SCI0_AGIDemakeDriver::~SCI0_AGIDemakeDriver() {
	delete[] _demakeBuffer;
}

void SCI0_AGIDemakeDriver::copyRectToScreen(const byte *src, int srcX, int srcY, int pitch, int destX, int destY, int w, int h, const PaletteMod *palMods, const byte *palModMapping) {
	GFXDRV_ASSERT_READY;

	// Align to pixel pairs so every fat pixel is rebuilt from its left source pixel
	int diff = destX & 1;
	srcX -= diff;
	destX -= diff;
	w += diff;
	w = (w + 1) & ~1;
	if (destX + w > _screenW)
		w = _screenW - destX;

	// Per-room colour for an undithered pair, where the nearest single colour reads wrongly. PQ2
	// motel room (room 26): the floor is dithered black and brown, which snaps to black. It is shown
	// dark grey instead, which also keeps it apart from the brown bed
	static const uint16 kAnyRoom = 0xFFFF;
	// QfG1 (every room, kAnyRoom): black dithered with brown is everywhere (tree trunks, wood, earth)
	// and also snaps to black, so it is shown dark grey too. Dithers with black are always stored
	// with black in the low nibble (0x60), so this never touches solid brown (6)
	static const struct { SciGameId game; uint16 room; byte pair; byte color; } kRoomPairColors[] = {
		{ GID_PQ2, 26, 0x60, 8 },
		{ GID_QFG1, kAnyRoom, 0x60, 8 },
		// QfG1 forest (every room): light green with green shows light green (was green), cyan with
		// black shows cyan (was black), green with dark grey shows dark grey (was green)
		{ GID_QFG1, kAnyRoom, 0x2A, 10 },
		{ GID_QFG1, kAnyRoom, 0xA2, 10 },
		{ GID_QFG1, kAnyRoom, 0x30, 3 },
		{ GID_QFG1, kAnyRoom, 0x28, 8 },
		{ GID_QFG1, kAnyRoom, 0x82, 8 },
		// QfG1: brown with green is dark foliage (bushes, leaves in front of the path), shown green
		// (it was brown)
		{ GID_QFG1, kAnyRoom, 0x26, 2 },
		{ GID_QFG1, kAnyRoom, 0x62, 2 },
	};
	const byte *lut = _lut;
	byte roomLut[256];
	if (g_sci && g_sci->getEngineState()) {
		const uint16 room = g_sci->getEngineState()->currentRoomNumber();
		for (uint i = 0; i < ARRAYSIZE(kRoomPairColors); ++i) {
			if (kRoomPairColors[i].game != g_sci->getGameId() || (kRoomPairColors[i].room != room && kRoomPairColors[i].room != kAnyRoom))
				continue;
			if (lut == _lut) {
				memcpy(roomLut, _lut, sizeof(roomLut));
				lut = roomLut;
			}
			roomLut[kRoomPairColors[i].pair] = kRoomPairColors[i].color;
		}
	}

	// Dither handler (agi_dither, a pulldown under the AGI demake option, read at start-up):
	// 0 Flat approximation (default): each dithered pair is snapped to the nearest single colour,
	//   with the per-room colours above
	// 1 Chunky dither: each dithered pair in the room pictures is drawn as a checkerboard of fat
	//   pixels (one colour, then the other, along each row and down each column). Pairs are any
	//   picture value of 0x10 or more (0xFF is plain white). The pattern is fixed to the screen, so
	//   it doesn't crawl when part of the screen is redrawn
	const bool fatChecker = _chunkyDither;
	auto px = [&](byte code, int col, int row) -> byte {
		if (fatChecker && code >= 0x10 && code != 0xFF)
			return (((col >> 1) + row) & 1) ? (code >> 4) : (code & 0x0F);
		return lut[code];
	};

	// palModMapping is the full-screen map buffer: 1 marks text, 2 marks window frames
	const byte *s = src + srcY * pitch + srcX;
	const byte *m = palModMapping ? palModMapping + destY * _screenW + destX : nullptr;
	byte *d = _demakeBuffer;
	for (int y = 0; y < h; ++y) {
		const int row = destY + y;
		for (int x = 0; x < w; x += 2) {
			bool hasRight = (x + 1 < w);
			const int scol = destX + x;
			byte fl = m ? m[x] : 0;
			byte fr = (m && hasRight) ? m[x + 1] : 0;
			if (fl == 1 || fr == 1) {
				d[x] = px(s[x], scol, row);
				if (hasRight)
					d[x + 1] = px(s[x + 1], scol + 1, row);
			} else if (fr == 2 && fl != 2) {
				byte c = lut[s[x + 1]];
				d[x] = c;
				d[x + 1] = c;
			} else if ((fl == 3 && fr == 0) || (fr == 3 && fl == 0)) {
				// half of a sprite's fat pixel next to background (the sprite's edge cut it on screen):
				// the sprite pixel takes the whole pair
				byte c = lut[(fl == 3) ? s[x] : s[x + 1]];
				d[x] = c;
				if (hasRight)
					d[x + 1] = c;
			} else {
				byte c = px(s[x], scol, row);
				// A 1 pixel vertical line in the right half of the pair (left pixel matches its
				// left neighbour, right pixel differs from its right neighbour) wins the pair, so
				// thin picture lines on odd columns are not lost. Neighbours are only read from a
				// full-width screen buffer, using absolute columns, so the result never depends
				// on which rectangle is being updated.
				if (hasRight && pitch == _screenW) {
					const int col = srcX + x;
					const byte b = px(s[x + 1], scol + 1, row);
					if (b != c && col - 1 >= 0 && col + 2 < _screenW) {
						const byte leftN = px(s[x - 1], scol - 1, row);
						const byte rightN = px(s[x + 2], scol + 2, row);
						if (c == leftN && b != rightN)
							c = b;
					}
				}
				d[x] = c;
				if (hasRight)
					d[x + 1] = c;
			}
		}
		s += pitch;
		if (m)
			m += _screenW;
		d += w;
	}

	GfxDefaultDriver::copyRectToScreen(_demakeBuffer, 0, 0, w, destX, destY, w, h, nullptr, nullptr);
}

GfxDriver *SCI0_AGIDemakeDriver_create(int rgbRendering, ...) {
	va_list args;
	va_start(args, rgbRendering);
	va_arg(args, int);
	int width = va_arg(args, int);
	int height = va_arg(args, int);
	va_end(args);

	return new SCI0_AGIDemakeDriver(width, height, rgbRendering != 0);
}

} // End of namespace Sci
