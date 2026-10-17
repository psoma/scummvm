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
#include "common/system.h"
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

// How it works (checked against Sierra's QfG1 scripts, User.sc and Interface.sc): keys go to the
// game as normal. When the game itself decides to take a typed line it calls User getInput, which
// opens Sierra's input box (Print with an edit item, a modal Dialog that reads its keys through
// GetEvent). The engine takes over that box before it is ever shown:
// - Probe: the first key typed on an empty line. The box is cancelled (Esc) and the key starts the
//   AGI line. Rooms that claim keys themselves (title screens) never call getInput, so the line
//   never appears there
// - Dummy probe: whenever the room or User canInput / controls change, one fake key (a backquote,
//   which no Sierra game uses) is sent when no real event is waiting. If the game opens its box for
//   it, typing is allowed there and ">_" stays on screen. The answer is kept per room and state
// - Submit: Enter on the AGI line. A key is sent so the game calls getInput again, the line is put
//   in the game's own input buffer (User inputLineAddr), and the box is accepted (Enter). The game
//   then parses it and sends it round with User said:, exactly as with Sierra's box
enum Hijack {
	kHijackNone,
	kHijackProbe,
	kHijackDummy,
	kHijackSubmit
};

const uint16 kDummyKey = '`';
// ">_" stays up this long after typing stops being allowed, so it doesn't flicker in room changes
const uint32 kHideHoldMs = 500;

const EngineState *g_lineFor = nullptr;	// the game session the state below belongs to
Common::String g_line;
bool g_shown = false;	// the line is on screen
bool g_refreshPending = false;	// the line changed while screen updates were held back
reg_t g_user = NULL_REG;	// the User object, found once per game session
int g_getInputSel = -1;	// User getInput
int g_inputLineSel = -1;	// User inputLineAddr
Hijack g_hijack = kHijackNone;
uint16 g_probeKey = 0;
uint32 g_probeState = 0;	// the state the pending probe or submit was made in
Common::HashMap<uint32, bool> g_allowed;	// per room and state: does the game take typed lines there
uint32 g_lastWantedMs = 0;	// when ">_" last had a reason to be on screen
// The game's status line is on: DrawStatus was last called with text, not 0. QfG1 (Game.sc, SL)
// leaves it off on its start-up screens (copy protection notice, title) and turns it on for play
bool g_statusOn = false;
const EngineState *g_statusFor = nullptr;	// the game session g_statusOn was set in
Common::String g_submitText;
bool g_submitQueued = false;	// Enter was pressed: send the line next time User doit asks for an event
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

void logLine(const Common::String &text) {
	const int room = g_sci->getEngineState() ? g_sci->getEngineState()->currentRoomNumber() : -1;
	agiDemakeLogLine(Common::String::format("room %d agi input: %s", room, text.c_str()));
}

bool userHasVar(EngineState *s, reg_t user, int sel) {
	const Object *userObject = s->_segMan->getObject(user);
	return sel >= 0 && userObject && userObject->locateVarSelector(s->_segMan, sel) >= 0;
}

bool userCanInput(EngineState *s) {
	const reg_t user = findUser(s);
	if (!userHasVar(s, user, SELECTOR(canInput)))
		return false;
	return readSelectorValue(s->_segMan, user, SELECTOR(canInput)) != 0;
}

bool userControls(EngineState *s) {
	const reg_t user = findUser(s);
	if (!userHasVar(s, user, SELECTOR(controls)))
		return false;
	return readSelectorValue(s->_segMan, user, SELECTOR(controls)) != 0;
}

// The room and User canInput / controls, the state a probe answer is kept for
uint32 currentState(EngineState *s) {
	return ((uint32)s->currentRoomNumber() << 2) | (userCanInput(s) ? 2 : 0) | (userControls(s) ? 1 : 0);
}

