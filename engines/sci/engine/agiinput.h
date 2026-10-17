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

#ifndef SCI_ENGINE_AGIINPUT_H
#define SCI_ENGINE_AGIINPUT_H

#include "sci/engine/vm_types.h"

namespace Sci {

struct EngineState;
struct SciEvent;

// AGI demake, "Input box: AGI Type" (agi_input 0, the default): the typed line sits on the
// bottom text row (screen rows 192-199, with a black gap row at 191) like an AGI game, shown
// wherever the game takes typed lines, and the game keeps running while typing. "Pop-up type" (agi_input 1) leaves Sierra's own
// input box alone.

// True when the AGI input line is in use for this game
bool agiInputActive();

// Called by kGetEvent on every call, before the event is handed to the game: shows or hides the
// line, and redraws it once screen updates are allowed again
void agiInputUpdate(EngineState *s);

// Called by kGetEvent on every call after agiInputUpdate. Returns true if the key was typed into
// the line, in which case the event object has already been made a null event and s->r_acc set.
// Otherwise ev may have been changed (the key or event the game is given) and kGetEvent carries on
bool agiInputHandleKey(EngineState *s, reg_t obj, SciEvent &ev, uint16 modifiers, uint16 mask);

// Called by kDrawStatus: the status line was drawn (true) or switched off (false, DrawStatus 0).
// ">_" only shows while the status line is on, as in AGI
void agiInputStatusDrawn(bool on);

// True while Sierra's input box is being taken over: the AGI demake gfx driver skips screen
// updates so the box is never seen
bool agiInputHideScreenUpdates();

// For the AGI demake gfx driver: 9 rows of 320 EGA colours for screen rows 191-199, with 0xFF
// where the line is not drawn (under open windows). nullptr when the line is not on screen
const byte *agiInputOverlay();

} // End of namespace Sci

#endif
