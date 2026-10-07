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
#include "common/hashmap.h"
#include "common/file.h"
#include "common/util.h"
#include "common/stack.h"
#include "common/unicode-bidi.h"
#include "graphics/font.h"
#include "graphics/primitives.h"

#include "sci/sci.h"
#include "sci/engine/features.h"
#include "sci/engine/state.h"
#include "sci/graphics/cache.h"
#include "sci/graphics/coordadjuster.h"
#include "sci/graphics/macfont.h"
#include "sci/graphics/ports.h"
#include "sci/graphics/paint16.h"
#include "sci/graphics/scifont.h"
#include "sci/graphics/screen.h"
#include "sci/graphics/text16.h"

namespace Sci {

GfxText16::GfxText16(GfxCache *cache, GfxPorts *ports, GfxPaint16 *paint16, GfxScreen *screen, GfxMacFontManager *macFontManager)
	: _cache(cache), _ports(ports), _paint16(paint16), _screen(screen), _macFontManager(macFontManager) {
	init();
}

GfxText16::~GfxText16() {
	delete[] _codeFonts;
	delete[] _codeColors;
	delete _agiFont;
}

void GfxText16::init() {
	_font = nullptr;
	_agiFont = nullptr;
	_agiTight = false;
	_agiSpace = 4;
	_agiFixedSpace = false;
	_codeFonts = nullptr;
	_codeFontsCount = 0;
	_codeColors = nullptr;
	_codeColorsCount = 0;
	_useEarlyGetLongestTextCalculations = g_sci->_features->useEarlyGetLongestTextCalculations();
}

GuiResourceId GfxText16::GetFontId() {
	return _ports->_curPort->fontId;
}

// AGI demake: every SCI font is replaced by the 8x8 PC BIOS font, which is what ScummVM's
// own AGI engine uses for DOS games when no Sierra font file is supplied
GfxFont *GfxText16::agiFont(GuiResourceId fontId) {
	if (!_agiFont)
		_agiFont = new GfxFontAgi(_screen);
	_agiFont->setFallback(_cache->getFont(fontId));
	_agiFont->setTight(_agiTight);
	_agiFont->setSpaceWidth(_agiSpace);
	return _agiFont;
}

int16 GfxText16::agiWidth(const char *text, bool trimTrailingSpaces) {
	GfxFont *f = GetFont();
	int16 w = 0, trimmed = 0;
	for (const char *p = text; *p; ++p) {
		w += f->getCharWidth((byte)*p);
		if (*p != ' ')
			trimmed = w;
	}
	return trimTrailingSpaces ? trimmed : w;
}

GfxFont *GfxText16::GetFont() {
	if (_screen->agiDemake()) {
		_font = agiFont(_ports->_curPort->fontId);
		_ports->_curPort->fontHeight = _font->getHeight();
		return _font;
	}
	if ((_font == nullptr) || (_font->getResourceId() != _ports->_curPort->fontId))
		_font = _cache->getFont(_ports->_curPort->fontId);

	return _font;
}

void GfxText16::SetFont(GuiResourceId fontId) {
	if (_screen->agiDemake()) {
		_font = agiFont(fontId);
		_ports->_curPort->fontId = fontId;
		_ports->_curPort->fontHeight = _font->getHeight();
		return;
	}
	if ((_font == nullptr) || (_font->getResourceId() != fontId))
		_font = _cache->getFont(fontId);

	_ports->_curPort->fontId = _font->getResourceId();
	_ports->_curPort->fontHeight = _font->getHeight();
}

#if 0
void GfxText16::ClearChar(int16 chr) {
	if (_ports->_curPort->penMode != 1)
		return;
	Common::Rect rect;
	rect.top = _ports->_curPort->curTop;
	rect.bottom = rect.top + _ports->_curPort->fontHeight;
	rect.left = _ports->_curPort->curLeft;
	rect.right = rect.left + GetFont()->getCharWidth(chr);
	_paint16->eraseRect(rect);
}
#endif

// This internal function gets called as soon as a '|' is found in a text. It
// will process the encountered code and set new font/set color.
// Returns textcode character count.
int16 GfxText16::CodeProcessing(const char *&text, GuiResourceId orgFontId, int16 orgPenColor, bool doingDrawing) {
	// Find the end of the textcode
	const char *textCode = text;
	int16 textCodeSize = 1;
	while ((*text != 0) && (*text++ != 0x7C)) {
		textCodeSize++;
	}

	// possible TextCodes:
	//  c -> sets textColor to current port pen color
	//  cX -> sets textColor to _textColors[X-1]
	char curCode = textCode[0];
	signed char curCodeParm = strtol(textCode+1, nullptr, 10);
	if (!Common::isDigit(textCode[1])) {
		curCodeParm = -1;
	}
	switch (curCode) {
	case 'c': // set text color
		if (curCodeParm == -1) {
			_ports->_curPort->penClr = orgPenColor;
		} else {
			if (curCodeParm < _codeColorsCount) {
				_ports->_curPort->penClr = _codeColors[curCodeParm];
			}
		}
		break;
	case 'f': // set text font
		if (curCodeParm == -1) {
			SetFont(orgFontId);
		} else {
			if (curCodeParm < _codeFontsCount) {
				SetFont(_codeFonts[curCodeParm]);
			}
		}
		break;
	case 'r': // reference (used in pepper)
		if (doingDrawing) {
			if (_codeRefTempRect.top == -1) {
				// Starting point
				_codeRefTempRect.top = _ports->_curPort->curTop;
				_codeRefTempRect.left = _ports->_curPort->curLeft;
			} else {
				// End point reached
				_codeRefTempRect.bottom = _ports->_curPort->curTop + _ports->_curPort->fontHeight;
				_codeRefTempRect.right = _ports->_curPort->curLeft;
				_codeRefRects.push_back(_codeRefTempRect);
				_codeRefTempRect.left = _codeRefTempRect.top = -1;
			}
		}
		break;
	default:
		break;
	}
	return textCodeSize;
}

// Has actually punctuation and characters in it, that may not be the first in a line
// SCI1 didn't check for exclamation nor question marks, us checking for those too shouldn't be bad
static const uint16 text16_shiftJIS_punctuation[] = {
	0x4181,	0x4281, 0x7681, 0x7881, 0x4981, 0x4881, 0
};

// Table from Quest for Glory 1 PC-98 (SCI01)
// has pronunciation and small combining form characters on top (details right after this table)
static const uint16 text16_shiftJIS_punctuation_SCI01[] = {
	0x9F82, 0xA182, 0xA382, 0xA582, 0xA782, 0xC182, 0xE182, 0xE382, 0xE582, 0xEC82,	0x4083, 0x4283,
	0x4483, 0x4683, 0x4883, 0x6283, 0x8383, 0x8583, 0x8783, 0x8E83, 0x9583, 0x9683,	0x5B81, 0x4181,
	0x4281, 0x7681, 0x7881, 0x4981, 0x4881, 0
};

// Police Quest 2 (SCI0) only checked for: 0x4181, 0x4281, 0x7681, 0x7881, 0x4981, 0x4881
// Castle of Dr. Brain/King's Quest 5/Space Quest 4 (SCI1) only checked for: 0x4181, 0x4281, 0x7681, 0x7881

// SCI0/SCI01/SCI1:
// 0x4181 -> comma,                 0x4281 -> period / full stop
// 0x7681 -> ending quotation mark, 0x7881 -> secondary quotation mark

// SCI0/SCI01:
// 0x4981 -> exclamation mark,      0x4881 -> question mark

// SCI01 (Quest for Glory only):
// 0x9F82, 0xA182, 0xA382, 0xA582, 0xA782 -> specifies vowel part of prev. hiragana char or pronunciation/extension of vowel
// 0xC182 -> pronunciation
// 0xE182, 0xE382, 0xE582, 0xEC82 -> small combining form of hiragana
// 0x4083, 0x4283, 0x4483, 0x4683, 0x4883 -> small combining form of katagana
// 0x6283 -> glottal stop / sokuon
// 0x8383, 0x8583 0x8783, 0x8E83 -> small combining form of katagana
// 0x9583 -> combining form
// 0x9683 -> abbreviation for the kanji (ka), the counter for months, places or provisions
// 0x5b81 -> low line / underscore (full width)


// return max # of chars to fit maxwidth with full words, does not include
// breaking space
//  Also adjusts text pointer to the new position for the caller
//
// Special cases in games:
//  Laura Bow 2 - Credits in the game menu - all the text lines start with spaces (bug #5159)
//                Act 6 Coroner questionnaire - the text of all control buttons has trailing spaces
//                                              "Detective Ryan Hanrahan O'Riley" contains even more spaces (bug #5334)
//  Conquests of the Longbow - talking with Lobb - one text box of the dialogue contains a longer word,
//                                                 that will be broken into 2 lines (bug #5159)
int16 GfxText16::GetLongest(const char *&textPtr, int16 maxWidth, GuiResourceId orgFontId) {
	uint16 curChar = 0;
	const char *textStartPtr = textPtr;
	const char *lastSpacePtr = nullptr;
	int16 lastSpaceCharCount = 0;
	int16 curCharCount = 0, resultCharCount = 0;
	uint16 curWidth = 0, tempWidth = 0;
	GuiResourceId previousFontId = GetFontId();
	int16 previousPenColor = _ports->_curPort->penClr;
	bool escapedNewLine = false;

	GetFont();
	if (!_font)
		return 0;

	for (;;) {
		curChar = (*(const byte *)textPtr);
		if (_font->isDoubleByte(curChar)) {
			curChar |= (*(const byte *)(textPtr + 1)) << 8;
		} else if (escapedNewLine) {
			escapedNewLine = false;
			curChar = 0x0D;
		} else if (curChar && isJapaneseNewLine(curChar, *(textPtr + 1))) {
			escapedNewLine = true;
			curChar = ' ';
		}

		switch (curChar) {
		case 0x7C:
			if (getSciVersion() >= SCI_VERSION_1_1) {
				curCharCount++; textPtr++;
				curCharCount += CodeProcessing(textPtr, orgFontId, previousPenColor, false);
				continue;
			}
			break;

		// We need to add 0xD, 0xA and 0xD 0xA to curCharCount and then exit
		//  which means, we split text like for example
		//  - 'Mature, experienced software analyst available.' 0xD 0xA
		//    'Bug installation a proven speciality. "No version too clean."' (normal game text, this is from lsl2)
		//  - 0xA '-------' 0xA (which is the official sierra subtitle separator) (found in multilingual versions)
		//  Sierra did it the same way.
		case 0xD:
			// Check, if 0xA is following, if so include it as well
			if ((*(const byte *)(textPtr + 1)) == 0xA) {
				curCharCount++; textPtr++;
			}
			// 'J'
			// fall through
		case 0xA:
		case 0x9781: // this one is used by SQ4/japanese as line break as well (was added for SCI1/PC98)
			curCharCount++; textPtr++;
			if (curChar > 0xFF) {
				// skip another byte in case char is double-byte (PC-98)
				curCharCount++; textPtr++;
			}
			// fall through
		case 0:
			SetFont(previousFontId);
			_ports->penColor(previousPenColor);
			return curCharCount;

		case ' ':
			lastSpaceCharCount = curCharCount; // return count up to (but not including) breaking space
			lastSpacePtr = textPtr + 1; // remember position right after the current space
			break;

		default:
			break;
		}
		tempWidth += _font->getCharWidth(curChar);

		// Width is too large? -> break out
		if (tempWidth > maxWidth)
			break;

		// the previous greater than test was originally a greater than or equals when
		//  no space character had been reached yet
		// AGI demake: skipped. With the fixed 8 pixel font, text boxes are often exactly as wide as
		// their longest line, and this early break then leaves that line's newline for a line of its
		// own, adding a blank line that pushes the last line out of the box (PQ2 desk passwords)
		if (_useEarlyGetLongestTextCalculations && !_screen->agiDemake()) {
			if (lastSpaceCharCount == 0 && tempWidth == maxWidth) {
				break;
			}
		}

		// still fits, remember width
		curWidth = tempWidth;

		// go to next character
		curCharCount++; textPtr++;
		if (curChar > 0xFF) {
			// Double-Byte
			curCharCount++; textPtr++;
		 }
	}

	if (lastSpaceCharCount) {
		// Break and at least one space was found before that
		resultCharCount = lastSpaceCharCount;

		// additionally skip over all spaces, that are following that space, but don't count them for displaying purposes
		textPtr = lastSpacePtr;
		while (*textPtr == ' ')
			textPtr++;

	} else {
		// Break without spaces found, we split the very first word - may also be Kanji/Japanese

		if (curChar > 0xFF) {
			// current character is Japanese

			// PC-9801 SCI actually added the last character, which shouldn't fit anymore, still onto the
			//  screen in case maxWidth wasn't fully reached with the last character
			if (( maxWidth - 1 ) > curWidth) {
				curCharCount += 2; textPtr += 2;

				curChar = (*(const byte *)textPtr);
				if (_font->isDoubleByte(curChar)) {
					curChar |= (*(const byte *)(textPtr + 1)) << 8;
				}
			}

			// But it also checked, if the current character is not inside a punctuation table and it even
			//  went backwards in case it found multiple ones inside that table.
			// Note: PQ2 PC-98 only went back 1 character and not multiple ones
			uint nonBreakingPos = 0;

			const uint16 *punctuationTable;

			if (getSciVersion() != SCI_VERSION_01) {
				punctuationTable = text16_shiftJIS_punctuation;
			} else {
				// Quest for Glory 1 PC-98 only
				punctuationTable = text16_shiftJIS_punctuation_SCI01;
			}

			for (;;) {
				// Look up if character shouldn't be the first on a new line
				nonBreakingPos = 0;
				while (punctuationTable[nonBreakingPos]) {
					if (punctuationTable[nonBreakingPos] == curChar)
						break;
					nonBreakingPos++;
				}
				if (!punctuationTable[nonBreakingPos]) {
					// character is fine
					break;
				}
				// Character is not acceptable, seek backward in the text
				curCharCount -= 2; textPtr -= 2;
				if (textPtr < textStartPtr)
					error("Seeking back went too far, data corruption?");

				curChar = (*(const byte *)textPtr);
				if (!_font->isDoubleByte(curChar))
					error("Non double byte while seeking back");
				curChar |= (*(const byte *)(textPtr + 1)) << 8;
			}

			if (curChar == 0x4081) {
				// Skip over alphabetic double-byte space
				// This was introduced for SCI1
				// Happens in Castle of Dr. Brain PC-98 in room 120, when looking inside the mirror
				// (game mentions Mixed Up Fairy Tales and uses English letters for that)
				textPtr += 2;
			}
		} else {
			// Add a character to the count for games whose interpreter would count the
			//  character that exceeded the width if a space hadn't been reached yet.
			//  Fixes #10000 where the notebook in LB1 room 786 displays "INCOMPLETE" with
			//  a width that's too short which would have otherwise wrapped the last "E".
			if (_useEarlyGetLongestTextCalculations) {
				curCharCount++; textPtr++;
			}
		}

		// We split the word in that case
		resultCharCount = curCharCount;
	}
	SetFont(previousFontId);
	_ports->penColor(previousPenColor);
	return resultCharCount;
}

void GfxText16::Width(const char *text, int16 from, int16 len, GuiResourceId orgFontId, int16 &textWidth, int16 &textHeight, bool restoreFont) {
	GuiResourceId previousFontId = GetFontId();
	int16 previousPenColor = _ports->_curPort->penClr;

	textWidth = 0;
	textHeight = 0;

	GetFont();
	if (_font) {
		bool escapedNewLine = false;
		text += from;
		while (len--) {
			uint16 curChar = (*(const byte *)text++);
			if (_font->isDoubleByte(curChar)) {
				curChar |= (*(const byte *)text++) << 8;
				len--;
			} else if (escapedNewLine) {
				escapedNewLine = false;
				curChar = 0x0D;
			} else if (curChar && isJapaneseNewLine(curChar, *text)) {
				escapedNewLine = true;
				curChar = ' ';
			}

			switch (curChar) {
			case 0x0A:
			case 0x0D:
			case 0x9781: // this one is used by SQ4/japanese as line break as well
				textHeight = MAX<int16> (textHeight, _ports->_curPort->fontHeight);
				break;
			case 0x7C:
				// pipe character is a control character in SCI1.1, otherwise treat as normal
				if (getSciVersion() >= SCI_VERSION_1_1) {
					len -= CodeProcessing(text, orgFontId, 0, false);
					break;
				}
				// fall through
			default:
				textHeight = MAX<int16> (textHeight, _ports->_curPort->fontHeight);
				textWidth += _font->getCharWidth(curChar);
			}
		}
	}
	// When calculating size, we do not restore font because we need the current (code modified) font active
	//  If we are drawing this is called inbetween, so font needs to get restored
	//  If we are calculating size of just one fixed string (::StringWidth), then we need to restore
	if (restoreFont) {
		SetFont(previousFontId);
		_ports->penColor(previousPenColor);
	}
	return;
}

void GfxText16::StringWidth(const Common::String &str, GuiResourceId orgFontId, int16 &textWidth, int16 &textHeight) {
	Width(str.c_str(), 0, str.size(), orgFontId, textWidth, textHeight, true);
}

#if 0
void GfxText16::ShowString(const Common::String &str, GuiResourceId orgFontId, int16 orgPenColor) {
	Show(str.c_str(), 0, str.size(), orgFontId, orgPenColor);
}
#endif

void GfxText16::DrawString(const Common::String &str, GuiResourceId orgFontId, int16 orgPenColor) {
	Draw(str.c_str(), 0, str.size(), orgFontId, orgPenColor);
}

int16 GfxText16::Size(Common::Rect &rect, const char *text, uint16 languageSplitter, GuiResourceId fontId, int16 maxWidth) {
	GuiResourceId previousFontId = GetFontId();
	int16 previousPenColor = _ports->_curPort->penClr;
	int16 maxTextWidth = 0, textWidth;
	int16 textHeight;

	if (fontId != -1)
		SetFont(fontId);
	else
		fontId = previousFontId;

	rect.top = rect.left = 0;

	if (maxWidth < 0) { // force output as single line
		if (g_sci->getLanguage() == Common::KO_KOR)
			SwitchToFont1001OnKorean(text, languageSplitter);
		if (g_sci->getLanguage() == Common::JA_JPN)
			SwitchToFont900OnSjis(text, languageSplitter);

		StringWidth(text, fontId, textWidth, textHeight);
		rect.bottom = textHeight;
		rect.right = textWidth;
	} else {
		// rect.right=found widest line with RTextWidth and GetLongest
		// rect.bottom=num. lines * GetPointSize
		rect.right = (maxWidth ? maxWidth : 192);
		const char *curTextPos = text; // in work position for GetLongest()
		const char *curTextLine = text; // starting point of current line

		// Check for Korean text
		if (g_sci->getLanguage() == Common::KO_KOR)
			SwitchToFont1001OnKorean(curTextPos, languageSplitter);

		int16 totalHeight = 0;
		while (*curTextPos) {
			// We need to check for Shift-JIS every line
			if (g_sci->getLanguage() == Common::JA_JPN)
				SwitchToFont900OnSjis(curTextPos, languageSplitter);

			int16 charCount = GetLongest(curTextPos, rect.right, fontId);
			if (charCount == 0)
				break;
			Width(curTextLine, 0, charCount, fontId, textWidth, textHeight, false);
			maxTextWidth = MAX(textWidth, maxTextWidth);
			totalHeight += textHeight;
			curTextLine = curTextPos;
		}
		rect.bottom = totalHeight;
		rect.right = maxWidth ? maxWidth : MIN(rect.right, maxTextWidth);
	}
	SetFont(previousFontId);
	_ports->penColor(previousPenColor);
	return rect.right;
}

// returns maximum font height used
void GfxText16::Draw(const char *text, int16 from, int16 len, GuiResourceId orgFontId, int16 orgPenColor) {
	GetFont();
	if (!_font)
		return;

	Common::Rect rect;
	rect.top = _ports->_curPort->curTop;
	rect.bottom = rect.top + _ports->_curPort->fontHeight;
	text += from;
	bool escapedNewLine = false;
	while (len--) {
		uint16 curChar = (*(const byte *)text++);
		if (_font->isDoubleByte(curChar)) {
			curChar |= (*(const byte *)text++) << 8;
			len--;
		} else if (escapedNewLine) {
			escapedNewLine = false;
			curChar = 0x0D;
		} else if (curChar && isJapaneseNewLine(curChar, *text)) {
			escapedNewLine = true;
			curChar = ' ';
		}

		switch (curChar) {
		case 0x0A:
		case 0x0D:
		case 0:
		case 0x9781: // this one is used by SQ4/japanese as line break as well
			break;
		case 0x7C:
			// pipe character is a control character in SCI1.1, otherwise treat as normal
			if (getSciVersion() >= SCI_VERSION_1_1) {
				len -= CodeProcessing(text, orgFontId, orgPenColor, true);
				break;
			}
			// fall through
		default: {
			uint16 charWidth = _font->getCharWidth(curChar);
			// clear char
			if (_ports->_curPort->penMode == 1) {
				rect.left = _ports->_curPort->curLeft;
				rect.right = rect.left + charWidth;
				_paint16->eraseRect(rect);
			}
			// CharStd
			_font->draw(curChar, _ports->_curPort->top + _ports->_curPort->curTop, _ports->_curPort->left + _ports->_curPort->curLeft, _ports->_curPort->penClr, _ports->_curPort->greyedOutput);
			_ports->_curPort->curLeft += charWidth;
		}
		}
	}
}

void GfxText16::Show(const char *text, int16 from, int16 len, GuiResourceId orgFontId, int16 orgPenColor) {
	Common::Rect rect;

	rect.top = _ports->_curPort->curTop;
	rect.bottom = rect.top + _ports->getPointSize();
	rect.left = _ports->_curPort->curLeft;
	Draw(text, from, len, orgFontId, orgPenColor);
	rect.right = _ports->_curPort->curLeft;
	_paint16->bitsShow(rect);
}

// Draws a text in rect.
// Diagnostics for adding games: TEMP always on in test builds (normally "agi_demake_debug=true" in the game's section of
// scummvm.ini). Writes agi_demake_text.txt (every text box, plus PQ2 computer drawing) next to the exe.
static Common::DumpFile *g_agiDemakeTextLog = nullptr;
void agiDemakeLogLine(const Common::String &line) {
	// TEMP: always on for testing, remove before next public release
	if (!g_agiDemakeTextLog) {
		g_agiDemakeTextLog = new Common::DumpFile();
		if (!g_agiDemakeTextLog->open(Common::Path("agi_demake_text.txt"))) {
			delete g_agiDemakeTextLog;
			g_agiDemakeTextLog = nullptr;
			return;
		}
	}
	g_agiDemakeTextLog->writeString(line + "\n");
	g_agiDemakeTextLog->flush();
}

// Temporary diagnostics: every distinct text box drawn (text with control characters shown as
// <hex>, box rectangle, alignment, font, port), written next to scummvm.exe
static void agiDemakeLogText(const char *text, const Common::Rect &rect, int alignment, int fontId, const Common::Rect &port, int portFont) {
	static Common::DumpFile *file = nullptr;
	static Common::HashMap<Common::String, bool> seen;
	const int room = g_sci->getEngineState() ? g_sci->getEngineState()->currentRoomNumber() : -1;
	Common::String line = Common::String::format("room %d box %d,%d-%d,%d port %d,%d-%d,%d align %d font %d portfont %d: ",
		room, rect.left, rect.top, rect.right, rect.bottom, port.left, port.top, port.right, port.bottom, alignment, fontId, portFont);
	for (const char *c = text; *c; ++c) {
		const byte b = (byte)*c;
		if (b < 32 || b > 126)
			line += Common::String::format("<%02x>", b);
		else
			line += (char)b;
	}
	if (seen.contains(line))
		return;
	seen[line] = true;
	(void)file;
	agiDemakeLogLine(line);
}

// AGI demake: a text box that runs to the right edge of the screen has no real width limit, so
// the game relied on its own font to keep the text inside whatever is drawn behind it. Wrap such
// boxes no wider than the widest line the game's own font would have produced (same greedy word
// wrap at the box width), so the wider 8x8 text stays inside Sierra's layout.
int16 GfxText16::agiDemakeOriginalWidth(const char *text, int16 maxWidth, GuiResourceId fontId) {
	GfxFont *orig = _cache->getFont(fontId != -1 ? fontId : _ports->_curPort->fontId);
	if (!orig)
		return maxWidth;
	int16 widest = 0, cur = 0, atSpace = -1;
	for (const char *p = text; ; ++p) {
		const byte c = (byte)*p;
		if (c == 0 || c == 0x0A || c == 0x0D) {
			widest = MAX(widest, cur);
			cur = 0;
			atSpace = -1;
			if (c == 0)
				break;
			continue;
		}
		if (c == ' ')
			atSpace = cur;
		const int16 cw = orig->getCharWidth(c);
		if (cur + cw > maxWidth) {
			// the original would have broken the line here: at the last space, or mid-word
			widest = MAX<int16>(widest, (atSpace >= 0) ? atSpace : cur);
			cur = (atSpace >= 0) ? (cur - atSpace) : 0;
			atSpace = -1;
		}
		cur += cw;
	}
	return MIN(widest, maxWidth);
}

// AGI demake: game text replaced where the picture it describes can't carry the detail in AGI
// pixels. Used both when the game sizes the box (kTextSize) and when it draws it, so the box fits.
static const struct AgiDemakeTextSub {
	SciGameId game;
	const char *from, *to;
} kAgiDemakeTextSubs[] = {
	// PQ2 envelope corner: the name and address on it are too small for AGI pixels
	{ GID_PQ2, "Corner of an envelope with name and address.",
		"Corner of an envelope with name and address: \"Bill Cole, 753 Third St, Lytton City\"" },
	// PQ2 the envelope it was torn from
	{ GID_PQ2, "You find an envelope with the corner torn off.",
		"You find an envelope with the corner torn off. It's addressed to \"Woodrow Roberts, 5556 Oak St, Lytton City, USA\"" },
	// PQ2 the same envelope, looked at in the inventory
	{ GID_PQ2, "Envelope with corner torn off.",
		"Envelope with corner torn off. It's addressed to \"Woodrow Roberts, 5556 Oak St, Lytton City, USA\"" },
	// PQ2 Colby's business card
	{ GID_PQ2, "Colby's business card, found in Bains' motel room.",
		"Colby's business card, found in Bains' motel room: \"Colby Imports, Steelton, U.S.A., 407-555-3323\"" },
};

// AGI demake: part of a longer message changed, matched on a distinctive part of the message. Used where only
// a line needs to change and the rest must stay exactly as the game has it.
static const struct AgiDemakeTextEdit {
	SciGameId game;
	const char *start, *find, *replace;
} kAgiDemakeTextEdits[] = {
	// PQ2 bomb instructions: the WARNING line is indented like the rest, and broken by hand so the
	// part that wraps stays indented too
	{ GID_PQ2, "KUDOFI'S DO-IT-YOURSELF BOMB", "\nWARNING: Only connect same color wires.", "\n  WARNING: Only connect same color\n  wires." },
};

static const char *agiDemakeSubstituteText(const char *text) {
	if (!text)
		return text;
	for (uint i = 0; i < ARRAYSIZE(kAgiDemakeTextSubs); ++i)
		if (g_sci->getGameId() == kAgiDemakeTextSubs[i].game && !strcmp(text, kAgiDemakeTextSubs[i].from))
			return kAgiDemakeTextSubs[i].to;
	static Common::String edited;
	for (uint i = 0; i < ARRAYSIZE(kAgiDemakeTextEdits); ++i) {
		const AgiDemakeTextEdit &e = kAgiDemakeTextEdits[i];
		if (g_sci->getGameId() != e.game || !strstr(text, e.start))
			continue;
		const char *at = strstr(text, e.find);
		if (!at)
			continue;
		edited = Common::String(text, at - text) + e.replace + (at + strlen(e.find));
		return edited.c_str();
	}
	return text;
}

// AGI demake: messages whose box width is changed for the 8x8 font, matched on a distinctive part
// of the message. They are wrapped no wider than the given width when the game sizes their box.
static const struct AgiDemakeTextWidth {
	SciGameId game;
	const char *start;
	int16 maxWidth;
	bool force;		// true: this width whatever the game asks for (wider), false: no wider than this
} kAgiDemakeTextWidths[] = {
	// PQ2 reading the envelope beside its picture: at 192 (and 184) wide the window, with the AGI
	// frame, ran past the right edge of the screen
	{ GID_PQ2, "\"Bains mailed the letter hoping to suck Roberts in", 176, false },
	// PQ2 bomb instructions: sized for SCI's small font at 184 wide, which wraps nearly every line in
	// the 8x8 font. 296 is the widest that still fits on screen with the window frame
	{ GID_PQ2, "KUDOFI'S DO-IT-YOURSELF BOMB", 296, true },
};

static int16 agiDemakeCapTextWidth(const char *text, int16 maxWidth) {
	if (!text)
		return maxWidth;
	for (uint i = 0; i < ARRAYSIZE(kAgiDemakeTextWidths); ++i) {
		const AgiDemakeTextWidth &tw = kAgiDemakeTextWidths[i];
		if (g_sci->getGameId() == tw.game && strstr(text, tw.start) &&
				((tw.force && maxWidth >= 0) || maxWidth == 0 || maxWidth > tw.maxWidth))	// 0 means the default width (192), below 0 one line
			return tw.maxWidth;
	}
	return maxWidth;
}

void GfxText16::Box(const char *text, uint16 languageSplitter, bool show, const Common::Rect &rectIn, TextAlignment alignment, GuiResourceId fontId) {
	Common::Rect rect = rectIn;
	if (_screen->agiDemake()) {
		text = agiDemakeSubstituteText(text);
		agiDemakeLogText(text, rect, alignment, fontId, _ports->_curPort->rect, _ports->_curPort->fontId);
		if (_ports->_curPort->left + rect.right >= _screen->getWidth()) {
			// at least as wide as the left margin mirrored on the right (pages are centred), since
			// Sierra's line breaks are often hand-placed and only just fit their own font
			const int16 absLeft = _ports->_curPort->left + rect.left;
			const int16 mirrored = _screen->getWidth() - 2 * absLeft;
			const int16 w = MAX<int16>(agiDemakeOriginalWidth(text, rect.width(), fontId), mirrored);
			if (w > 0 && w < rect.width())
				rect.right = rect.left + w;
		}
	}
	int16 textWidth, maxTextWidth, textHeight;
	int16 offset = 0;
	int16 hline = 0;
	GuiResourceId previousFontId = GetFontId();
	int16 previousPenColor = _ports->_curPort->penClr;
	bool doubleByteMode = false;
	const char *curTextPos = text;
	const char *curTextLine = text;

	if (fontId != -1)
		SetFont(fontId);
	else
		fontId = previousFontId;

	// Check for Korean text
	if (g_sci->getLanguage() == Common::KO_KOR) {
		if (SwitchToFont1001OnKorean(curTextPos, languageSplitter)) {
			doubleByteMode = true;
			fontId = 1001;
		}
	}

	// Reset reference code rects
	_codeRefRects.clear();
	_codeRefTempRect.left = _codeRefTempRect.top = -1;

	maxTextWidth = 0;
	while (*curTextPos) {
		//  We need to check for Shift-JIS every line. Police Quest 2 PC-9801 often draws English + Japanese text into the same box.
		if (g_sci->getLanguage() == Common::JA_JPN) {
			doubleByteMode = SwitchToFont900OnSjis(curTextPos, languageSplitter) ||

			// Ignore leading space characters for PQ2 PC-9801, so as not to break the Japanese F1 help screen. These text lines
			// start with a normal (single-byte) space character which is followed by SJIS text.
			// 
			// The original PQ 2 PC-9801 interpreter's SJIS text drawing method is less sophisticated than what the other PC-9801
			// interpreters do. It doesn't even do this font change. It just keeps the English font and uses that to (wrongfully)
			// measure the width of the Japanese text. This appears to be the main reason why the text layout looks different in
			// the original and in ScummVM in some places (first obvious example: the copy protection dialogs; also, in the
			// Japanese F1 help box, the headline is not centered at the exact same position as with the original interpreter).
			// 
			// Also, the original PQ 2 PC-9801 interpreter doesn't need to prevent background graphics updates, since the SJIS
			// text is drawn in PC-9801 text mode and the text mode layer is always displayed on top of the graphics layer, so it
			// can never get corrupted by graphics updates (with an emulator you can see how even the mouse cursor is drawn under
			// the Japanese text).
			// 
			// In contrast to that, we do the font switching for PQ2 (we could eventually try to aim for a faithful text measuring
			// and positioning, but it is more complex than just dropping the font switching) and we also need to prevent graphics
			// updates for SJIS lines, since we don't emulate the PC-9801 text mode layer to that extent (could be done, but I
			// don't see why we should). So, here we ignore leading space characters and determine the ASCII or SJIS text line type
			// by the character after that...
				(g_sci->getGameId() == GID_PQ2 && *curTextPos == ' ' && curTextPos[1] != '\0' && SwitchToFont900OnSjis(curTextPos + 1, languageSplitter));
		}

		// AGI demake, tight spacing: a hand-broken line that is slightly too long gets narrower spaces
		// (3, then 2 pixels) if that saves it from wrapping
		if (_screen->agiDemake() && _agiTight && !_agiFixedSpace) {
			_agiSpace = 4;
			for (int16 sw = 4; sw >= 2; --sw) {
				_agiSpace = sw;
				GfxFont *f = GetFont();
				int16 lw = 0;
				for (const char *p = curTextPos; *p && *p != 0x0A && *p != 0x0D; ++p)
					lw += f->getCharWidth((byte)*p);
				if (lw <= rect.width())
					break;
				if (sw == 2)
					_agiSpace = 4;	// still too long: wrap normally
			}
			GetFont();
		}
		int16 charCount = GetLongest(curTextPos, rect.width(), fontId);
		if (charCount == 0)
			break;
		Width(curTextLine, 0, charCount, fontId, textWidth, textHeight, true);
		maxTextWidth = MAX<int16>(maxTextWidth, textWidth);
		switch (alignment) {
		case SCI_TEXT16_ALIGNMENT_RIGHT:
			if (!g_sci->isLanguageRTL())
				offset = rect.width() - textWidth;
			else
				offset = 0;
			break;
		case SCI_TEXT16_ALIGNMENT_CENTER:
			offset = (rect.width() - textWidth) / 2;
			break;
		case SCI_TEXT16_ALIGNMENT_LEFT:
			if (!g_sci->isLanguageRTL())
				offset = 0;
			else
				offset = rect.width() - textWidth;
			break;

		default:
			warning("Invalid alignment %d used in TextBox()", alignment);
		}


		if (g_sci->isLanguageRTL() && offset > 0)
			// In the game fonts, characters have spacing on the left, and no spacing on the right,
			// therefore, when we start drawing from the right, they "start from the border"
			// e.g., in SQ3 Hebrew user's input prompt.
			// We can't add spacing on the right of the Hebrew letters, because then characters in mixed
			// English-Hebrew text might be stuck together.
			// Therefore, we shift one pixel to the left, for proper spacing
			offset--;

		_ports->moveTo(rect.left + offset, rect.top + hline);

		Common::String textString;
		if (g_sci->isLanguageRTL()) {
			const char *curTextLineOrig = curTextLine;
			Common::String textLogical = Common::String(curTextLineOrig, (uint32)charCount);
			textString = Common::convertBiDiString(textLogical, g_sci->getLanguage());
			curTextLine = textString.c_str();
		}

		// This seems to be the method used by the original (non-PQ2) PC-9801 interpreters. They will set
		// the `show` argument (only) for the SCI_CONTROLS_TYPE_TEXT, but then there is a separate code
		// path for the SJIS characters, which will not get the screen surface update (since they get
		// rendered directly into the video memory). We handle PQ2 the same way as the other PC-9801 targets.
		if (show && !doubleByteMode) {
			Show(curTextLine, 0, charCount, fontId, previousPenColor);
		} else {
			Draw(curTextLine, 0, charCount, fontId, previousPenColor);
		}

		hline += textHeight;
		curTextLine = curTextPos;
	}
	SetFont(previousFontId);
	_ports->penColor(previousPenColor);
}

void GfxText16::DrawString(const Common::String &textOrig) {
	GuiResourceId previousFontId = GetFontId();
	int16 previousPenColor = _ports->_curPort->penClr;

	Common::String text;
	if (!g_sci->isLanguageRTL())
		text = textOrig;
	else
		text = Common::convertBiDiString(textOrig, g_sci->getLanguage());

	Draw(text.c_str(), 0, text.size(), previousFontId, previousPenColor);
	SetFont(previousFontId);
	_ports->penColor(previousPenColor);
}

// we need to have a separate status drawing code
//  In KQ4 the IV char is actually 0xA, which would otherwise get considered as linebreak and not printed
void GfxText16::DrawStatus(const Common::String &strOrig) {
	GetFont();
	if (!_font)
		return;

	Common::String str;
	if (!g_sci->isLanguageRTL())
		str = strOrig;
	else
		str = Common::convertBiDiString(strOrig, g_sci->getLanguage());

	const byte *text = (const byte *)str.c_str();
	uint16 textLen = str.size();
	Common::Rect rect;

	rect.top = _ports->_curPort->curTop;
	rect.bottom = rect.top + _ports->_curPort->fontHeight;
	while (textLen--) {
		uint16 curChar = *text++;
		switch (curChar) {
		case 0:
			break;
		default: {
			uint16 charWidth = _font->getCharWidth(curChar);
			_font->draw(curChar, _ports->_curPort->top + _ports->_curPort->curTop, _ports->_curPort->left + _ports->_curPort->curLeft, _ports->_curPort->penClr, _ports->_curPort->greyedOutput);
			_ports->_curPort->curLeft += charWidth;
		}
		}
	}
}

// Check for Korean strings, and use font 1001 to render them
bool GfxText16::SwitchToFont1001OnKorean(const char *text, uint16 languageSplitter) {
	const byte* ptr = (const byte *)text;
	if (languageSplitter != 0x6b23) { // #k prefix as language splitter
		// Check if the text contains at least one Korean character
		while (*ptr) {
			byte ch = *ptr++;
			if (ch >= 0xB0 && ch <= 0xC8) {
				ch = *ptr++;
				if (!ch)
					return false;

				if (ch >= 0xA1 && ch <= 0xFE) {
					SetFont(1001);
					return true;
				}
			}
		}
	}
	return false;
}

// Sierra did this in their PC98 interpreter only, they identify a text as being
// sjis and then switch to font 900
bool GfxText16::SwitchToFont900OnSjis(const char *text, uint16 languageSplitter) {
	byte firstChar = (*(const byte *)text++);
	if (languageSplitter != 0x6a23) { // #j prefix as language splitter
		if (((firstChar >= 0x81) && (firstChar <= 0x9F)) || ((firstChar >= 0xE0) && (firstChar <= 0xEF))) {
			SetFont(900);
			return true;
		}
	}
	return false;
}

// In PC-9801 SSCI, "\n", "\N", "\r" and "\R" were overwritten with SPACE + 0x0D
// inside GetLongest() (text16). This was a bit of a hack since it meant this
// version altered the input that it's normally only supposed to be measuring.
// Instead, we detect the newline sequences during string processing loops and
// apply the substitute characters on the fly. PQ2 is the only game known to use
// this feature. "\n" appears in most of its Japanese strings.
bool GfxText16::isJapaneseNewLine(int16 curChar, int16 nextChar) {
	return g_sci->getLanguage() == Common::JA_JPN &&
		curChar == '\\' && (nextChar == 'n' || nextChar == 'N' || nextChar == 'r' || nextChar == 'R');
}

reg_t GfxText16::allocAndFillReferenceRectArray() {
	uint rectCount = _codeRefRects.size();
	if (rectCount) {
		reg_t rectArray;
		byte *rectArrayPtr = g_sci->getEngineState()->_segMan->allocDynmem(4 * 2 * (rectCount + 1), "text code reference rects", &rectArray);
		GfxCoordAdjuster16 *coordAdjuster = g_sci->_gfxCoordAdjuster;
		for (uint curRect = 0; curRect < rectCount; curRect++) {
			coordAdjuster->kernelLocalToGlobal(_codeRefRects[curRect].left, _codeRefRects[curRect].top);
			coordAdjuster->kernelLocalToGlobal(_codeRefRects[curRect].right, _codeRefRects[curRect].bottom);
			WRITE_LE_UINT16(rectArrayPtr + 0, _codeRefRects[curRect].left);
			WRITE_LE_UINT16(rectArrayPtr + 2, _codeRefRects[curRect].top);
			WRITE_LE_UINT16(rectArrayPtr + 4, _codeRefRects[curRect].right);
			WRITE_LE_UINT16(rectArrayPtr + 6, _codeRefRects[curRect].bottom);
			rectArrayPtr += 8;
		}
		WRITE_LE_UINT16(rectArrayPtr + 0, 0x7777);
		WRITE_LE_UINT16(rectArrayPtr + 2, 0x7777);
		WRITE_LE_UINT16(rectArrayPtr + 4, 0x7777);
		WRITE_LE_UINT16(rectArrayPtr + 6, 0x7777);
		return rectArray;
	}
	return NULL_REG;
}

void GfxText16::kernelTextSize(const char *text, uint16 languageSplitter, int16 font, int16 maxWidth, int16 *textWidth, int16 *textHeight) {
	Common::Rect rect(0, 0, 0, 0);
	if (_screen->agiDemake()) {
		text = agiDemakeSubstituteText(text);
		maxWidth = agiDemakeCapTextWidth(text, maxWidth);
	}
	// AGI demake: text with hand-placed line breaks keeps them. If its longest line (trailing
	// spaces ignored) is wider than the game asked for in the 8x8 font, the box is made wide
	// enough for it, up to the screen width less a margin, instead of re-wrapping every line.
	if (_screen->agiDemake() && maxWidth > 0 && strchr(text, 0x0A)) {
		const GuiResourceId previousFontId = GetFontId();
		if (font != -1)
			SetFont(font);
		GfxFont *f = GetFont();
		int16 longest = 0, cur = 0, curTrimmed = 0;
		for (const char *p = text; ; ++p) {
			const byte c = (byte)*p;
			if (c == 0 || c == 0x0A || c == 0x0D) {
				longest = MAX(longest, curTrimmed);
				cur = curTrimmed = 0;
				if (c == 0)
					break;
				continue;
			}
			cur += f->getCharWidth(c);
			if (c != ' ')
				curTrimmed = cur;
		}
		SetFont(previousFontId);
		const int16 cap = _screen->getWidth() - 24;
		_screen->setAgiWindowShift(0, 0);
		if (longest > maxWidth) {
			Common::Rect original(0, 0, 0, 0);
			Size(original, text, languageSplitter, font, maxWidth);
			maxWidth = MIN(longest, cap);
			Common::Rect widened(0, 0, 0, 0);
			Size(widened, text, languageSplitter, font, maxWidth);
			if (widened.width() > original.width())
				_screen->setAgiWindowShift((widened.width() - original.width()) / 2, widened.width());
		}
	}
	Size(rect, text, languageSplitter, font, maxWidth);
	*textWidth = rect.width();
	*textHeight = rect.height();
}

// Used SCI1+ for text codes
void GfxText16::kernelTextFonts(int argc, reg_t *argv) {
	int i;

	delete[] _codeFonts;
	_codeFontsCount = argc;
	_codeFonts = new GuiResourceId[argc];
	for (i = 0; i < argc; i++) {
		_codeFonts[i] = (GuiResourceId)argv[i].toUint16();
	}
}

// Used SCI1+ for text codes
void GfxText16::kernelTextColors(int argc, reg_t *argv) {
	int i;

	delete[] _codeColors;
	_codeColorsCount = argc;
	_codeColors = new uint16[argc];
	for (i = 0; i < argc; i++) {
		_codeColors[i] = argv[i].toUint16();
	}
}

// This function is roughly equivalent to RTextSizeMac.
// (SCI1.1 Mac interpreters include function names.)
// The results of this function determine the size of SCI message boxes over
// which macDraw() is called later. Even though the Mac interpreter would use
// one of three font sizes for drawing, depending on the window size the user
// had selected, these calculations always use the small font. Otherwise, the
// size of the window would affect the size of the message box within the game,
// instead of just the text that was drawn within it.
void GfxText16::macTextSize(const Common::String &text, GuiResourceId sciFontId, GuiResourceId origSciFontId, int16 maxWidth, int16 *textWidth, int16 *textHeight) {
	if (sciFontId == -1) {
		sciFontId = origSciFontId;
	}

	// Always use the small font for calculating text size
	const Graphics::Font *font = _macFontManager->getSmallFont(sciFontId);

	// If maxWidth is negative then return the size of all characters on the same line
	if (maxWidth < 0) {
		*textWidth = 0;
		for (uint i = 0; i < text.size(); ++i) {
			*textWidth += font->getCharWidth(text[i]);
		}
		*textHeight = font->getFontAscent();
		return;
	}

	// Default max width is 193, otherwise increment the specified max
	maxWidth = (maxWidth == 0) ? 193 : (maxWidth + 1);

	// Build lists of lines and widths and calculate the largest line width.
	// The Mac interpreter did this by creating a hidden TEdit, settings its width
	// and text, and then querying TEdit's internal structures to count the lines
	// and find the largest. This means that Mac's own text wrapping algorithm
	// determined kTextResult results and the resulting message box sizes.
	// Due to the specifics of Mac's text-wrapping, it's possible for resulting
	// lines to be larger than maxWidth. See macGetLongest().
	Common::Array<Common::String> lines;
	Common::Array<int16> lineWidths;
	int16 maxLineCharCount = 0;
	int16 maxLineWidth = 0;
	int lineCount = 0;
	for (uint i = 0; i < text.size(); ++i) {
		int16 lineWidth;
		int16 lineCharCount = macGetLongest(text, i, font, maxWidth, &lineWidth);

		Common::String line;
		for (int16 j = 0; j < lineCharCount; ++j) {
			char ch = text[i + j];
			if (ch == '\r' || ch == '\n') {
				break;
			}
			if (ch == '\t') {
				ch = ' ';
			}
			line += ch;
		}
		lines.push_back(line);
		lineWidths.push_back(lineWidth);
		maxLineCharCount = MAX(lineCharCount, maxLineCharCount);

		if (lineCharCount == 0) {
			break;
		}
		maxLineWidth = MAX(lineWidth, maxLineWidth);
		lineCount++;
		i += (lineCharCount - 1);
	}

	// Mac TEdit line widths are 1 pixel wider than the sum of their character widths.
	// This extra pixel comes from the TEdit structure the Mac interpreter queries.
	*textWidth = maxLineWidth + 1;
	if (_macFontManager->usesSystemFonts()) {
		// QFG1VGA and LSL6 add another pixel to returned widths.
		*textWidth += 1;
	}

	// Mac TEdit height is the sum of font height and leading, which is space between lines.
	// Leading can be zero for fonts that have spacing embedded in their glyphs.
	*textHeight = lineCount * (font->getFontHeight() + font->getFontLeading());

	if (_macFontManager->usesSystemFonts() &&
		_screen->getUpscaledHires() == GFX_SCREEN_UPSCALED_640x400) {
		// QFG1VGA and LSL6 make this adjustment when the large font is used.
		*textHeight -= (lineCount + 1);
	}
}

// This function is roughly equivalent to RTextBoxMac.
// (SCI1.1 Mac interpreters include function names.)
// The main difference is that we draw each character ourselves.
// RTextBoxMac just created a TEdit with a transparent background, set its
// properties, and then had Mac render it on the window.
void GfxText16::macDraw(const Common::String &text, Common::Rect rect, TextAlignment alignment, GuiResourceId sciFontId, GuiResourceId origSciFontId, int16 color) {
	if (sciFontId == -1) {
		sciFontId = origSciFontId;
	}

	// Use the large font in hires mode, otherwise use the small font
	const Graphics::Font *font;
	uint16 scale;
	if (_screen->getUpscaledHires() == GFX_SCREEN_UPSCALED_640x400) {
		font = _macFontManager->getLargeFont(sciFontId);
		scale = 2;
	} else {
		font = _macFontManager->getSmallFont(sciFontId);
		scale = 1;
	}

	if (color == -1) {
		color = _ports->_curPort->penClr;
	}

	rect.left *= scale;
	rect.top *= scale;
	rect.right *= scale;
	rect.bottom *= scale;

	// Draw each line of text
	int16 maxWidth = rect.width();
	int16 y = (_ports->_curPort->top * scale) + rect.top;
	for (uint i = 0; i < text.size(); ++i) {
		int16 lineWidth;
		int16 lineCharCount = macGetLongest(text, i, font, maxWidth, &lineWidth);
		if (lineCharCount == 0) {
			break;
		}

		int16 offset = 0;
		if (alignment == SCI_TEXT16_ALIGNMENT_CENTER) {
			offset = (maxWidth - lineWidth) / 2;
		} else if (alignment == SCI_TEXT16_ALIGNMENT_RIGHT) {
			offset = maxWidth - lineWidth;
		}

		// Draw each character in the line
		int16 x = (_ports->_curPort->left * scale) + rect.left + offset;
		for (int16 j = 0; j < lineCharCount; ++j) {
			char ch = text[i + j];
			_screen->putMacChar(font, x, y, ch, color);
			x += font->getCharWidth(ch);
		}

		y += font->getFontHeight() + font->getFontLeading();
		i += (lineCharCount - 1);
	}
}

// This function mimics classic Mac's TEdit text wrapping behavior in a style
// similar to GfxText16::GetLongest() that we use for SCI text measurement.
// This implementation is based on black-box reverse engineering System 7.5.5
// behavior by inspecting the calculated TEdit widths and modding SCI games to
// display the results of kTextSize and altering their strings to test various
// inputs. It's possible that this Mac behavior was ROM or OS version dependent.
// In general, line width calculations work as one would expect, except for the
// oddity that space characters are applied to the current line and included
// in the calculated width even if that results in widths larger than maxWidth.
int16 GfxText16::macGetLongest(const Common::String &text, uint start, const Graphics::Font *font, int16 maxWidth, int16 *lineWidth) {
	*lineWidth = 0;
	int wordWidth = 0;
	int wordStart = start;
	char prevChar = '\0';
	for (uint i = start; i < text.size(); ++i) {
		char ch = text[i];
		int charWidth = font->getCharWidth(ch);
		if (ch == '\r') {
			*lineWidth += wordWidth;
			wordWidth = 0;
			// ignore \n that follows \r
			if (i + 1 < text.size() && text[i + 1] == '\n') {
				++i;
			}
			wordStart = i + 1;
			return wordStart - start;
		} else if (ch == '\n') {
			*lineWidth += wordWidth;
			wordWidth = 0;
			wordStart = i + 1;
			return wordStart - start;
		} else if (prevChar == ' ' && ch != ' ') {
			// start new word once a non-space is reached
			*lineWidth += wordWidth;
			wordWidth = charWidth;
			wordStart = i;
		} else {
			// add character to word width, including spaces
			wordWidth += charWidth;
		}

		// If the line plus the current width has reached maxWidth then we are done,
		// unless the current character is a space. Spaces continue to be applied
		// to the line regardless of maxWidth. This means that a line's width can
		// be larger than maxWidth due to whitespace, resulting in a larger text box
		// than what was requested, but that is indeed what happens in classic Mac.
		if (*lineWidth + wordWidth >= maxWidth && ch != ' ') {
			return wordStart - start;
		}

		prevChar = ch;
	}
	*lineWidth += wordWidth;
	return text.size() - start;
}

} // End of namespace Sci
