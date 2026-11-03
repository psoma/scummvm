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

#include "sci/sci.h"
#include "sci/engine/features.h"
#include "sci/engine/state.h"
#include "sci/engine/selector.h"
#include "sci/engine/workarounds.h"
#include "sci/graphics/cache.h"
#include "sci/graphics/coordadjuster.h"
#include "sci/graphics/ports.h"
#include "sci/graphics/paint16.h"
#include "sci/engine/agiinput.h"
#include "sci/graphics/animate.h"
#include "sci/graphics/scifont.h"
#include "sci/graphics/picture.h"
#include "sci/graphics/view.h"
#include "sci/graphics/screen.h"
#include "sci/graphics/drivers/gfxdriver.h"
#include "sci/graphics/palette16.h"
#include "sci/graphics/portrait.h"
#include "sci/graphics/text16.h"
#include "sci/graphics/transitions.h"

#include "sci/graphics/scifx.h"
#include "common/config-manager.h"
#include "graphics/fonts/dosfont.h"
#include "common/file.h"
#include "common/hashmap.h"

namespace Sci {

GfxPaint16::GfxPaint16(ResourceManager *resMan, SegManager *segMan, GfxCache *cache, GfxPorts *ports, GfxCoordAdjuster16 *coordAdjuster, GfxScreen *screen, GfxPalette *palette, GfxTransitions *transitions, AudioPlayer *audio)
	: _resMan(resMan), _segMan(segMan), _cache(cache), _ports(ports),
	  _coordAdjuster(coordAdjuster), _screen(screen), _palette(palette),
	  _transitions(transitions), _audio(audio), _EGAdrawingVisualize(false),
	  _hiresDrawObjs(nullptr), _hiresPortraitWorkaroundFlag(false) {

	// _animate and _text16 will be initialized later on
	_animate = nullptr;
	_text16 = nullptr;
}

// The original KQ6WinCD interpreter saves the hires drawing information in a linked list. This is used to redraw the hires cels after disposing a window.
struct HiresDrawData {
	HiresDrawData(HiresDrawData *chain, reg_t hiresHandle, GuiResourceId id, int16 loop, int16 cel, uint16 left, uint16 top, uint16 pal, byte priority, bool needsWorkaround)
		: handle(hiresHandle), viewId(id), lpNo(loop), celNo(cel), leftPos(left), topPos(top), palNo(pal), prio(priority), waFlag(needsWorkaround), prev(nullptr), next(chain) {
		if (chain)
			chain->prev = this;
	}
	reg_t handle;
	GuiResourceId viewId;
	int16 lpNo, celNo;
	uint16 leftPos, topPos, palNo;
	byte prio;
	bool waFlag;
	HiresDrawData *prev, *next;
};

GfxPaint16::~GfxPaint16() {
	while (_hiresDrawObjs) {
		HiresDrawData *next = _hiresDrawObjs->next;
		delete _hiresDrawObjs;
		_hiresDrawObjs = next;
	}
}

void GfxPaint16::init(GfxAnimate *animate, GfxText16 *text16) {
	_animate = animate;
	_text16 = text16;
}

void GfxPaint16::debugSetEGAdrawingVisualize(bool state) {
	_EGAdrawingVisualize = state;
}

void GfxPaint16::agiDemakeDumpPicture(GuiResourceId pictureId, bool addToFlag) {
	static Common::DumpFile *file = nullptr;
	static Common::HashMap<int, bool> seen;
	const int key = pictureId * 2 + (addToFlag ? 1 : 0);
	if (seen.contains(key) || _screen->getUpscaledHires() != GFX_SCREEN_UPSCALED_DISABLED)
		return;
	seen[key] = true;
	if (!file) {
		file = new Common::DumpFile();
		if (!file->open(Common::Path("agi_demake_pic.txt"))) {
			delete file;
			file = nullptr;
			return;
		}
	}
	const int w = _screen->getDisplayWidth(), h = _screen->getDisplayHeight();
	const int room = g_sci->getEngineState() ? g_sci->getEngineState()->currentRoomNumber() : -1;
	file->writeString(Common::String::format("pic %d room %d addTo %d w %d h %d\n", pictureId, room, addToFlag ? 1 : 0, w, h));
	const byte *d = _screen->agiDemakeDisplayScreen();
	const char *hex = "0123456789abcdef";
	for (int y = 0; y < h; ++y) {
		Common::String line;
		for (int x = 0; x < w; ++x) {
			const byte c = d[y * w + x];
			line += hex[c >> 4];
			line += hex[c & 15];
		}
		line += '\n';
		file->writeString(line);
	}
	// priority screen (what is in front of whom), one hex digit per pixel, then the priority band per row
	const byte *pr = _screen->agiDemakePriorityScreen();
	file->writeString(Common::String::format("priority %d room %d w %d h %d\n", pictureId, room, _screen->getWidth(), _screen->getHeight()));
	for (int y = 0; y < _screen->getHeight(); ++y) {
		Common::String line;
		for (int x = 0; x < _screen->getWidth(); ++x)
			line += hex[pr[y * _screen->getWidth() + x] & 15];
		line += '\n';
		file->writeString(line);
	}
	{
		Common::String line = "bands";
		const int portTop = _ports->_picWind ? _ports->_picWind->top : 0;
		for (int y = 0; y < _screen->getHeight(); ++y)
			line += Common::String::format(" %d", y < portTop ? 0 : _ports->kernelCoordinateToPriority(y - portTop));
		line += '\n';
		file->writeString(line);
	}
	// control screen (which areas are walkable), one hex digit per pixel
	const int cw = _screen->getWidth(), ch = _screen->getHeight();
	const byte *c = _screen->agiDemakeControlScreen();
	file->writeString(Common::String::format("control %d room %d w %d h %d\n", pictureId, room, cw, ch));
	for (int y = 0; y < ch; ++y) {
		Common::String line;
		for (int x = 0; x < cw; ++x)
			line += hex[c[y * cw + x] & 15];
		line += '\n';
		file->writeString(line);
	}
	file->flush();
}

void GfxPaint16::drawPicture(GuiResourceId pictureId, bool mirroredFlag, bool addToFlag, GuiResourceId paletteId) {
	agiInputPictureDrawn();	// (AGI demake: keys are ignored until a room's first picture is up)
	_screen->_agiPicGeneration++;
	// Set up custom per-picture palette mod
	doCustomPicPalette(_screen, pictureId);

	// do we add to a picture? if not -> clear screen with white
	if (!addToFlag)
		clearScreen(_screen->getColorWhite());

	// AGI demake, QfG1 town at night (drawn with a night palette): see agiDemakeRecolourWalkable
	_screen->_agiNightTown = _screen->agiDemake() && g_sci->getGameId() == GID_QFG1 && paletteId != 0 && !addToFlag &&
		(pictureId == 300 || pictureId == 310 || pictureId == 320 || pictureId == 330 || pictureId == 333);

	// Draw the picture
	GfxPicture picture(_resMan, _coordAdjuster, _ports, _screen, _palette, pictureId, _EGAdrawingVisualize);
	picture.draw(mirroredFlag, addToFlag, paletteId);


	// AGI demake debug capture (only with agi_demake_debug=true): each picture once, as the colour
	// pairs it was drawn with plus its control screen, into agi_demake_pic.txt next to the exe
	if (_screen->agiDemake() && ConfMan.hasKey("agi_demake_debug") && ConfMan.getBool("agi_demake_debug"))
		agiDemakeDumpPicture(pictureId, addToFlag);

	if (_screen->agiDemake() && g_sci->getGameId() == GID_QFG1) {
		// stray picture pixels, in screen coordinates, given their neighbours' colour pair
		static const struct { int16 pic, x, y, w; byte code; } kStray[] = {
			// town, Sheriff's office steps (picture 300): red at both ends of the bottom step and on the
			// step corner above the right end, and two in the dirt in front
			{ 300, 144, 169, 2, 0x68 }, { 300, 198, 169, 4, 0x68 }, { 300, 200, 163, 2, 0x07 },
			{ 300, 210, 190, 1, 0x76 }, { 300, 211, 189, 1, 0x76 }
		};
		for (uint i = 0; i < ARRAYSIZE(kStray); ++i)
			if (kStray[i].pic == pictureId && !addToFlag)
				for (int d = 0; d < kStray[i].w; ++d)
					_screen->agiDemakeSetPictureCode(kStray[i].x + d, kStray[i].y, kStray[i].code);
		// pictures painted over in fat pixels (each character is 2 screen pixels, a hex colour, '.' left
		// alone). Guild hall (picture 311): the "QUESTS" plate above the notice board is too narrow for
		// six readable letters, so a wider plate with them is painted on the wall here, and the plate
		// sprite itself (view 311 loop 2 cel 0) is left out
		static const struct { int16 pic, x, y; const char *rows[8]; } kArt[] = {
			{ 311, 108, 76, {
				"000000000000000000000000000",
				"ccccccccccccccccccccccccc40",
				"c66666666666666666666666640",
				"c66e66e6e6eee6eee6eee6eee40",
				"c6e6e6e6e6e666e6666e66e6640",
				"c6e6e6e6e6ee66eee66e66eee40",
				"c6ee66e6e6e66666e66e6666e40",
				"c66ee6eee6eee6eee66e66eee6." } }
		};
		for (uint i = 0; i < ARRAYSIZE(kArt); ++i) {
			if (kArt[i].pic != pictureId || addToFlag)
				continue;
			for (int r = 0; r < 8 && kArt[i].rows[r]; ++r)
				for (int c = 0; kArt[i].rows[r][c]; ++c) {
					const char ch = kArt[i].rows[r][c];
					if (ch == '.')
						continue;
					const byte code = (ch >= 'a') ? (ch - 'a' + 10) : (ch - '0');
					for (int d = 0; d < 2; ++d)
						_screen->agiDemakeSetPictureCode(kArt[i].x + 2 * c + d, kArt[i].y + r, code);
				}
		}
		// picture areas moved (screen pixels): guild hall (picture 311), the notice board with its
		// "QUESTS" plate goes 2 pixels down and 4 left. What it uncovers becomes the wall (light
		// grey dithered with dark grey)
		static const struct { int16 pic, left, top, right, bottom, dx, dy; byte fill; } kMove[] = {
			{ 311, 104, 76, 168, 104, -4, 2, 0x78 }
		};
		for (uint i = 0; i < ARRAYSIZE(kMove); ++i) {
			if (kMove[i].pic != pictureId || addToFlag)
				continue;
			const int w = kMove[i].right - kMove[i].left, h = kMove[i].bottom - kMove[i].top;
			Common::Array<byte> area(w * h);
			for (int y = 0; y < h; ++y)
				for (int x = 0; x < w; ++x) {
					area[y * w + x] = _screen->agiDemakeGetPictureCode(kMove[i].left + x, kMove[i].top + y);
					_screen->agiDemakeSetPictureCode(kMove[i].left + x, kMove[i].top + y, kMove[i].fill);
				}
			for (int y = 0; y < h; ++y)
				for (int x = 0; x < w; ++x)
					_screen->agiDemakeSetPictureCode(kMove[i].left + x + kMove[i].dx, kMove[i].top + y + kMove[i].dy, area[y * w + x]);
		}
		// Flat approximation (agi_dither 0) only: walkable ground gets its own dither colours
		if (!(ConfMan.hasKey("agi_dither") && ConfMan.getInt("agi_dither") == 1)) {
			// priority band per screen row (the picture port starts _picWind->top rows down)
			Common::Array<byte> band(_screen->getHeight(), 0);
			const int portTop = _ports->_picWind ? _ports->_picWind->top : 0;
			for (int y = portTop; y < (int)band.size(); ++y)
				band[y] = _ports->kernelCoordinateToPriority(y - portTop);
			_screen->agiDemakeRecolourWalkable(band.data(), pictureId);
		}
		// keep a copy of the room picture for sprite fixes that put background back
		_screen->agiDemakeSnapshotPicture();
	}

	// AGI demake, SQ3 ship's computer (picture 17): the panel headings "FLIGHT MODE" and "DEFENSE"
	// are painted in 5 pixel letters. They are wiped (with the frame line either side, to make room)
	// and written in the AGI font, tight spacing, at full resolution and flagged as text. Where the
	// frame line starts at a corner on the odd pixel of a pair, that corner is filled in so the corner
	// stays square (middle and right panels' top left corners)
	if (_screen->agiDemake() && g_sci->getGameId() == GID_SQ3 && pictureId == 17 && !addToFlag) {
		static const struct { const char *word; int16 x, y, wipeL, wipeR; } kHead[] = {
			{ "FLIGHT MODE", 118, 24, 114, 199 },
			{ "DEFENSE", 234, 24, 230, 291 }
		};
		for (uint i = 0; i < ARRAYSIZE(kHead); ++i) {
			for (int y = kHead[i].y; y < kHead[i].y + 7; ++y)
				for (int x = kHead[i].wipeL; x <= kHead[i].wipeR; ++x) {
					const byte c = _screen->agiDemakeGetPictureCode(x, y);
					if (c == 10 || (y == 27 && c == 2))
						_screen->agiDemakeSetPictureCode(x, y, 0);
				}
			int xPos = kHead[i].x;
			for (const char *ch = kHead[i].word; *ch; ++ch) {
				const uint8 *g = Graphics::DosFont::fontData_PCBIOS + (byte)*ch * 8;
				int a = 8, b = -1;
				for (int gx = 0; gx < 8; ++gx)
					for (int gy = 0; gy < 8; ++gy)
						if (g[gy] & (0x80 >> gx)) {
							a = MIN(a, gx);
							b = MAX(b, gx);
						}
				if (b < 0) {	// space
					a = 0;
					b = 2;
				}
				for (int gy = 0; gy < 7; ++gy)
					for (int gx = a; gx <= b; ++gx)
						if (g[gy] & (0x80 >> gx)) {
							const int x = xPos + gx - a, y = kHead[i].y + gy;
							_screen->agiDemakeSetPictureCode(x, y, 10);
							_screen->setDemakeFlag(x, y, 1);
						}
				xPos += b - a + 2;
			}
		}
		static const int16 kCorner[] = { 94, 226 };
		for (uint i = 0; i < ARRAYSIZE(kCorner); ++i)
			for (int d = 0; d < 2; ++d)
				_screen->agiDemakeSetPictureCode(kCorner[i] + d, 27, 2);
	}

	// AGI demake, SQ3 weapons screen (picture 18): Sierra's 5 pixel lettering redone in the AGI font
	// (tight spacing, full resolution, flagged as text). CONTROLS and SHIELDS move off the panel tops
	// (whose frame lines are joined up) and are stacked letter by letter in the black margins at the
	// screen's sides. RADAR sits one row higher. The key letters H, O, F, B are redrawn in their boxes.
	// SPACEBAR goes one row lower, the red bracket's sides end level with the FIRE box, and the blue
	// line under it steps down around the word, 1 pixel clear
	if (_screen->agiDemake() && g_sci->getGameId() == GID_SQ3 && pictureId == 18 && !addToFlag) {
		auto colourAt = [&](int x, int y) { return _screen->agiDemakeGetPictureCode(x, y) & 15; };
		auto setFat = [&](int p, int y, byte c) {
			_screen->agiDemakeSetPictureCode(2 * p, y, c);
			_screen->agiDemakeSetPictureCode(2 * p + 1, y, c);
		};
		auto wipe = [&](int x0, int x1, int y0, int y1, const byte *cols, int nCols) {
			for (int y = y0; y <= y1; ++y)
				for (int x = x0; x <= x1; ++x)
					for (int i = 0; i < nCols; ++i)
						if (colourAt(x, y) == cols[i]) {
							_screen->agiDemakeSetPictureCode(x, y, 0);
							break;
						}
		};
		auto glyphCols = [](byte ch, int &a, int &b) {
			const uint8 *g = Graphics::DosFont::fontData_PCBIOS + ch * 8;
			a = 8;
			b = -1;
			for (int gx = 0; gx < 8; ++gx)
				for (int gy = 0; gy < 8; ++gy)
					if (g[gy] & (0x80 >> gx)) {
						a = MIN(a, gx);
						b = MAX(b, gx);
					}
			if (b < 0) {
				a = 0;
				b = 2;
			}
		};
		auto wordWidth = [&](const char *w) {
			int n = 0;
			for (; *w; ++w) {
				int a, b;
				glyphCols((byte)*w, a, b);
				n += b - a + 2;
			}
			return n - 1;
		};
		auto text = [&](const char *word, int x, int y) {
			for (const char *ch = word; *ch; ++ch) {
				const uint8 *g = Graphics::DosFont::fontData_PCBIOS + (byte)*ch * 8;
				int a, b;
				glyphCols((byte)*ch, a, b);
				for (int gy = 0; gy < 7; ++gy)
					for (int gx = a; gx <= b; ++gx)
						if (g[gy] & (0x80 >> gx)) {
							_screen->agiDemakeSetPictureCode(x + gx - a, y + gy, 10);
							_screen->setDemakeFlag(x + gx - a, y + gy, 1);
						}
				x += b - a + 2;
			}
		};
		static const byte kGreen[] = { 10 };
		static const byte kGreenRed[] = { 10, 4 };
		static const byte kSpacebar[] = { 10, 4, 1, 9 };
		// panel tops: Sierra's headings wiped, frame line (row 49) joined up
		static const int16 kPanel[][2] = { { 31, 90 }, { 229, 288 } };
		for (int i = 0; i < 2; ++i) {
			wipe(kPanel[i][0] + 1, kPanel[i][1] - 1, 46, 52, kGreen, 1);
			for (int x = kPanel[i][0]; x <= kPanel[i][1]; ++x)
				_screen->agiDemakeSetPictureCode(x, 49, 4);
		}
		// stacked headings, each letter centred on the margin's middle column
		static const struct { const char *word; float cx; int top; } kStack[] = {
			{ "CONTROLS", 10.5f, 69 }, { "SHIELDS", 310.5f, 74 }
		};
		for (uint i = 0; i < ARRAYSIZE(kStack); ++i)
			for (int n = 0; kStack[i].word[n]; ++n) {
				const char letter[2] = { kStack[i].word[n], 0 };
				text(letter, (int)(kStack[i].cx - wordWidth(letter) / 2.0f + 0.5f), kStack[i].top + 8 * n);
			}
		// RADAR, rows 105-111, on its frame line (row 109)
		wipe(40, 82, 105, 111, kGreenRed, 2);
		text("RADAR", 43, 105);
		// key letters in their 7 pixel boxes
		static const struct { const char *word; int16 x, y; } kKey[] = {
			{ "H", 37, 57 }, { "O", 37, 85 }, { "F", 235, 57 }, { "B", 235, 85 }
		};
		for (uint i = 0; i < ARRAYSIZE(kKey); ++i) {
			wipe(kKey[i].x, kKey[i].x + 6, kKey[i].y - 1, kKey[i].y + 7, kGreen, 1);
			text(kKey[i].word, kKey[i].x + (7 - wordWidth(kKey[i].word)) / 2, kKey[i].y);
		}
		// SPACEBAR (x 129-188, rows 156-162) and the blue line around it
		wipe(128, 189, 156, 162, kSpacebar, 4);
		const int stepL = 63, stepR = 95;	// fat columns, 1 pixel clear of the word
		for (int p = stepL; p <= stepR; ++p) {
			setFat(p, 160, 0);
			setFat(p, 161, 0);
			setFat(p, 162, 0);
			setFat(p, 164, 9);
		}
		for (int y = 162; y <= 164; ++y) {
			setFat(stepL, y, 9);
			setFat(stepR, y, 9);
		}
		for (int y = 155; y <= 159; ++y)
			for (int x = 128; x <= 191; ++x)
				if (colourAt(x, y) == 4)
					_screen->agiDemakeSetPictureCode(x, y, 0);
		text("SPACEBAR", 129, 156);
	}

	// AGI demake, SQ3 weapons screen (picture 18): the radar box's blue sides are 1 pixel wide (x 33 and
	// 87) with black beside them, so black won their fat pixel and the sides only showed while a text
	// box over them drew them at full size. Their fat pixels are made blue
	// (and the same for the inner boxes of the controls and shields panels: x 46 and 87, 244 and 285)
	if (_screen->agiDemake() && g_sci->getGameId() == GID_SQ3 && pictureId == 18 && !addToFlag) {
		static const int16 kPairs[3][2] = { { 32, 86 }, { 46, 86 }, { 244, 284 } };	// even column of each side's pair
		static const int16 kRows[3][2] = { { 100, 160 }, { 50, 100 }, { 50, 100 } };
		for (int b = 0; b < 3; ++b)
			for (int y = kRows[b][0]; y < kRows[b][1]; ++y)
				for (int side = 0; side < 2; ++side) {
					const int x = kPairs[b][side];
					const byte l = _screen->agiDemakeGetPictureCode(x, y) & 15, r = _screen->agiDemakeGetPictureCode(x + 1, y) & 15;
					if (l == 9 && r == 0)
						_screen->agiDemakeSetPictureCode(x + 1, y, 9);
					else if (r == 9 && l == 0)
						_screen->agiDemakeSetPictureCode(x, y, 9);
				}
	}

	// AGI demake, SQ3 robot fight (picture 120): POWER LEVEL, GREY and RED are in Sierra's tiny font in
	// the picture. The picture is cleared behind them and they are redrawn in a compact fat pixel font,
	// clear of the status bar, in their own colours (blue, grey, light red)
	if (_screen->agiDemake() && g_sci->getGameId() == GID_SQ3 && pictureId == 120 && !addToFlag) {
		static const struct { int16 x0, x1, y0, y1; } kClear[] = { { 112, 199, 9, 16 }, { 114, 146, 18, 25 }, { 166, 194, 18, 25 } };
		for (uint i = 0; i < ARRAYSIZE(kClear); ++i)
			for (int y = kClear[i].y0; y < kClear[i].y1; ++y)
				for (int x = kClear[i].x0; x < kClear[i].x1; ++x)
					_screen->agiDemakeSetPictureCode(x, y, 0);
		// a compact 3 x 5 font on the AGI fat pixel grid, 1 fat pixel between letters
		static const struct { char ch; const char *rows[5]; } kGlyphs[] = {
			{ 'D', { "11.", "1.1", "1.1", "1.1", "11." } },
			{ 'E', { "111", "1..", "11.", "1..", "111" } },
			{ 'G', { "111", "1..", "1.1", "1.1", "111" } },
			{ 'L', { "1..", "1..", "1..", "1..", "111" } },
			{ 'O', { "111", "1.1", "1.1", "1.1", "111" } },
			{ 'P', { "111", "1.1", "111", "1..", "1.." } },
			{ 'R', { "11.", "1.1", "11.", "1.1", "1.1" } },
			{ 'V', { "1.1", "1.1", "1.1", "1.1", ".1." } },
			{ 'W', { "1.1", "1.1", "111", "111", "1.1" } },
			{ 'Y', { "1.1", "1.1", ".1.", ".1.", ".1." } },
		};
		static const struct { const char *text; int16 x, y; byte colour; } kLabels[] = {
			{ "POWER LEVEL", 114, 11, 9 }, { "GREY", 116, 19, 7 }, { "RED", 168, 19, 12 }
		};
		for (uint i = 0; i < ARRAYSIZE(kLabels); ++i) {
			int16 x = kLabels[i].x;
			for (const char *p = kLabels[i].text; *p; ++p) {
				if (*p == ' ') {
					x += 4;
					continue;
				}
				for (uint g = 0; g < ARRAYSIZE(kGlyphs); ++g) {
					if (kGlyphs[g].ch != *p)
						continue;
					for (int r = 0; r < 5; ++r)
						for (int c = 0; kGlyphs[g].rows[r][c]; ++c)
							if (kGlyphs[g].rows[r][c] == '1') {
								_screen->agiDemakeSetPictureCode(x + 2 * c, kLabels[i].y + r, kLabels[i].colour);
								_screen->agiDemakeSetPictureCode(x + 2 * c + 1, kLabels[i].y + r, kLabels[i].colour);
							}
					x += 2 * (int16)strlen(kGlyphs[g].rows[0]) + 2;
				}
			}
		}
	}

	// AGI demake, SQ3 Astro Chicken's cabinet (pictures 290-292): the red bit below the F of Feed: (x 30-50,
	// rows 195-199) goes black
	if (_screen->agiDemake() && g_sci->getGameId() == GID_SQ3 && pictureId >= 290 && pictureId <= 292 && !addToFlag) {
		for (int y = 195; y <= 199; ++y)
			for (int x = 30; x <= 50; ++x)
				if ((_screen->agiDemakeGetPictureCode(x, y) & 15) == 4)
					_screen->agiDemakeSetPictureCode(x, y, 0);
	}

	// AGI demake, SQ3 the Terminator's face (picture 30): the innermost dark blue column at the left of
	// his visor (x 104-105) turns black, so the visor's text has a fat pixel of black before the blue
	if (_screen->agiDemake() && g_sci->getGameId() == GID_SQ3 && pictureId == 30 && !addToFlag) {
		for (int y = 41; y <= 101; ++y)
			if ((_screen->agiDemakeGetPictureCode(104, y) & 15) == 1 || (_screen->agiDemakeGetPictureCode(105, y) & 15) == 1) {
				_screen->agiDemakeSetPictureCode(104, y, 0);
				_screen->agiDemakeSetPictureCode(105, y, 0);
			}
	}

	// We make a call to SciPalette here, for increasing sys timestamp and also loading targetpalette, if palvary active
	//  (SCI1.1 only)
	if (getSciVersion() == SCI_VERSION_1_1)
		_palette->drewPicture(pictureId);

	// Reset custom per-picture palette mod
	_screen->setCurPaletteMapValue(0);
}

// This one is the only one that updates screen!
void GfxPaint16::drawCelAndShow(GuiResourceId viewId, int16 loopNo, int16 celNo, uint16 leftPos, uint16 topPos, byte priority, uint16 paletteNo, uint16 scaleX, uint16 scaleY, uint16 scaleSignal) {
	GfxView *view = _cache->getView(viewId);
	Common::Rect celRect;

	if (view) {
		celRect.left = leftPos;
		celRect.top = topPos;
		celRect.right = celRect.left + view->getWidth(loopNo, celNo);
		celRect.bottom = celRect.top + view->getHeight(loopNo, celNo);

		drawCel(view, loopNo, celNo, celRect, priority, paletteNo, scaleX, scaleY, scaleSignal);

		if (getSciVersion() >= SCI_VERSION_1_1) {
			if (!_screen->_picNotValidSci11) {
				bitsShow(celRect);
			}
		} else {
			if (!_screen->_picNotValid)
				bitsShow(celRect);
		}
	}
}

// This version of drawCel is not supposed to call bitsShow()!
void GfxPaint16::drawCel(GuiResourceId viewId, int16 loopNo, int16 celNo, const Common::Rect &celRect, byte priority, uint16 paletteNo, uint16 scaleX, uint16 scaleY, uint16 scaleSignal) {
	drawCel(_cache->getView(viewId), loopNo, celNo, celRect, priority, paletteNo, scaleX, scaleY, scaleSignal);
}

// This version of drawCel is not supposed to call bitsShow()!
void GfxPaint16::drawCel(GfxView *view, int16 loopNo, int16 celNo, const Common::Rect &celRect, byte priority, uint16 paletteNo, uint16 scaleX, uint16 scaleY, uint16 scaleSignal) {
	Common::Rect clipRect = celRect;
	clipRect.clip(_ports->_curPort->rect);
	if (clipRect.isEmpty()) // nothing to draw
		return;

	Common::Rect clipRectTranslated = clipRect;
	_ports->offsetRect(clipRectTranslated);
	if (scaleX == 128 && scaleY == 128)
		view->draw(celRect, clipRect, clipRectTranslated, loopNo, celNo, priority, paletteNo, false, scaleSignal);
	else
		view->drawScaled(celRect, clipRect, clipRectTranslated, loopNo, celNo, priority, scaleX, scaleY, scaleSignal);
}

// This is used as replacement for drawCelAndShow() when hires cels are drawn to screen. Hires cels are available only in SCI 1.1.
void GfxPaint16::drawHiresCelAndShow(GuiResourceId viewId, int16 loopNo, int16 celNo, uint16 leftPos, uint16 topPos, byte priority, uint16 paletteNo, reg_t hiresHandle, bool storeDrawingInfo) {
	GfxView *view = _cache->getView(viewId);
	if (!view)
		return;

	byte *memoryPtr = _segMan->getHunkPointer(hiresHandle);
	if (!memoryPtr) {
		// The original KQ6WinCD interpreter does not treat this as an error, it just skips the hires drawing. It happens all the time
		// when attempting to redraw the hires cels from the _hiresDrawObjs chain. We're supposed to just skip the invalidated items.
		return;
	}

	Common::Rect picRect;
	_screen->bitsGetRect(memoryPtr, &picRect);
	Common::Rect clipRect(makeHiresRect(picRect));
	Common::Rect celRect(view->getWidth(loopNo, celNo), view->getHeight(loopNo, celNo));
	celRect.translate(leftPos + clipRect.left, topPos + clipRect.top);
	clipRect.clip(celRect);

	view->draw(celRect, clipRect, clipRect, loopNo, celNo, priority, paletteNo, true);

	// The original KQ6WinCD interpreter saves the hires drawing information in a linked list. There are two use cases, one is redrawing the
	// window background when receiving WM_PAINT messages (which is irrelevant for us, since that happens in the backend) and the other is
	// redrawing the inventory after displaying a text window over it. This only happens in mixed speech+text mode, which does not even exist
	// in the original. We do have that mode as a ScummVM feature, though. That's why we have that code, to be able to refresh the inventory.
	// We also check if the portrait is drawn outside the viewport boundaries (happens in the unofficial mixed speech+text mode) and set
	// a flag to trigger a workaround when restoring the background.
	if (storeDrawingInfo && !hasHiresDrawObjectAt(leftPos, topPos))
		_hiresDrawObjs = new HiresDrawData(_hiresDrawObjs, hiresHandle, viewId, loopNo, celNo, leftPos, topPos, paletteNo, priority, picRect.top < _ports->_curPort->top);
}

void GfxPaint16::redrawHiresCels() {
	for (HiresDrawData *i = _hiresDrawObjs; i; i = i->next)
		drawHiresCelAndShow(i->viewId, i->lpNo, i->celNo, i->leftPos, i->topPos, i->prio, i->palNo, i->handle, false);
}

void GfxPaint16::clearScreen(byte color) {
	fillRect(_ports->_curPort->rect, GFX_SCREEN_MASK_ALL, color, 0, 0);
}

void GfxPaint16::invertRect(const Common::Rect &rect) {
	int16 oldpenmode = _ports->_curPort->penMode;
	_ports->_curPort->penMode = 2;
	fillRect(rect, GFX_SCREEN_MASK_VISUAL, _ports->_curPort->penClr, _ports->_curPort->backClr);
	_ports->_curPort->penMode = oldpenmode;
}

// used in SCI0early exclusively
void GfxPaint16::invertRectViaXOR(const Common::Rect &rect) {
	Common::Rect r = rect;

	r.clip(_ports->_curPort->rect);
	if (r.isEmpty()) // nothing to invert
		return;

	_ports->offsetRect(r);
	for (int16 y = r.top; y < r.bottom; y++) {
		for (int16 x = r.left; x < r.right; x++) {
			byte curVisual = _screen->getVisual(x, y);
			const byte flag = _screen->agiDemake() ? _screen->getDemakeFlag(x, y) : 0;
			_screen->putPixel(x, y, GFX_SCREEN_MASK_VISUAL, curVisual ^ 0x0f, 0, 0);
			if (_screen->agiDemake())
				_screen->setDemakeFlag(x, y, flag);
		}
	}
}

void GfxPaint16::eraseRect(const Common::Rect &rect) {
	fillRect(rect, GFX_SCREEN_MASK_VISUAL, _ports->_curPort->backClr);
}

void GfxPaint16::paintRect(const Common::Rect &rect) {
	fillRect(rect, GFX_SCREEN_MASK_VISUAL, _ports->_curPort->penClr);
}

void GfxPaint16::fillRect(const Common::Rect &rect, int16 drawFlags, byte color, byte priority, byte control) {
	Common::Rect r = rect;
	r.clip(_ports->_curPort->rect);
	if (r.isEmpty()) // nothing to fill
		return;

	int16 oldPenMode = _ports->_curPort->penMode;
	_ports->offsetRect(r);
	int16 x, y;

	// Doing visual first
	if (drawFlags & GFX_SCREEN_MASK_VISUAL) {
		if (oldPenMode == 2) { // invert mode
			for (y = r.top; y < r.bottom; y++) {
				for (x = r.left; x < r.right; x++) {
					byte curVisual = _screen->getVisual(x, y);
					// AGI demake: inverting (menu highlights) must keep the text flag, or the
					// highlighted text gets collapsed to fat pixels and corrupts
					const byte flag = _screen->agiDemake() ? _screen->getDemakeFlag(x, y) : 0;
					if (curVisual == color) {
						_screen->putPixel(x, y, GFX_SCREEN_MASK_VISUAL, priority, 0, 0);
					} else if (curVisual == priority) {
						_screen->putPixel(x, y, GFX_SCREEN_MASK_VISUAL, color, 0, 0);
					}
					if (_screen->agiDemake())
						_screen->setDemakeFlag(x, y, flag);
				}
			}
		} else { // just fill rect with color
			for (y = r.top; y < r.bottom; y++) {
				for (x = r.left; x < r.right; x++) {
					_screen->putPixel(x, y, GFX_SCREEN_MASK_VISUAL, color, 0, 0);
				}
			}
		}
	}

	if (drawFlags < 2)
		return;
	drawFlags &= GFX_SCREEN_MASK_PRIORITY|GFX_SCREEN_MASK_CONTROL;

	// we need to isolate the bits, sierra sci saved priority and control inside one byte, we don't
	priority &= 0x0f;
	control &= 0x0f;

	if (oldPenMode != 2) {
		for (y = r.top; y < r.bottom; y++) {
			for (x = r.left; x < r.right; x++) {
				_screen->putPixel(x, y, drawFlags, 0, priority, control);
			}
		}
	} else {
		for (y = r.top; y < r.bottom; y++) {
			for (x = r.left; x < r.right; x++) {
				_screen->putPixel(x, y, drawFlags, 0, !_screen->getPriority(x, y), !_screen->getControl(x, y));
			}
		}
	}
}

void GfxPaint16::frameRect(const Common::Rect &rect, bool demakeFrame) {
	// AGI demake: flag frame pixels so thin borders become fat lines instead of vanishing
	byte oldMapValue = _screen->getCurPaletteMapValue();
	if (_screen->agiDemake() && demakeFrame)
		_screen->setCurPaletteMapValue(2);
	Common::Rect r = rect;
	// left
	r.right = rect.left + 1;
	paintRect(r);
	// right
	r.right = rect.right;
	r.left = rect.right - 1;
	paintRect(r);
	//top
	r.left = rect.left;
	r.bottom = rect.top + 1;
	paintRect(r);
	//bottom
	r.bottom = rect.bottom;
	r.top = rect.bottom - 1;
	paintRect(r);
	_screen->setCurPaletteMapValue(oldMapValue);
}

void GfxPaint16::bitsShow(const Common::Rect &rect) {
	Common::Rect workerRect(rect.left, rect.top, rect.right, rect.bottom);

	// WORKAROUND for vertically misplaced hires portraits in mixed speech+text mode in KQ6CD. The original interpreter
	// did not have that mode, so the devs had no need to fix it. These portraits get drawn above the viewport top, where
	// the engine would normally be unable to update the screen. We just have to offset the rect first, before clipping it,
	// to make it work. Another solution would be to improve the script patches that implement the speech/text mode for
	// better vertical placement of the portrait frames (inside the port rect).
	bool triggeredWorkaround = false;
	if (rect.top < 0 && _hiresPortraitWorkaroundFlag) {
		_ports->offsetRect(workerRect);
		triggeredWorkaround = true;
		_hiresPortraitWorkaroundFlag = false;
	}

	workerRect.clip(_ports->_curPort->rect);
	if (workerRect.isEmpty()) // nothing to show
		return;

	// WORKAROUND, see comment above. Normally, the call to _ports->offsetRect(workerRect) would be unconditional here.
	if (!triggeredWorkaround)
		_ports->offsetRect(workerRect);

	// We adjust the left/right coordinates to even coordinates
	workerRect.left &= 0xFFFE; // round down
	workerRect.right = (workerRect.right + 1) & 0xFFFE; // round up

	_screen->copyRectToScreen(workerRect);
}
reg_t GfxPaint16::bitsSave(const Common::Rect &rect, byte screenMask, bool hiresFlag) {
	Common::Rect workerRect(rect.left, rect.top, rect.right, rect.bottom);
	if (!hiresFlag) { // KQ6CD Win only does this if not called from the special kGraph 15 case (= kGraphSaveUpscaledHiresBox)
		workerRect.clip(_ports->_curPort->rect);
		if (workerRect.isEmpty()) // nothing to save
			return NULL_REG;
		_ports->offsetRect(workerRect);
	}

	// now actually ask _screen how much space it will need for saving
	int size = _screen->bitsGetDataSize(workerRect, screenMask);

	reg_t memoryId = _segMan->allocateHunkEntry("SaveBits()", size);
	byte *memoryPtr = _segMan->getHunkPointer(memoryId);
	if (memoryPtr)
		_screen->bitsSave(workerRect, screenMask, memoryPtr);
	return memoryId;
}

void GfxPaint16::bitsGetRect(reg_t memoryHandle, Common::Rect *destRect) {
	if (!memoryHandle.isNull()) {
		byte *memoryPtr = _segMan->getHunkPointer(memoryHandle);

		if (memoryPtr) {
			_screen->bitsGetRect(memoryPtr, destRect);
		}
	}
}

void GfxPaint16::bitsRestore(reg_t memoryHandle) {
	if (!memoryHandle.isNull()) {
		byte *memoryPtr = _segMan->getHunkPointer(memoryHandle);

		if (memoryPtr) {
			_screen->bitsRestore(memoryPtr);
			bitsFree(memoryHandle);
		}

		// KQ6WinCD specific
		if (_screen->gfxDriver()->supportsHiResGraphics())
			removeHiresDrawObject(memoryHandle);
	}
}

void GfxPaint16::bitsFree(reg_t memoryHandle) {
	if (!memoryHandle.isNull())	// happens in KQ5CD
		_segMan->freeHunkEntry(memoryHandle);
}

void GfxPaint16::kernelDrawPicture(GuiResourceId pictureId, int16 animationNr, bool animationBlackoutFlag, bool mirroredFlag, bool addToFlag, int16 EGApaletteNo) {
	Port *oldPort = _ports->setPort((Port *)_ports->_picWind);

	if (_ports->isFrontWindow(_ports->_picWind)) {
		_screen->_picNotValid = 1;
		drawPicture(pictureId, mirroredFlag, addToFlag, EGApaletteNo);
		_transitions->setup(animationNr, animationBlackoutFlag);
	} else {
		// We need to set it for SCI1EARLY+ (sierra sci also did so), otherwise we get at least the following issues:
		//  LSL5 (english) - last wakeup (taj mahal flute dream)
		//  SQ5 (english v1.03) - during the scene following the scrubbing
		//   in both situations a window is shown when kDrawPic is called, which would result otherwise in
		//   no showpic getting called from kAnimate and we would get graphic corruption
		// XMAS1990 EGA did not set it in this case, VGA did
		if (getSciVersion() >= SCI_VERSION_1_EARLY)
			_screen->_picNotValid = 1;
		_ports->beginUpdate(_ports->_picWind);
		drawPicture(pictureId, mirroredFlag, addToFlag, EGApaletteNo);
		_ports->endUpdate(_ports->_picWind);
	}
	_ports->setPort(oldPort);
}

void GfxPaint16::kernelDrawCel(GuiResourceId viewId, int16 loopNo, int16 celNo, uint16 leftPos, uint16 topPos, int16 priority, uint16 paletteNo, uint16 scaleX, uint16 scaleY, bool hiresMode, reg_t hiresHandle) {
	// some calls are hiresMode even under kq6 DOS, that's why we check for hires caps here
	if (!hiresMode || !_screen->gfxDriver()->supportsHiResGraphics()) {
		drawCelAndShow(viewId, loopNo, celNo, leftPos, topPos, priority, paletteNo, scaleX, scaleY);
	} else {
		drawHiresCelAndShow(viewId, loopNo, celNo, leftPos, topPos, priority, paletteNo, hiresHandle, true);
	}
}

bool GfxPaint16::agiTerminal() const {
	return _screen->agiDemake() && g_sci->getGameId() == GID_PQ2 && g_sci->getEngineState() &&
		g_sci->getEngineState()->currentRoomNumber() == 8;
}


void agiDemakeLogLine(const Common::String &line);

void GfxPaint16::kernelGraphFillBoxForeground(const Common::Rect &rectIn) {
	Common::Rect rect = rectIn;
	if (agiTerminal()) {
		agiDemakeLogLine(Common::String::format("room 8 kGraph fillfore %d,%d-%d,%d", rect.left, rect.top, rect.right, rect.bottom));
		if (rect.top < 24 && rect.left > 73 && rect.width() < 12)
			rect.moveTo(_screen->agiTermScreenX(rect.left), rect.top);
	}
	paintRect(rect);
}

void GfxPaint16::kernelGraphFillBoxBackground(const Common::Rect &rectIn) {
	Common::Rect rect = rectIn;
	if (agiTerminal()) {
		agiDemakeLogLine(Common::String::format("room 8 kGraph fillback %d,%d-%d,%d", rect.left, rect.top, rect.right, rect.bottom));
		if (rect.top < 24 && rect.left > 73 && rect.width() < 12)
			rect.moveTo(_screen->agiTermScreenX(rect.left), rect.top);
	}
	eraseRect(rect);
}

void GfxPaint16::kernelGraphFillBox(const Common::Rect &rectIn, uint16 colorMask, int16 color, int16 priority, int16 control) {
	Common::Rect rect = rectIn;
	if (agiTerminal()) {
		agiDemakeLogLine(Common::String::format("room 8 kGraph fillbox %d,%d-%d,%d mask %d color %d", rect.left, rect.top, rect.right, rect.bottom, colorMask, color));
		// the cursor block on the prompt line follows the typed characters
		if (rect.top < 24 && rect.left > 73 && rect.width() < 12)
			rect.moveTo(_screen->agiTermScreenX(rect.left), rect.top);
	}
	fillRect(rect, colorMask, color, priority, control);
}

void GfxPaint16::kernelGraphFrameBox(const Common::Rect &rect, int16 color) {
	int16 oldColor = _ports->getPort()->penClr;
	_ports->penColor(color);
	frameRect(rect);
	_ports->penColor(oldColor);
}

void GfxPaint16::kernelGraphDrawLine(Common::Point startPoint, Common::Point endPoint, int16 color, int16 priority, int16 control) {
	if (agiTerminal())
		agiDemakeLogLine(Common::String::format("room 8 kGraph line %d,%d-%d,%d color %d", startPoint.x, startPoint.y, endPoint.x, endPoint.y, color));
	_ports->clipLine(startPoint, endPoint);
	_ports->offsetLine(startPoint, endPoint);
	_screen->drawLine(startPoint.x, startPoint.y, endPoint.x, endPoint.y, color, priority, control);
}

reg_t GfxPaint16::kernelGraphSaveBox(const Common::Rect &rect, uint16 screenMask, bool hiresFlag) {
	return bitsSave(rect, screenMask, hiresFlag);
}

void GfxPaint16::kernelGraphRestoreBox(reg_t handle) {
	bitsRestore(handle);
}

void GfxPaint16::kernelGraphUpdateBox(const Common::Rect &rect) {
	bitsShow(rect);
}

void GfxPaint16::kernelGraphRedrawBox(Common::Rect rect) {
	_coordAdjuster->kernelLocalToGlobal(rect.left, rect.top);
	_coordAdjuster->kernelLocalToGlobal(rect.right, rect.bottom);
	Port *oldPort = _ports->setPort((Port *)_ports->_picWind);
	_coordAdjuster->kernelGlobalToLocal(rect.left, rect.top);
	_coordAdjuster->kernelGlobalToLocal(rect.right, rect.bottom);

	_animate->reAnimate(rect);

	_ports->setPort(oldPort);
}

#define SCI_DISPLAY_MOVEPEN				100
#define SCI_DISPLAY_SETALIGNMENT		101
#define SCI_DISPLAY_SETPENCOLOR			102
#define SCI_DISPLAY_SETBACKGROUNDCOLOR	103
#define SCI_DISPLAY_SETGREYEDOUTPUT		104
#define SCI_DISPLAY_SETFONT				105
#define SCI_DISPLAY_WIDTH				106
#define SCI_DISPLAY_SAVEUNDER			107
#define SCI_DISPLAY_RESTOREUNDER		108
#define SCI_DISPLAY_DONTSHOWBITS		121
#define SCI_DISPLAY_SETSTROKE			122

// AGI demake: text displayed straight onto the screen (a full-width port, not a window) uses
// tight letter spacing, so pages of text fit like Sierra's narrower font did
struct AgiDemakeTightGuard {
	GfxText16 *_text16;
	AgiDemakeTightGuard(GfxText16 *text16, bool on) : _text16(text16) { _text16->setAgiTight(on); }
	~AgiDemakeTightGuard() { _text16->setAgiTight(false); }
};

reg_t GfxPaint16::kernelDisplay(const char *text, uint16 languageSplitter, int argc, reg_t *argv) {
	AgiDemakeTightGuard tightGuard(_text16, _screen->agiDemake() && _ports->getPort()->rect.width() >= _screen->getWidth());
	reg_t displayArg;
	TextAlignment alignment = SCI_TEXT16_ALIGNMENT_LEFT;
	int16 colorPen = -1, colorBack = -1, width = -1, bRedraw = 1;
	bool doSaveUnder = false;
	Common::Rect rect;
	reg_t result = NULL_REG;
	int16 stroke = 0; // Kawa's SCI11+

	// Make a "backup" of the port settings (required for some SCI0LATE and
	// SCI01+ only)
	Port oldPort = *_ports->getPort();

	// setting defaults
	_ports->penMode(0);
	_ports->penColor(0);
	_ports->textGreyedOutput(false);
	// processing codes in argv
	while (argc > 0) {
		displayArg = argv[0];
		if (displayArg.getSegment())
			displayArg.setOffset(0xFFFF);
		argc--; argv++;
		switch (displayArg.getOffset()) {
		case SCI_DISPLAY_MOVEPEN:
			// AGI demake, SQ3 ship's computer (room 17): the sector number goes at x 100, which in the
			// AGI font is on top of "CURRENT SECTOR:" (that ends at x 125). It moves to x 132
			if (_screen->agiDemake() && g_sci->getGameId() == GID_SQ3 && g_sci->getEngineState()->currentRoomNumber() == 17 &&
					argv[0].toUint16() == 100 && argv[1].toUint16() == 144) {
				_ports->moveTo(132, 144);
				argc -= 2; argv += 2;
				break;
			}
			// AGI demake, SQ3 weapons screen (room 18): "TARGET:" at x 44 runs over its panel's edge in
			// the AGI font. It goes to the top of the blue box, centred (x 37, 2 pixels below the top)
			if (_screen->agiDemake() && g_sci->getGameId() == GID_SQ3 && g_sci->getEngineState()->currentRoomNumber() == 18 &&
					argv[0].toUint16() == 44 && argv[1].toUint16() == 110) {
				_ports->moveTo(37, 106);
				argc -= 2; argv += 2;
				break;
			}
			_ports->moveTo(argv[0].toUint16(), argv[1].toUint16());
			argc -= 2; argv += 2;
			break;
		case SCI_DISPLAY_SETALIGNMENT:
			alignment = argv[0].toSint16();
			argc--; argv++;
			break;
		case SCI_DISPLAY_SETPENCOLOR:
			colorPen = argv[0].toUint16();
			_ports->penColor(colorPen);
			argc--; argv++;
			break;
		case SCI_DISPLAY_SETBACKGROUNDCOLOR:
			colorBack = argv[0].toUint16();
			argc--; argv++;
			break;
		case SCI_DISPLAY_SETGREYEDOUTPUT:
			_ports->textGreyedOutput(!argv[0].isNull());
			argc--; argv++;
			break;
		case SCI_DISPLAY_SETFONT:
			_text16->SetFont(argv[0].toUint16());
			argc--; argv++;
			break;
		case SCI_DISPLAY_WIDTH:
			width = argv[0].toUint16();
			argc--; argv++;
			break;
		case SCI_DISPLAY_SAVEUNDER:
			doSaveUnder = true;
			break;
		case SCI_DISPLAY_RESTOREUNDER:
			bitsGetRect(argv[0], &rect);
			rect.translate(-_ports->getPort()->left, -_ports->getPort()->top);
			bitsRestore(argv[0]);
			kernelGraphRedrawBox(rect);
			// finishing loop
			argc = 0;
			break;
		case SCI_DISPLAY_DONTSHOWBITS:
			bRedraw = 0;
			break;
		case SCI_DISPLAY_SETSTROKE: // From Kawa's SCI11+
			stroke = argv[0].toUint16();
			argc--; argv++;
			break;

		default:
			SciCallOrigin originReply;
			SciWorkaroundSolution solution = trackOriginAndFindWorkaround(0, kDisplay_workarounds, &originReply);
			if (solution.type == WORKAROUND_NONE)
				error("Unknown kDisplay argument (%04x:%04x)", PRINT_REG(displayArg));
			assert(solution.type == WORKAROUND_IGNORE);
			break;
		}
	}

	// AGI demake: PQ2's police computer (room 8, terminal font 7). The program places every
	// character itself, spaced for Sierra's narrow font: the prompt line steps 6 pixels per typed
	// character, and directory listings sit in two columns at x 73 and 155.
	Common::String agiTerminalText;
	int16 agiTermTypedX = -1;
	bool agiTermPrompt = false;
	if (agiTerminal() && _ports->getPort()->fontId == 7) {
		Port *port = _ports->getPort();
		// spaces keep the terminal font's own width, so Sierra's blanking strings cover what
		// Sierra meant them to
		// blanking strings (only spaces) keep the terminal font's own space width, so they cover
		// what Sierra meant them to; spaces between words are 3 pixels
		GfxFont *orig = _cache->getFont(7);
		bool onlySpaces = true;
		for (const char *c = text; *c; ++c)
			if (*c != ' ')
				onlySpaces = false;
		if (port->curTop < 24)
			_text16->setAgiSpaceWidth(onlySpaces ? 9 : 3);	// prompt line blanking must cover a whole AGI prompt
		else
			_text16->setAgiSpaceWidth(onlySpaces ? (orig ? orig->getCharWidth(' ') : 6) : 3);
		if (port->curTop < 24) {
			if (port->curLeft <= 73) {
				agiTermPrompt = !onlySpaces;
				if (agiTermPrompt) {
					// prompts start level with the rest of the screen ("  DIR? > " is indented in the game)
					while (*text == ' ')
						++text;
					// a new prompt: the whole prompt line is cleared first (old typing, old cursor)
					fillRect(Common::Rect(73, port->curTop, 252, port->curTop + 9), GFX_SCREEN_MASK_VISUAL, 0, 0, 0);
					bitsShow(Common::Rect(73, port->curTop, 252, port->curTop + 9));
				}
			} else if (onlySpaces) {
				// blanking typed text: starts where that character was drawn, and typing resumes there
				int16 x;
				if (_screen->_agiTermX.contains(port->curLeft))
					x = _screen->_agiTermX[port->curLeft];
				else if (_screen->_agiTermFirstScriptX < 0 || port->curLeft <= _screen->_agiTermFirstScriptX)
					x = _screen->_agiTermStart;
				else
					x = _screen->_agiTermNext;
				const int16 scriptX = port->curLeft;
				port->curLeft = x;
				_screen->_agiTermNext = x;
				Common::Array<int16> stale;
				for (Common::HashMap<int16, int16>::const_iterator it = _screen->_agiTermX.begin(); it != _screen->_agiTermX.end(); ++it)
					if (it->_key > scriptX)
						stale.push_back(it->_key);
				for (uint i = 0; i < stale.size(); ++i)
					_screen->_agiTermX.erase(stale[i]);
				_screen->_agiTermX[scriptX] = x;
			} else {
				agiTermTypedX = port->curLeft;
				if (_screen->_agiTermFirstScriptX < 0 || port->curLeft < _screen->_agiTermFirstScriptX)
					_screen->_agiTermFirstScriptX = port->curLeft;
				port->curLeft = _screen->agiTermScreenX(port->curLeft);
				// clear the character's cell (and any cursor block left there) before drawing it
				const Common::Rect cell(port->curLeft, port->curTop, MIN<int16>(port->curLeft + 9, 252), port->curTop + 9);
				fillRect(cell, GFX_SCREEN_MASK_VISUAL, 0, 0, 0);
				bitsShow(cell);	// the text only refreshes its own (narrower) area
			}
		} else {
			// listings: Sierra's own game list uses short names so both columns fit
			static const char *const kShortNames[][2] = {
				{ "POLICE QUEST", "PQ I" }, { "KING'S QUEST", "KQ I" }, { "MOTHER GOOSE", "MUMG" }, { "SPACE QUEST", "SQ I" }
			};
			agiTerminalText = text;
			for (uint n = 0; n < ARRAYSIZE(kShortNames); ++n) {
				if (agiTerminalText == kShortNames[n][0])
					agiTerminalText = kShortNames[n][1];
				else if (agiTerminalText == Common::String("| ") + kShortNames[n][0])
					agiTerminalText = Common::String("| ") + kShortNames[n][1];
			}
			// personnel: "Surname, Firstname" becomes "Surname, F." so both columns fit
			const int comma = agiTerminalText.find(", ");
			if (comma >= 0 && comma + 2 < (int)agiTerminalText.size() && Common::isAlpha(agiTerminalText[comma + 2])) {
				bool lettersOnly = true;
				for (uint i = comma + 2; i < agiTerminalText.size(); ++i)
					if (!Common::isAlpha(agiTerminalText[i]) && agiTerminalText[i] != ' ')
						lettersOnly = false;
				if (lettersOnly)
					agiTerminalText = Common::String(agiTerminalText.c_str(), comma + 3) + ".";
			}
			text = agiTerminalText.c_str();
			// right-hand column ("| name"): moved 10 pixels right so long left-hand names fit, and the
			// selection frame is moved by the same amount the name start moves
			if (agiTerminalText.hasPrefix("| ") && port->curLeft >= 150) {
				const int16 origPrefix = orig ? orig->getCharWidth('|') + orig->getCharWidth(' ') : 8;
				const int16 agiPrefix = _text16->agiWidth("| ", false);
				port->curLeft += 10;
				_screen->_agiTermFrameShift = 10 + agiPrefix - origPrefix;
			}
		}
	}

	Common::String agiFirstLine, agiSecondLine;	// QfG1 spells list, see below
	// AGI demake: QfG1 inventory screen (any room: the full-screen window that starts with "You Are
	// Carrying:"). The item list is indented 30 pixels, which the 8x8 font can't spare, and "Weight
	// Carried" runs into the right frame. The quantities (right-aligned boxes at x 15) and items (x 40)
	// move 11 pixels left, which still leaves room for two digit quantities, and the right column
	// (x 180 and 220) moves 6 pixels left, matching the gap left of "You"
	if (_screen->agiDemake() && g_sci->getGameId() == GID_QFG1 && text) {
		Port *port = _ports->getPort();
		if (strstr(text, "You Are Carrying:")) {
			_agiInventoryPort = port;
			_agiItemShift = 0;
			_agiItemPending = 0;
		}
		else if (port != _agiInventoryPort)
			_agiInventoryPort = nullptr;
		if (_agiInventoryPort && port == _agiInventoryPort && port->rect.width() >= 318) {
			if (port->curLeft == 15 || port->curLeft == 40) {
				// items: a name that would run into the right column (wider than 150 pixels, "Flask of
				// Undead Unguent") puts its last word(s) on the next line, indented, and every row after
				// it moves down a line. The quantity (x 15) starts each row, then the name (x 40)
				if (port->curTop >= 24) {
					if (port->curLeft == 15) {
						_agiItemShift += _agiItemPending;
						_agiItemPending = 0;
					}
					port->curTop += _agiItemShift;
					if (port->curLeft == 40 && _text16->agiWidth(text, true) > 150) {
						const char *cut = nullptr;
						for (const char *sp = strchr(text, ' '); sp; sp = strchr(sp + 1, ' ')) {
							Common::String first(text, sp - text);
							if (_text16->agiWidth(first.c_str(), true) <= 150)
								cut = sp;
						}
						if (cut) {
							agiFirstLine = Common::String(text, cut - text);
							agiSecondLine = cut + 1;
							text = agiFirstLine.c_str();
							_agiItemPending += 9;
						}
					}
				}
				port->curLeft -= 11;
			} else if (port->curLeft == 180 || port->curLeft == 220) {
				port->curLeft -= 6;
				// coins and weights: rows 9 pixels apart like the item list (Sierra's were 13)
				if (port->curTop == 23 || port->curTop == 36 || port->curTop == 49)
					port->curTop = 10 + (port->curTop - 10) * 9 / 13;
			}
			// spells (a magic user's inventory): a two word name, too wide for its column in the 8 pixel
			// font ("Detect Magic", "Flame Dart"), puts its second word on the next line, indented, and every row after it
			// moves down a line. The MP value (x 192) starts each row, then the name (218), then
			// the skill (290)
			if (port->curTop >= 91 && (port->curLeft == 192 || port->curLeft == 218 || port->curLeft == 290)) {
				if (port->curLeft == 192) {
					_agiSpellShift += _agiSpellPending;
					_agiSpellPending = 0;
				}
				port->curTop += _agiSpellShift;
				const char *space = strchr(text, ' ');
				if (port->curLeft == 218 && space) {
					agiFirstLine = Common::String(text, space - text);
					agiSecondLine = space + 1;
					text = agiFirstLine.c_str();
					_agiSpellPending += 9;
				}
			} else if (strstr(text, "Spells:")) {
				_agiSpellShift = 0;
				_agiSpellPending = 0;
			}
		}
	}

	// AGI demake: QfG1 new hero screen (room 203) and the character sheet in game (any room, same
	// layout). The Health, Stamina and Magic Points values ("18 / 18") sit at x 103, which the tight
	// AGI font reaches with "Stamina Points". They are moved 8 pixels right, and their box keeps its
	// right edge (clear of the "values." line above on the new hero screen) unless the value needs more.
	if (_screen->agiDemake() && g_sci->getGameId() == GID_QFG1 && text && strstr(text, " / ")) {
		Port *port = _ports->getPort();
		if (port->rect.width() >= 318 && port->curLeft == 103 && (port->curTop == 148 || port->curTop == 160 || port->curTop == 172)) {
			port->curLeft += 8;
			if (width > 8)
				width = MAX<int16>(width - 8, _text16->agiWidth(text, true));
		}
	}

	// AGI demake, SQ3 navigation screen (room 19): a planet's details shown left of its zoom box (the
	// planet in the right half of the map) get a 91 pixel column ending just left of the box. In the
	// AGI font that wraps to 9 lines and runs off the bottom. The column widens to the longest
	// hand-broken line and starts further left, its right edge where it was
	if (_screen->agiDemake() && g_sci->getGameId() == GID_SQ3 && g_sci->getEngineState()->currentRoomNumber() == 19 &&
			width == 91 && text) {
		int16 widest = 0;
		const char *line = text;
		for (const char *p = text; ; ++p) {
			if (*p == 0 || *p == 0x0A || *p == 0x0D) {
				widest = MAX<int16>(widest, _text16->agiWidth(Common::String(line, p - line).c_str(), true));
				if (*p == 0)
					break;
				line = p + 1;
			}
		}
		if (widest + 1 > width) {
			Port *port = _ports->getPort();
			port->curLeft = MAX<int16>(0, port->curLeft - (widest + 1 - width));
			width = widest + 1;
		}
	}
	// and a planet shown right of its zoom box (width 220, planet on the left of the map): its text
	// starts 1 pixel further right, so the black outline sits 1 pixel clear of the box's frame
	if (_screen->agiDemake() && g_sci->getGameId() == GID_SQ3 && g_sci->getEngineState()->currentRoomNumber() == 19 &&
			width == 220 && text)
		_ports->getPort()->curLeft += 1;

	// AGI demake, SQ3 the Terminator's visor (room 31): the messages must fit the black of his
	// glasses (x 104 to 235) in the AGI font. ROGER WILCO / CASE OU812 starts below IDENTITY
	// CONFIRMED with a 4 row gap. WANTED FOR is reworded to fit (the text is mirrored, so it is
	// written backwards) and drawn as two pairs of lines 4 rows apart, centred on the visor
	bool sq3Wanted = false;
	if (_screen->agiDemake() && g_sci->getGameId() == GID_SQ3 && g_sci->getEngineState()->currentRoomNumber() == 31 && text) {
		Port *port = _ports->getPort();
		if (port->curLeft == 116 && port->curTop == 45 && !strcmp(text, "OCLIW REGOR\n:218UO ESAC"))
			port->curTop = 55;
		if (!strcmp(text, "ROF DETNAW OCLIW\nDUARF ENIHCAM GNIDNEV\n\n:FFITNIALP\n.OC YTLEVON DIOZAPPIG ")) {
			sq3Wanted = true;
			port->curLeft = 106;
			width = 130;
		}
	}

	// AGI demake, SQ3 Astro Chicken (room 290): the attract screen's texts fit the arcade screen in the
	// AGI font (208 pixels wide from x 55). The title is broken into 3 + 2 centred lines. The
	// instructions are hand broken, with a 3 pixel gap between sections and the labels coloured
	// (Controls: and Feed: light red, the arrows yellow); see the drawing below
	static const char *const kAstroInstr[] = {
		"Controls:", "Left arrow: move left", "Down arrow: stop left/right", "movement", "Right arrow: move right",
		"Up arrow: toggle flapping", "Feed: don't use up all your feed!", "Hit the landing pad too fast and",
		"you'll bounce back up.", "Landing outside the landing pad", "is fatal.", "If you fly too high you'll bounce",
		"off the atmosphere and plummet", "back to the surface."
	};
	// AGI demake, SQ3 Monolith Burger menu (room 280): Sierra lines it up with "~" (blank in its font,
	// tildes in the AGI font) and dots that no longer reach the prices. Laid out again: names from 4
	// pixels in, prices right-aligned 290 pixels in, dot leaders between, the Big Belcher's contents
	// over 3 lines. Drawn for both of Sierra's calls (the black shadow, then the white 1 pixel over)
	static const struct { const char *name, *price; } kMenu[] = {
		{ "1. MINI MONOLITH ..................................................", "1.00" },
		{ "2. MONOLITH ............................................................", "2.00" },
		{ "3.    W/POLYCHEEZE .............................................", "3.00" },
		{ "4. FILET-O-ORAT ..................................................", "2.00" },
		{ "5. JUMBO MONOLITH W/POLYCHEEZE .............", "5.00" },
		{ "6. BIG BELCHER COMBO .......................................", "9.00" },
		{ "    INCLUDES: JUMBO MONO W/POLYCHEEZE,", nullptr },
		{ "    SPACE SPUDS W/EXTRA GREASE AND", nullptr },
		{ "    SLOPPY SLURPER", nullptr },
		{ "7. MONOLITH FUN MEAL ........................................", "7.00" },
		{ "8. SPACE SPUDS ......................................................", "1.00" },
		{ "TANG", nullptr },
		{ "9.    SMALL ...............................................................", "1.00" },
		{ "A.    MED ....................................................................", "2.00" },
		{ "B.    LARGE ...............................................................", "3.00" },
		{ "C. SLOPPY SLURPER .............................................", "4.00" }
	};
	const bool sq3Menu = _screen->agiDemake() && g_sci->getGameId() == GID_SQ3 &&
		g_sci->getEngineState()->currentRoomNumber() == 280 && text && !strncmp(text, "1. MINI MONOLITH", 16);
	// SQ3 Monolith Burger (room 25): the price in the employee's YOUR TOTAL IS _ BUCKAZOIDS. bubble sits
	// just after IS in the redrawn bubble
	if (_screen->agiDemake() && g_sci->getGameId() == GID_SQ3 && g_sci->getEngineState()->currentRoomNumber() == 25 &&
			_ports->getPort()->curLeft == 207 && _ports->getPort()->curTop == 98) {
		_ports->getPort()->curLeft = 229;
		_ports->getPort()->curTop = 94;
	}
	// SQ3 robot fight instructions (room 96): shortened ("key" and the dashes gone), each line centred
	// in the black gap between the hands; "Press Any Key To Start" is replaced by PRESS ANY KEY
	// flashing on the bottom line (agiInputSq3PressAnyKey)
	if (_screen->agiDemake() && g_sci->getGameId() == GID_SQ3 && g_sci->getEngineState()->currentRoomNumber() == 96 && text) {
		static const struct { const char *from, *to; int16 x, y; } kInstr[] = {
			{ "-- INSTRUCTIONS --", "INSTRUCTIONS", 121, 136 },
			{ "  use arrow keys to", "use arrows to", 119, 144 },
			{ " control your robot", "control robot", 119, 152 },
			{ "use the J key to punch", "press J to punch", 108, 160 },
			{ "use the M key to block", "press M to block", 112, 168 },
			{ "Press Any Key To Start", "", 106, 173 }
		};
		for (uint i = 0; i < ARRAYSIZE(kInstr); ++i)
			if (!strcmp(text, kInstr[i].from)) {
				text = kInstr[i].to;
				_ports->getPort()->curLeft = kInstr[i].x;
				_ports->getPort()->curTop = kInstr[i].y;
				if (!*text)
					agiInputSq3PressAnyKey();
				break;
			}
	}
	// SQ3 weapons screen (room 18): the shield readout under TARGET: shortened (NEGATIVE reads MID) and
	// centred between the radar box's blue sides (x 34-86, centre 60)
	if (_screen->agiDemake() && g_sci->getGameId() == GID_SQ3 && g_sci->getEngineState()->currentRoomNumber() == 18 && text &&
			_ports->getPort()->curLeft == 40) {
		static const struct { const char *from, *to; int16 x; } kShield[] = {
			{ "IN FRONT", "FRONT", 41 }, { "NEGATIVE", "MID", 50 }, { " IN REAR", "REAR", 45 }
		};
		for (uint i = 0; i < ARRAYSIZE(kShield); ++i)
			if (!strcmp(text, kShield[i].from)) {
				text = kShield[i].to;
				_ports->getPort()->curLeft = kShield[i].x;
				width = 84 - kShield[i].x;	// the text box stops short of the frame's blue side (x 87), which it was wiping
				break;
			}
	}
	bool sq3AstroInstr = false;
	// (and the cabinet lines: Insert Buckazoid! and PRESS F6 TO EXIT move up into the black strip above
	// the arcade screen, PRESS F6 in yellow, so they are on one line each and nothing covers them; Feed:
	// and its count move left to clear the chickens-left icons)
	if (_screen->agiDemake() && g_sci->getGameId() == GID_SQ3 && g_sci->getEngineState()->currentRoomNumber() == 290 && text) {
		Port *port = _ports->getPort();
		if (!strcmp(text, "Insert Buckazoid!")) {
			port->curLeft = 40;
			port->curTop = 12;
			width = 116;
		} else if (!strcmp(text, "PRESS F6 TO EXIT")) {
			port->curLeft = 176;
			port->curTop = 12;
			width = 110;
			colorPen = 14;
			_ports->penColor(colorPen);
		} else if (!strcmp(text, "Feed:") && port->curTop == 180) {
			port->curLeft = 34;
		} else if (port->curLeft == 80 && port->curTop == 180) {
			port->curLeft = 69;
			width = 24;	// the black box behind the count covers all 3 digits ("100" left a 0 behind)
		}
	}
	if (_screen->agiDemake() && g_sci->getGameId() == GID_SQ3 && g_sci->getEngineState()->currentRoomNumber() == 290 && text) {
		if (!strncmp(text, "   ASTRO CHICKEN MUST LAND ON THE", 33)) {
			text = "ASTRO CHICKEN MUST\nLAND ON THE ASTRO\nCHICKEN LANDING PAD.\n\nHE'S DEPENDING ON YOU\nTO BRING HIM TO SAFETY!";
			alignment = SCI_TEXT16_ALIGNMENT_CENTER;
			_ports->getPort()->curLeft = 55;
			width = 208;
		} else if (!strncmp(text, "Controls:\nLeft arrow:", 21)) {
			sq3AstroInstr = true;
			_ports->getPort()->curLeft = 55;
			width = 208;
		}
	}

	// now drawing the text
	_text16->Size(rect, text, languageSplitter, -1, width);
	if (sq3AstroInstr)
		rect.bottom = rect.top + (int16)ARRAYSIZE(kAstroInstr) * 8 + 4 * 3;
	if (sq3Menu)
		rect.bottom = rect.top + (int16)ARRAYSIZE(kMenu) * 8;
	rect.moveTo(_ports->getPort()->curLeft, _ports->getPort()->curTop);
	if (agiTerminal() && _ports->getPort()->fontId == 7) {
		// keep blanking inside the monitor's screen
		if (rect.right > 252)
			rect.right = MAX<int16>(rect.left, 252);
		// listing columns as actually drawn, for sizing the selection frame
		bool listingText = rect.top >= 24;
		for (const char *c = text; *c && listingText; ++c)
			if (*c != ' ')
				listingText = false;
		listingText = !listingText && rect.top >= 24;
		if (listingText) {
			if (rect.left < 150) {
				if (rect.top == 24)
					_screen->_agiTermCol1Right = _screen->_agiTermCol2Right = 0;
				_screen->_agiTermCol1Right = MAX<int16>(_screen->_agiTermCol1Right, rect.left + _text16->agiWidth(text, true));
			} else {
				_screen->_agiTermCol2X = rect.left;
				_screen->_agiTermCol2Right = MAX<int16>(_screen->_agiTermCol2Right, rect.left + _text16->agiWidth(text, true));
			}
		}
		if (agiTermPrompt) {
			// typing starts one terminal space after the prompt's visible text
			_screen->_agiTermX.clear();
			GfxFont *orig = _cache->getFont(7);
			_screen->_agiTermNext = rect.left + _text16->agiWidth(text, true) + (orig ? orig->getCharWidth(' ') : 6);
			_screen->_agiTermStart = _screen->_agiTermNext;
			_screen->_agiTermFirstScriptX = -1;
			_screen->_agiTermPromptGeneration = _screen->_agiPicGeneration;
		} else if (agiTermTypedX >= 0 && text[0] && text[0] != ' ') {
			// a typed character: the next one follows it, tightly spaced like the rest of the screen
			_screen->_agiTermX[agiTermTypedX] = rect.left;
			_screen->_agiTermNext = rect.left + MAX<int16>(_text16->agiWidth(text, false), 6);
			_screen->_agiTermX[agiTermTypedX + 6] = _screen->_agiTermNext;
		}
	}
	// Note: This code has been found in SCI1 middle and newer games. It was
	// previously only for SCI1 late and newer, but the LSL1 interpreter contains
	// this code.
	if (getSciVersion() >= SCI_VERSION_1_MIDDLE) {
		int16 leftPos = rect.right <= _screen->getWidth() ? 0 : _screen->getWidth() - rect.right;
		int16 topPos = rect.bottom <= _screen->getHeight() ? 0 : _screen->getHeight() - rect.bottom;
		_ports->move(leftPos, topPos);
		rect.moveTo(_ports->getPort()->curLeft, _ports->getPort()->curTop);
	}

	// AGI demake, SQ3 navigation screen (room 19): text written straight onto the star map gets a
	// black outline round every letter, rounded out to whole fat pixels, so it reads over the purple
	// grid. The saved area grows to take the outline (2 pixels each side, 1 above and below), so
	// erasing the text takes the outline with it
	const bool sq3Outline = _screen->agiDemake() && g_sci->getGameId() == GID_SQ3 && g_sci->getEngineState()->currentRoomNumber() == 19 && !stroke;
	const Common::Rect textRect = rect;
	if (sq3Outline) {
		rect.left -= 2;
		rect.right += 2;
		rect.top -= 1;
		rect.bottom += 1;
	}

	// Kawa's SCI11+
	if (stroke)
		rect.grow(1);

	if (doSaveUnder)
		result = bitsSave(rect, GFX_SCREEN_MASK_VISUAL);
	if (colorBack != -1)
		fillRect(rect, GFX_SCREEN_MASK_VISUAL, colorBack, 0, 0);

	// Kawa's SCI11+
	if (stroke)	{
		_ports->penColor(0);
		rect.translate(1, 0); if (stroke & 1) _text16->Box(text, languageSplitter, false, rect, alignment, -1); // right
		rect.translate(0, 1); if (stroke & 2) _text16->Box(text, languageSplitter, false, rect, alignment, -1); // bottom right
		rect.translate(-1, 0); if (stroke & 4) _text16->Box(text, languageSplitter, false, rect, alignment, -1); // bottom
		rect.translate(-1, 0); if (stroke & 8) _text16->Box(text, languageSplitter, false, rect, alignment, -1); // bottom left
		rect.translate(0, -1); if (stroke & 16) _text16->Box(text, languageSplitter, false, rect, alignment, -1); // left
		rect.translate(0, -1); if (stroke & 32) _text16->Box(text, languageSplitter, false, rect, alignment, -1); // top left
		rect.translate(1, 0); if (stroke & 64) _text16->Box(text, languageSplitter, false, rect, alignment, -1); // top
		rect.translate(1, 0); if (stroke & 128) _text16->Box(text, languageSplitter, false, rect, alignment, -1); // top right
		rect.translate(-1, 1); // and back to center
		_ports->penColor(colorPen);
	}

	// To make sure that the hires font used by PQ2 PC-98 and by the Korean fan translations does not get overdrawn we update the
	// display area before printing the text. The other (non-PQ2) PC-98 versions use a lowres font here, so this fix is only for
	// PQ2 PC-98 and for the Korean fan translations.
	bool needCJKFix = (g_sci->getLanguage() == Common::KO_KOR || (g_sci->getPlatform() == Common::kPlatformPC98 && g_sci->getGameId() == GID_PQ2));
	if (needCJKFix && !_screen->_picNotValid && bRedraw)
		bitsShow(rect);

	if (sq3Menu) {
		for (int i = 0; i < (int)ARRAYSIZE(kMenu); ++i) {
			const int16 y = rect.top + 8 * i;
			_text16->Box(kMenu[i].name, languageSplitter, false, Common::Rect(rect.left + 4, y, rect.left + 300, y + 8),
				SCI_TEXT16_ALIGNMENT_LEFT, -1);
			if (kMenu[i].price)
				_text16->Box(kMenu[i].price, languageSplitter, false, Common::Rect(rect.left + 200, y, rect.left + 290, y + 8),
					SCI_TEXT16_ALIGNMENT_RIGHT, -1);
		}
	} else if (sq3AstroInstr) {
		static const struct { const char *label; byte colour; } kLabels[] = {
			{ "Controls:", 12 }, { "Left arrow:", 14 }, { "Down arrow:", 14 }, { "Right arrow:", 14 }, { "Up arrow:", 14 }, { "Feed:", 12 }
		};
		int16 y = rect.top;
		for (int i = 0; i < (int)ARRAYSIZE(kAstroInstr); ++i) {
			if (i == 6 || i == 7 || i == 9 || i == 11)	// a gap before Feed, and each of the three notes
				y += 3;
			const Common::Rect line(rect.left, y, rect.right, y + 8);
			_text16->Box(kAstroInstr[i], languageSplitter, false, line, SCI_TEXT16_ALIGNMENT_LEFT, -1);
			if (colorPen != 0)
				for (uint l = 0; l < ARRAYSIZE(kLabels); ++l)
					if (!strncmp(kAstroInstr[i], kLabels[l].label, strlen(kLabels[l].label))) {
						_ports->penColor(kLabels[l].colour);
						_text16->Box(kLabels[l].label, languageSplitter, false, line, SCI_TEXT16_ALIGNMENT_LEFT, -1);
						_ports->penColor(colorPen);
					}
			y += 8;
		}
	} else if (sq3Wanted) {
		Common::Rect top(rect.left, rect.top, rect.right, rect.top + 16), bottom(rect.left, rect.top + 20, rect.right, rect.top + 36);
		_text16->Box("GNIDNEV ROF DETNAW\nDUARF ENIHCAM", languageSplitter, needCJKFix, top, alignment, -1);
		_text16->Box(":FFITNIALP\nYTLEVON DIOZAPPIG", languageSplitter, needCJKFix, bottom, alignment, -1);
	} else
		_text16->Box(text, languageSplitter, needCJKFix, sq3Outline ? textRect : rect, alignment, -1);
	if (sq3Outline) {
		Common::Rect scr = rect;
		_ports->offsetRect(scr);
		scr.clip(Common::Rect(0, 0, _screen->getWidth(), _screen->getHeight()));
		const int w = scr.width(), h = scr.height();
		if (w > 0 && h > 0) {
			Common::Array<byte> ink(w * h, 0), out(w * h, 0);
			for (int y = 0; y < h; ++y)
				for (int x = 0; x < w; ++x)
					ink[y * w + x] = (_screen->getDemakeFlag(scr.left + x, scr.top + y) == 1 &&
						_screen->getVisual(scr.left + x, scr.top + y) == (byte)colorPen) ? 1 : 0;
			for (int y = 0; y < h; ++y)
				for (int x = 0; x < w; ++x) {
					if (!ink[y * w + x])
						continue;
					for (int dy = -1; dy <= 1; ++dy)
						for (int dx = -1; dx <= 1; ++dx) {
							const int ox = x + dx, oy = y + dy;
							if (ox < 0 || oy < 0 || ox >= w || oy >= h || ink[oy * w + ox])
								continue;
							// round out to the whole fat pixel (screen columns 2n, 2n+1)
							const int sx = scr.left + ox;
							for (int px = sx & ~1; px <= (sx | 1); ++px) {
								const int lx = px - scr.left;
								if (lx >= 0 && lx < w && !ink[oy * w + lx])
									out[oy * w + lx] = 1;
							}
						}
				}
			for (int y = 0; y < h; ++y)
				for (int x = 0; x < w; ++x)
					if (out[y * w + x])
						_screen->putFontPixel(scr.top + y, scr.left + x, 0, 0);
		}
	}

	// See comment above.
	if (!needCJKFix && _screen->_picNotValid == 0 && bRedraw)
		bitsShow(rect);

	// QfG1 spells list: the second word of a long spell name, a line down and indented 12 pixels
	if (!agiSecondLine.empty()) {
		Common::Rect rect2;
		_text16->Size(rect2, agiSecondLine.c_str(), languageSplitter, -1, 0);
		rect2.moveTo(rect.left + 12, rect.top + 9);
		if (colorBack != -1)
			fillRect(rect2, GFX_SCREEN_MASK_VISUAL, colorBack, 0, 0);
		_text16->Box(agiSecondLine.c_str(), languageSplitter, false, rect2, alignment, -1);
		if (_screen->_picNotValid == 0 && bRedraw)
			bitsShow(rect2);
	}

	// restoring port and cursor pos
	Port *currport = _ports->getPort();
	uint16 tTop = currport->curTop;
	uint16 tLeft = currport->curLeft;
	if (!g_sci->_features->usesOldGfxFunctions()) {
		// Restore port settings for some SCI0LATE and SCI01+ only.
		//
		// The change actually happened inbetween .530 (hoyle1) and .566 (heros
		// quest). We don't have any detection for that currently, so we are
		// using oldGfxFunctions (.502). The only games that could get
		// regressions because of this are hoyle1, kq4 and funseeker. If there
		// are regressions, we should use interpreter version (which would
		// require exe version detection).
		//
		// If we restore the port for whole SCI0LATE, at least sq3old will get
		// an issue - font 0 will get used when scanning for planets instead of
		// font 600 - a setfont parameter is missing in one of the kDisplay
		// calls in script 19. I assume this is a script bug, because it was
		// added in sq3new.
		*currport = oldPort;
	}
	currport->curTop = tTop;
	currport->curLeft = tLeft;
	return result;
}

reg_t GfxPaint16::kernelPortraitLoad(const Common::String &resourceName) {
	//Portrait *myPortrait = new Portrait(_resMan, _screen, _palette, resourceName);
	return NULL_REG;
}

void GfxPaint16::kernelPortraitShow(const Common::String &resourceName, Common::Point position, uint16 resourceId, uint16 noun, uint16 verb, uint16 cond, uint16 seq) {
	Portrait *myPortrait = new Portrait(_resMan, g_sci->getEventManager(), _screen, _palette, _audio, resourceName);
	// TODO: cache portraits
	// adjust given coordinates to curPort (but dont adjust coordinates on upscaledHires_Save_Box and give us hires coordinates
	//  on kDrawCel, yeah this whole stuff makes sense)
	position.x += _ports->getPort()->left; position.y += _ports->getPort()->top;
	myPortrait->doit(position, resourceId, noun, verb, cond, seq);
	delete myPortrait;
}

void GfxPaint16::kernelPortraitUnload(uint16 portraitId) {
}

void GfxPaint16::removeHiresDrawObject(reg_t handle) {
	for (HiresDrawData *i = _hiresDrawObjs; i; ) {
		HiresDrawData *next = i->next;
		if (i->handle != handle) {
			i = next;
			continue;
		}

		// WORKAROUND for vertically misplaced hires portraits in mixed speech+text mode in KQ6CD. If we have
		// an entry which is flagged as needing a workaround, we set the notification for bitsShow() here.
		if (i->waFlag)
			_hiresPortraitWorkaroundFlag = true;

		// Unlink and delete entry
		if (i->next)
			i->next->prev = i->prev;
		if (i->prev)
			i->prev->next = i->next;
		else
			_hiresDrawObjs = i->next;
		delete i;

		i = next;
	}
}

bool GfxPaint16::hasHiresDrawObjectAt(uint16 x, uint16 y) const {
	for (HiresDrawData *i = _hiresDrawObjs; i; i = i->next) {
		if (i->leftPos == x && i->topPos == y)
			return true;
	}
	return false;
}

Common::Rect GfxPaint16::makeHiresRect(Common::Rect &rect) const {
	Common::Point topLeft(rect.left, rect.top);
	Common::Point bottomRight(rect.right, rect.bottom);
	topLeft = _screen->gfxDriver()->getRealCoords(topLeft);
	bottomRight = _screen->gfxDriver()->getRealCoords(bottomRight);
	return Common::Rect(topLeft.x, topLeft.y, bottomRight.x, bottomRight.y);
}

} // End of namespace Sci