// Typing is allowed here: 1 yes, 0 no, -1 not probed yet
int allowedHere(EngineState *s) {
	if (!userCanInput(s))
		return 0;
	const uint32 state = currentState(s);
	if (!g_allowed.contains(state))
		return -1;
	return g_allowed[state] ? 1 : 0;
}

void setAllowed(uint32 state, bool allowed) {
	g_allowed[state] = allowed;
	logLine(Common::String::format("room %d input %d controls %d: typing %s", state >> 2, (state >> 1) & 1, state & 1,
		allowed ? "allowed" : "not allowed"));
}

// The game is inside User getInput (Sierra's input box)
bool inGetInput(EngineState *s) {
	if (g_getInputSel < 0)
		return false;
	for (Common::List<ExecStack>::const_iterator it = s->_executionStack.begin(); it != s->_executionStack.end(); ++it)
		if (it->debugSelector == g_getInputSel)
			return true;
	return false;
}

// This GetEvent is the one in User doit, the game's main input loop (directly, or through Event
// new). Other scripts also call GetEvent (QfG1: Event new in System.sc, Cat.sc), and a key handed to
// them never reaches User handleEvent, so probes and lines are only sent through this one
bool fromUserDoit(EngineState *s) {
	const reg_t user = findUser(s);
	int scriptFrames = 0;
	for (Common::List<ExecStack>::const_iterator it = s->_executionStack.reverse_begin(); it != s->_executionStack.end(); --it) {
		if (it->type == EXEC_STACK_TYPE_KERNEL)
			continue;
		if (++scriptFrames > 2)
			return false;
		if (it->debugSelector == SELECTOR(doit) && it->sendp == user)
			return true;
	}
	return false;
}

// Sierra's input box is up and the AGI line takes it over: everywhere the status line is on
// (start-up screens without it keep Sierra's box)
bool takeOverBox(EngineState *s) {
	return g_statusOn && g_statusFor == s && inGetInput(s);
}

bool windowOpen() {
	if (!g_sci->_gfxPorts)
		return false;
	Common::Array<Common::Rect> rects;
	g_sci->_gfxPorts->agiDemakeOpenWindowRects(rects);
	return !rects.empty();
}

void refreshRow() {
	if (g_sci->_gfxScreen)
		g_sci->_gfxScreen->copyRectToScreen(Common::Rect(0, kRowTop, kScreenW, kRowTop + kRowHeight));
}

void updateShown(EngineState *s) {
	const uint32 now = g_system->getMillis();
	// A message box is open: ">_" stays as it is, as in AGI, and anything typed is cleared
	if (windowOpen() && !inGetInput(s)) {
		if (!g_line.empty()) {
			g_line.clear();
			g_refreshPending = true;
		}
		g_lastWantedMs = now;
		return;
	}
	if (g_statusFor != s)
		g_statusOn = false;
	// While the game has typing off (HandsOff: the hero searching, walking in, other short
	// animations) ">_" stays as it was, as in AGI. It goes when the status line goes, or when the
	// next room or state with typing on turns out not to take typed lines
	// A new room or state not checked yet (the dummy probe is a frame or two away) also keeps ">_" as
	// it was. Room transitions don't call GetEvent while they run, so a timed hold alone ran out
	// during them and ">_" flashed off and on
	const bool canInput = userCanInput(s);
	const int allowed = allowedHere(s);
	const bool wanted = g_statusOn && ((canInput && (!g_line.empty() || allowed == 1)) || ((!canInput || allowed < 0) && g_shown));
	if (wanted)
		g_lastWantedMs = now;
	const bool shown = wanted || (g_shown && now - g_lastWantedMs < kHideHoldMs);
	if (shown == g_shown)
		return;
	g_shown = shown;
	g_refreshPending = true;
}

} // End of anonymous namespace

