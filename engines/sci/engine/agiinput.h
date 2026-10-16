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
// bottom text row (screen rows 192-199, with a black gap row at 191) like an AGI game, and the game keeps running while
// typing. "Pop-up type" (agi_input 1) leaves Sierra's own input box alone.

// True when the AGI input line is in use for this game
bool agiInputActive();

// Called by kGetEvent on every call: shows or hides the line as typing becomes allowed or not
void agiInputUpdate(EngineState *s);

// Called by kGetEvent for each key press. Returns true if the key was taken by the input line,
// in which case the event object has already been filled in (a null event, or a said event on
// Enter) and s->r_acc set
bool agiInputHandleKey(EngineState *s, reg_t obj, const SciEvent &ev, uint16 modifiers);

// For the AGI demake gfx driver: 9 rows of 320 EGA colours for screen rows 191-199, with 0xFF
// where the line is not drawn (under open windows). nullptr when the line is not on screen
const byte *agiInputOverlay();

} // End of namespace Sci

#endif
