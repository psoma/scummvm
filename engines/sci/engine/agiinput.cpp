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
#include "graphics/fonts/dosfont.h"

#include "sci/sci.h"
#include "sci/event.h"
#include "sci/resource/resource.h"
#include "sci/engine/agiinput.h"
#include "sci/engine/kernel.h"
#include "sci/engine/seg_manager.h"
#include "sci/engine/selector.h"
#include "sci/engine/state.h"
#include "sci/engine/vm.h"
#include "sci/graphics/ports.h"
#include "sci/graphics/screen.h"
#include "sci/parser/vocabulary.h"

namespace Sci {

void agiDemakeLogLine(const Common::String &line);	// text16.cpp, AGI demake diagnostics

namespace {

// The bottom AGI text row: one black gap row (191) between the picture and the text, then 8 rows
// of the 8x8 AGI font (192-199), 40 columns of 8 pixels
const int kRowTop = 191;
const int kGapRows = 1;
const int kRowHeight = 9;
const int kScreenW = 320;
const int kColumns = kScreenW / 8;
// Prompt (1 column) + text + cursor (1 column) fill the 40 columns
const uint kMaxChars = kColumns - 2;
const char kPrompt = '>';
const char kCursor = '_';
const byte kTextColor = 15;	// white
const byte kBackColor = 0;	// black
const char *const kBufferName = "AGI demake input line";
const int kBufferSize = 64;

const EngineState *g_lineFor = nullptr;	// the game session the typed line belongs to
Common::String g_line;
bool g_shown = false;	// typing is allowed, so the line is on screen
reg_t g_user = NULL_REG;	// the User object, found once per game session
byte g_overlay[kRowHeight * kScreenW];

bool hasParserVocabulary() {
	if (!g_sci->hasParser() || !g_sci->getVocabulary())
		return false;
	ResourceManager *resMan = g_sci->getResMan();
	return resMan->testResource(ResourceId(kResourceTypeVocab, VOCAB_RESOURCE_SCI0_MAIN_VOCAB)) ||
		resMan->testResource(ResourceId(kResourceTypeVocab, VOCAB_RESOURCE_SCI1_MAIN_VOCAB));
}

// The game's User object, found the same way as GuestAdditions::userHasControl
reg_t findUser(EngineState *s) {
	if (!g_user.isNull() && s->_segMan->getObject(g_user))
		return g_user;
	reg_t user = s->_segMan->findObjectByName("User");
	if (user.isNull() && (g_sci->getGameId() == GID_LSL3 || g_sci->getGameId() == GID_SQ3))
		user = s->_segMan->findObjectByName("PUser");
	if (user.isNull())
		user = s->variables[VAR_GLOBAL][kGlobalVarUser];
	g_user = user;
	return user;
}

// Typing is allowed when the game has the player "hands on": User canInput and controls both on,
// the same test as GuestAdditions::userHasControl. Title, logo and cutscene screens leave at least
// one of them off (QfG1 leaves canInput on at its title screen, so canInput alone isn't enough)
bool userCanType(EngineState *s) {
	const reg_t user = findUser(s);
	const Object *userObject = s->_segMan->getObject(user);
	if (!userObject || userObject->locateVarSelector(s->_segMan, SELECTOR(canInput)) < 0 ||
			userObject->locateVarSelector(s->_segMan, SELECTOR(controls)) < 0)
		return false;
	return readSelectorValue(s->_segMan, user, SELECTOR(canInput)) != 0 &&
		readSelectorValue(s->_segMan, user, SELECTOR(controls)) != 0;
}

bool windowOpen() {
	if (!g_sci->_gfxPorts)
		return false;
	Common::Array<Common::Rect> rects;
	g_sci->_gfxPorts->agiDemakeOpenWindowRects(rects);
	return !rects.empty();
}

// A small block of game memory that holds the typed line for the parser. It is found again by
// name after a restore (it is saved with the game), so only one is ever made per game
reg_t lineBuffer(EngineState *s) {
	const Common::Array<SegmentObj *> &heap = s->_segMan->getSegments();
	for (uint seg = 1; seg < heap.size(); ++seg) {
		if (heap[seg] && heap[seg]->getType() == SEG_TYPE_DYNMEM &&
				((DynMem *)heap[seg])->_description == kBufferName && ((DynMem *)heap[seg])->_size >= (uint)kBufferSize)
			return make_reg(seg, 0);
	}
	reg_t addr;
	s->_segMan->allocDynmem(kBufferSize, kBufferName, &addr);
	return addr;
}

void refreshRow() {
	if (g_sci->_gfxScreen)
		g_sci->_gfxScreen->copyRectToScreen(Common::Rect(0, kRowTop, kScreenW, kRowTop + kRowHeight));
}

void logLine(const Common::String &text) {
	const int room = g_sci->getEngineState() ? g_sci->getEngineState()->currentRoomNumber() : -1;
	agiDemakeLogLine(Common::String::format("room %d agi input: %s", room, text.c_str()));
}

} // End of anonymous namespace

// Worked out on every call (not cached), so changing the option between games always takes
bool agiInputActive() {
	if (!g_sci || !g_sci->getEngineState())
		return false;
	if (g_lineFor != g_sci->getEngineState()) {
		g_lineFor = g_sci->getEngineState();
		g_line.clear();
		g_shown = false;
		g_user = NULL_REG;
	}
	const bool demake = ConfMan.hasKey("agi_demake") && ConfMan.getBool("agi_demake");
	const bool popUp = ConfMan.hasKey("agi_input") && ConfMan.getInt("agi_input") == 1;
	return demake && !popUp && g_sci->_gfxScreen && g_sci->_gfxScreen->agiDemake() &&
		g_sci->_gfxScreen->getHeight() == 200 && hasParserVocabulary();
}

void agiInputUpdate(EngineState *s) {
	const bool shown = agiInputActive() && userCanType(s);
	if (shown == g_shown)
		return;
	g_shown = shown;
	logLine(shown ? "line shown" : "line hidden");
	refreshRow();
}

bool agiInputHandleKey(EngineState *s, reg_t obj, const SciEvent &ev, uint16 modifiers) {
	if (!agiInputActive())
		return false;
	// Ctrl and Alt keys stay with the game (menu shortcuts, debug keys)
	if (modifiers & (kSciKeyModCtrl | kSciKeyModAlt))
		return false;
	// While a message box or dialog is open the keys go to it, as in Pop-up type
	if (windowOpen() || !g_shown)
		return false;

	const uint16 key = ev.character;
	SegManager *segMan = s->_segMan;
	const bool printable = (key >= 32 && key <= 126);

	if (printable) {
		// A space on an empty line still goes to the game, whatever it does there
		if (key == ' ' && g_line.empty())
			return false;
		if (g_line.size() < kMaxChars) {
			g_line += (char)key;
			refreshRow();
		}
	} else if (key == kSciKeyBackspace || key == kSciKeyEsc) {
		// Backspace and Esc on an empty line go to the game (Esc opens the menu bar)
		if (g_line.empty())
			return false;
		if (key == kSciKeyBackspace)
			g_line.deleteLastChar();
		else
			g_line.clear();
		refreshRow();
	} else if (key == kSciKeyEnter) {
		if (g_line.empty())
			return false;
		const Common::String text = g_line;
		g_line.clear();
		refreshRow();

		// Hand the line to the parser and the game exactly as the pop-up would have: the event
		// becomes a said event, and kParse claims it (and prints the game's "I don't understand"
		// message) when the line doesn't parse
		const reg_t buffer = lineBuffer(s);
		segMan->strcpy_(buffer, text.c_str());
		writeSelectorValue(segMan, obj, SELECTOR(type), kSciEventSaid);
		writeSelectorValue(segMan, obj, SELECTOR(message), 0);
		writeSelectorValue(segMan, obj, SELECTOR(modifiers), modifiers);
		writeSelectorValue(segMan, obj, SELECTOR(claimed), 0);
		reg_t params[2] = { buffer, obj };
		const reg_t parsed = kParse(s, 2, params);
		const bool ok = !parsed.isNull() && readSelectorValue(segMan, obj, SELECTOR(claimed)) == 0;
		logLine(Common::String::format("\"%s\" %s", text.c_str(), ok ? "parsed" : "not parsed"));
		if (ok) {
			s->r_acc = TRUE_REG;
			return true;
		}
		// The game has already answered; nothing more happens with this event
		writeSelectorValue(segMan, obj, SELECTOR(type), kSciEventNone);
		s->r_acc = NULL_REG;
		return true;
	} else {
		return false;
	}

	// The key was typed into the line: the game sees no event
	writeSelectorValue(segMan, obj, SELECTOR(type), kSciEventNone);
	writeSelectorValue(segMan, obj, SELECTOR(message), 0);
	writeSelectorValue(segMan, obj, SELECTOR(modifiers), modifiers);
	s->r_acc = NULL_REG;
	return true;
}

const byte *agiInputOverlay() {
	if (!agiInputActive() || !g_shown)
		return nullptr;

	memset(g_overlay, kBackColor, sizeof(g_overlay));
	// ">" + text + "_", one 8x8 AGI font character per column, drawn pixel for pixel
	const Common::String shown = Common::String(kPrompt) + g_line + kCursor;
	for (uint col = 0; col < shown.size() && col < (uint)kColumns; ++col) {
		const byte *glyph = Graphics::DosFont::fontData_PCBIOS + (byte)shown[col] * 8;
		for (int y = 0; y < 8; ++y)
			for (int x = 0; x < 8; ++x)
				if (glyph[y] & (0x80 >> x))
					g_overlay[(kGapRows + y) * kScreenW + col * 8 + x] = kTextColor;
	}

	// Open windows stay on top of the line
	if (g_sci->_gfxPorts) {
		Common::Array<Common::Rect> rects;
		g_sci->_gfxPorts->agiDemakeOpenWindowRects(rects);
		for (uint i = 0; i < rects.size(); ++i) {
			const Common::Rect row(0, kRowTop, kScreenW, kRowTop + kRowHeight);
			if (!rects[i].intersects(row))
				continue;
			const Common::Rect r = rects[i].findIntersectingRect(row);
			for (int y = r.top; y < r.bottom; ++y)
				memset(g_overlay + (y - kRowTop) * kScreenW + r.left, 0xFF, r.width());
		}
	}
	return g_overlay;
}

} // End of namespace Sci