// Worked out on every call (not cached), so changing the option between games always takes
bool agiInputActive() {
	if (!g_sci || !g_sci->getEngineState())
		return false;
	const bool demake = ConfMan.hasKey("agi_demake") && ConfMan.getBool("agi_demake");
	const bool popUp = ConfMan.hasKey("agi_input") && ConfMan.getInt("agi_input") == 1;
	if (!demake || popUp || !g_sci->_gfxScreen || !g_sci->_gfxScreen->agiDemake() ||
			g_sci->_gfxScreen->getHeight() != 200 || !hasParserVocabulary())
		return false;
	EngineState *s = g_sci->getEngineState();
	if (g_lineFor != s) {
		g_lineFor = s;
		g_line.clear();
		g_shown = false;
		g_refreshPending = false;
		g_user = NULL_REG;
		g_hijack = kHijackNone;
		g_submitQueued = false;
		g_allowed.clear();
		g_getInputSel = g_sci->getKernel()->findSelector("getInput");
		g_inputLineSel = g_sci->getKernel()->findSelector("inputLineAddr");
		if (g_getInputSel < 0 || g_inputLineSel < 0)
			logLine("game has no User getInput / inputLineAddr, using Sierra's input box");
	}
	// Games whose User isn't built like Sierra's SCI0 one keep Sierra's input box
	return g_getInputSel >= 0 && g_inputLineSel >= 0;
}

void agiInputUpdate(EngineState *s) {
	if (!agiInputActive())
		return;
	// Back in the game's main loop with a probe or submit still pending: the game never opened its
	// input box for that key (a room claimed it, or typing is off)
	if (g_hijack != kHijackNone && !inGetInput(s)) {
		if (g_hijack == kHijackSubmit) {
			g_line = g_submitText;
			g_refreshPending = true;
			logLine(Common::String::format("\"%s\" not taken by the game", g_submitText.c_str()));
		}
		setAllowed(g_probeState, false);
		g_hijack = kHijackNone;
	}
	updateShown(s);
	if (g_refreshPending && !inGetInput(s)) {
		g_refreshPending = false;
		refreshRow();
	}
}

bool agiInputHandleKey(EngineState *s, reg_t obj, SciEvent &ev, uint16 modifiers, uint16 mask) {
	if (!agiInputActive())
		return false;

	// Inside Sierra's input box that we opened on purpose: answer it straight away (any event the
	// box asks for becomes Esc for a probe, or Enter with the line in place for a submit)
	// The game opened its input box by itself, not for a probe or submit: a menu shortcut that
	// fills the box first (QfG1 Menu.sc: Ctrl+A "ask about ", Ctrl+L "look at "), or a room that
	// asks for a line itself. The box's starting text goes into the AGI line instead
	if (g_hijack == kHijackNone && takeOverBox(s)) {
		const reg_t user = findUser(s);
		g_line = s->_segMan->getString(readSelector(s->_segMan, user, g_inputLineSel));
		if (g_line.size() > kMaxChars)
			g_line = Common::String(g_line.c_str(), kMaxChars);
		g_refreshPending = true;
		logLine(Common::String::format("game's box taken over, starting \"%s\"", g_line.c_str()));
		setAllowed(currentState(s), true);
		ev.type = kSciEventKeyDown;
		ev.character = kSciKeyEsc;
		updateShown(s);
		return false;
	}

	if (g_hijack != kHijackNone && inGetInput(s)) {
		if (g_hijack == kHijackProbe || g_hijack == kHijackDummy) {
			if (g_hijack == kHijackProbe) {
				g_line = Common::String((char)g_probeKey);
				g_refreshPending = true;
			}
			setAllowed(g_probeState, true);
			ev.type = kSciEventKeyDown;
			ev.character = kSciKeyEsc;
		} else {
			const reg_t user = findUser(s);
			s->_segMan->strcpy_(readSelector(s->_segMan, user, g_inputLineSel), g_submitText.c_str());
			logLine(Common::String::format("\"%s\" sent", g_submitText.c_str()));
			ev.type = kSciEventKeyDown;
			ev.character = kSciKeyEnter;
		}
		g_hijack = kHijackNone;
		updateShown(s);
		return false;	// kGetEvent hands the box the event made here
	}

	// While a message box or dialog is open the keys go to it. Screens without the status line
	// (start-up screens) keep Sierra's own input box
	if (g_hijack != kHijackNone || windowOpen() || !g_statusOn || g_statusFor != s)
		return false;
	const bool canInput = userCanInput(s);
	// With typing off, keys still go into the line while ">_" is up (Enter waits until typing is on)
	if (!canInput && !g_shown)
		return false;
	const int allowed = allowedHere(s);
	const bool userDoit = fromUserDoit(s) && canInput;

	// Enter was pressed on the line: send it now that User doit is asking (see the Enter key below)
	if (g_submitQueued && userDoit && ev.type == kSciEventNone) {
		g_submitQueued = false;
		g_hijack = kHijackSubmit;
		g_probeState = currentState(s);
		ev.type = kSciEventKeyDown;
		ev.character = (byte)g_submitText[0];
		ev.modifiers = kSciKeyModNone;
		return false;
	}

	// Nothing waiting and this room and state not probed yet: send the dummy key
	if (!g_submitQueued && userDoit && ev.type == kSciEventNone && allowed < 0 && (mask & kSciEventKeyDown)) {
		g_hijack = kHijackDummy;
		g_probeState = currentState(s);
		ev.type = kSciEventKeyDown;
		ev.character = kDummyKey;
		ev.modifiers = kSciKeyModNone;
		return false;
	}

	if (ev.type != kSciEventKeyDown)
		return false;
	// Ctrl and Alt keys stay with the game (menu shortcuts, debug keys)
	if (modifiers & (kSciKeyModCtrl | kSciKeyModAlt))
		return false;

	const uint16 key = ev.character;
	const bool printable = (key >= 32 && key <= 126);

	if (g_line.empty() && canInput && allowed != 1) {
		// Not known to be allowed here: the key goes to the game, and if the game opens its input
		// box for it the box is taken over (see above)
		if (printable && userDoit && !g_submitQueued) {
			g_hijack = kHijackProbe;
			g_probeKey = key;
			g_probeState = currentState(s);
		}
		return false;
	}
	// Enter, Backspace and Esc on an empty line go to the game (Esc opens the menu bar)
	if (g_line.empty() && !printable)
		return false;

	if (printable) {
		if (g_line.size() < kMaxChars)
			g_line += (char)key;
	} else if (key == kSciKeyBackspace) {
		g_line.deleteLastChar();
	} else if (key == kSciKeyEsc) {
		g_line.clear();
	} else if (key == kSciKeyEnter) {
		// The line's first letter is sent when User doit next asks for an event, so the game opens
		// its input box again as it did for the probe, then the box gets the whole line
		if (g_submitQueued)
			return false;
		g_submitText = g_line;
		g_submitQueued = true;
		g_line.clear();
	} else {
		return false;
	}

	// The key was typed into the line: the game sees no event
	refreshRow();
	updateShown(s);
	SegManager *segMan = s->_segMan;
	writeSelectorValue(segMan, obj, SELECTOR(type), kSciEventNone);
	writeSelectorValue(segMan, obj, SELECTOR(message), 0);
	writeSelectorValue(segMan, obj, SELECTOR(modifiers), modifiers);
	s->r_acc = NULL_REG;
	return true;
}

void agiInputStatusDrawn(bool on) {
	g_statusOn = on;
	g_statusFor = g_sci ? g_sci->getEngineState() : nullptr;
}

bool agiInputHideScreenUpdates() {
	EngineState *s = g_sci ? g_sci->getEngineState() : nullptr;
	return s && agiInputActive() && (g_hijack != kHijackNone ? inGetInput(s) : takeOverBox(s));
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
