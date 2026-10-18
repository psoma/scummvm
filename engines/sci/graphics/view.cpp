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
#include "common/file.h"
#include "graphics/fonts/dosfont.h"
#include "common/hashmap.h"
#include "common/hash-str.h"
#include "sci/sci.h"
#include "sci/engine/state.h"
#include "sci/graphics/drivers/gfxdriver.h"
#include "sci/graphics/screen.h"
#include "sci/graphics/palette16.h"
#include "sci/graphics/remap.h"
#include "sci/graphics/coordadjuster.h"
#include "sci/graphics/view.h"
#include "sci/graphics/ports.h"

#include "sci/graphics/scifx.h"

namespace Sci {

// QfG1 intro lettering, drawn ready: the credits (view 902) and character select (view 506 loop 1)
// in the alphabet Tom signed off (headings pink, names the same shapes in green), letters kerned
// so every row keeps a fat pixel between them and 2 between words, and the title (view 913 loop 1)
// with Sierra's letters re-spaced, 3 fat pixels between every word. Each cel is resized to its
// lettering and moved to "left" on screen (the cel is placed by the script at posnX, centred), so:
// words on their own stay centred, words sharing a line keep Sierra's gaps (WRITTEN & DIRECTED,
// GAME DEVELOPMENT, CHOOSE YOUR CHARACTER), ANIMATIONS ends a word gap before the "&" and BACKGROUNDS
// ends where the "&" does, and the title is one evenly spaced line. "BY" is plain pink letters and
// SCENES is gone (no rows). Tom's second pass: "WRITTEN &" and "DIRECTED BY" on two centred lines,
// with the "&" and "BY" drawn into those cels (the "&" cel is blanked, and "BY" is left out where it
// is placed at x 155, the WRITTEN screen, but still shows for MUSIC BY and PRODUCED BY at x 156);
// "ANIMATIONS &" and "BACKGROUNDS" both centred with the "&" still ending where the S does; GAME
// dropped, DEVELOPMENT and SYSTEM centred. One string per row, colours in hex, '.' empty
struct AgiDemakeQfg1Art {
	int16 view, loop, cel;
	int16 posnX, from, left;
	int16 down;			// moved down this many pixels (DIRECTED BY goes on the second line)
	bool onlyHere;		// drawn only when the cel is placed at posnX, blank elsewhere ("BY")
	const char *rows[17];	// no rows: the cel is left blank
};
static const AgiDemakeQfg1Art kAgiDemakeQfg1Art[] = {
	{ 902, 0, 0, 110, 67, 114, 0, false, { "cc..cc.cccc.ccc.cccc..cccc..ccccc.cc..c...cc.", "cc...c.cc.c.cc..c.ccc.c.ccc.cc.c..ccc.c...cc.", ".c.d.c..c.c11c..c.c.c.c.c.c..c.c...cc.c..cddc", ".d.5.d.1d.d..d1..1d.1..1d.1..d..1..dd.d...cc.", "5c.d.c.5dd1115..15c1..15c1..1cd....d.dd..ddd.", "1d.d.d..d.d..d...1d.1..1d.1..d..1..d.dd..d.5.", ".d.51d.1d.d..d....d1....d1...d.1d..d.dd..5...", ".55555.d51d.d5...d5....d5...d5.d1.55..5..5d.d", ".5..5..55.5.555..5551..5551.5555..55..5..555.", "...........5..............................5..", "...........5..............................5.." } },
	{ 902, 0, 3, 203, 75, 106, 17, false, { "ccc..ccc.cccc.ccccc..ccc.cccc..ccccc.ccc....cccc..c..c", "cc.c.cc..cc.c.cc.c..cc.c.c.ccc.cc.c..cc.c...cc.c..c.cc", ".c.c.1c...c.c1.c.c..c..d1c.c.c..c.c...c.c....c.c..c.c1", ".d3d..d1.1d.d..d..1.c135..1d.1..d..1..d3d....dc11.d.d1", "5ce3d15..5dd111cd...d3e1115c1..1cd...5ce3d..5c.c..d.5.", "1d3.d.d...d.d..d..1.d13...1d.1..d..1.1d3.d..1d..d..dd1", ".d.1d.d..1d.d..d.1d.51.1d..d1...d.1d..d.1d...d.1d.1.d1", "d5.5.d5..d51d.d5.d1.551d5.d5...d5.d1.d5.5...d5..5.d1d.", "5555.555.55.5.5555...155..5551.5555..5555...5555..d.5.", ".............5....................................d.d.", ".............5.....................................55." } },
	{ 902, 0, 4, 157, 110, 94, 0, false, { "cccc.cccc..ccc...ccc..cccc...c...ccc.cc.ccc.cc.ccc.cc..c..ccc.", "cc.c.cc.c.cc.cc.cc.cc.cc.c...cc...ccccc..ccccc.cc..ccc.c.cc.cc", ".c.c..c.c1c...c.c......c.c1.c.c..1c.c.c.1c.c.c.1c...cc.c.c....", ".d.d.1d.d.d.3.d.d13...1d.d..d1d1.1d.d.d.1d.d.d..d1..dd.d.d13..", "5dd..5dd11d3e3d.d3edd.5dd11.5555.151d.c.151d.c.15...d.dd.d3edd", "1d....d.d.d.3.d.d13.d..d.d.1d5d...d5c.d..d5c.d..d...d.dd.d13.d", ".d.1.1d.d.d...d.d...d.1d.d..d.d..1d.d.d.1d.d.d..d...d.dd.d...d", "d5...d51d.55.55.55.55.d51d..d.5d..d...5..d...5.d5..55..5.55.55", "555..55.5..555...555..55.5.55.55.555.55.555.55.555.55..5..555.", ".........5................5...................................", ".........5................5..................................." } },
	{ 902, 0, 5, 143, 81, 102, 0, false, { "..c...cc..c.ccc.ccc.cc...c..cccc..ccc.ccc..cc..c..ccc....cc.", "..cc..ccc.c.cc...ccccc...cc.c.ccc.cc.cc.cc.ccc.c.cc.cc...cc.", ".c.c...cc.c.1c..1c.c.c..c.c.c.c.c.1c.c...c..cc.c.cc.....cddc", ".d1d1..dd.d..d1.1d.d.d..d1d1.1d.1..d1d.3.d..dd.d..dd.....cc.", ".5555..d.dd.15..151d.c..555515c1..15.d3e3d..d.dd...dd...ddd.", "1d5d...d.dd..d...d5c.d.1d5d..1d.1..d.d.3.d..d.dd....dd..d.5.", ".d.d...d.dd..d..1d.d.d..d.d...d1...d.d...d..d.dd.d..dd..5...", ".d.5d.55..5.d5...d...5..d.5d.d5...d5.55.55.55..5.55.55..5d.d", "55.55.55..5.555.555.55.55.55.5551.555.555..55..5..555...555.", ".........................................................5..", ".........................................................5.." } },
	{ 902, 0, 6, 124, 99, 98, 0, false, { "cccc...c....ccc..cc.cc..ccc..cccc..ccc..cc.c.cc..c.ccc....ccc.", "cc.c...cc..cc.c..cc.c..cc.cc.cc.c.cc.cc.cc.c.ccc.c.cc.c..cc.cc", ".c.c..c.c..c..d1..c.c1.c......c.c1c...c.c..c..cc.c..c.c..cc...", ".dc11.d1d1.c135..1dd.1.d13...1d.d.d.3.d.d.3d..dd.d..d3d...dd..", "5c.c..5555.d3e11.5dd15.d3edd.5dd11d3e3d.c.e5..d.dd.5ce3d...dd.", "1d..d1d5d..d13...1dd.1.d13.d..d.d.d.3.d.d.3d..d.dd.1d3.d....dd", ".d.1d.d.d..51.1d..d.d1.d...d.1d.d.d...d.d.1d..d.dd..d.1d.d..dd", "d5..5.d.5d.551d5.d51d..55.55.d51d.55.55.1dd5.55..5.d5.5..55.55", "5555.55.55..155..55.5...555..55.5..555...551.55..5.5555...555.", ".....................5...........5............................", ".....................5...........5............................" } },
	{ 902, 0, 7, 211, 59, 0, 0, false, {  } },
	{ 902, 0, 8, 99, 42, 0, 0, false, {  } },
	{ 902, 0, 9, 180, 109, 98, 0, false, { "ccc..ccccc.cc..c.ccccc.ccc...ccc..cccc.ccc.cc.ccccc.cc..c.cccc.", "cc.c.cc.c...cc.c.cc.c..cc...cc.cc.cc.c..ccccc.cc.c..ccc.c.c.ccc", ".c.c..c.c..1.c.c..c.c..1c...c...c..c.c.1c.c.c..c.c...cc.c.c.c.c", ".d3d..d..1...d3d..d..1..d1..d.3.d..d.d.1d.d.d..d..1..dd.d..1d.1", "5ce3d1cd....5ce5.1cd...55...d3e3d.5dd..151d.c.1cd....d.dd.15c1.", "1d3.d.d..1.1.d3d..d..1.1d11.d.3.d.1d....d5c.d..d..1..d.dd..1d.1", ".d.1d.d.1d...d1d..d.1d..d1..d...d..d.1.1d.d.d..d.1d..d.dd...d1.", "d5.5.d5.d1..1dd..d5.d1.d5.d.55.55.d5....d...5.d5.d1.55..5..d5..", "5555.5555.....5..5555..5555..555..555..555.55.5555..55..5..5551", "...............................................................", "..............................................................." } },
	{ 902, 0, 10, 156, 59, 124, 0, false, { ".ccc..c..c..ccc..cccc..ccccc.ccc.cc", "cc.cc.c.cc.cc.cc.c.ccc.cc.c...ccccc", "cc....c.c1.cc....c.c.c..c.c..1c.c.c", ".dd...d.d1..dd....1d.1..d..1.1d.d.d", "..dd..d.5....dd..15c1..1cd...151d.c", "...dd..dd1....dd..1d.1..d..1..d5c.d", "d..dd.1.d1.d..dd...d1...d.1d.1d.d.d", "55.55.d1d..55.55..d5...d5.d1..d...5", ".555..d.5...555...5551.5555..555.55", "......d.d..........................", ".......55.........................." } },
	{ 902, 0, 11, 157, 76, 112, 0, false, { "cccc.cccc..ccc..ccc...cc.c..ccc..ccccc.ccc..", "cc.c.cc.c.cc.cc.cc.c..cc.c.cc.c..cc.c..cc.c.", ".c.c..c.c1c...c..c.c..c..c.c..d1..c.c...c.c.", ".d.d.1d.d.d.3.d..d3d..d.3d.c135...d..1..d3d.", "5dd..5dd11d3e3d.5ce3d.c.e5.d3e11.1cd...5ce3d", "1d....d.d.d.3.d.1d3.d.d.3d.d13....d..1.1d3.d", ".d.1.1d.d.d...d..d.1d.d.1d.51.1d..d.1d..d.1d", "d5...d51d.55.55.d5.5..1dd5.551d5.d5.d1.d5.5.", "555..55.5..555..5555...551..155..5555..5555.", ".........5..................................", ".........5.................................." } },
	{ 902, 0, 12, 158, 84, 108, 0, false, { "ccccc.cc.cc.ccccc..ccc..cc.c.cccc..ccc.cc..c.ccccc", "cc.c...c1cc.cc.c..cc.c..cc.c.c.ccc.cc...cc.c.cc.c.", ".c.c...c.c...c.c..c..d1.c..c.c.c.c.1c..1.c.c..c.c.", ".d..1.1ccc1..d..1.c135..d.3d..1d.1..d1...d3d..d..1", "1cd....1c.1.1cd...d3e11.c.e5.15c1..15...5ce5.1cd..", ".d..1.1ddd...d..1.d13...d.3d..1d.1..d..1.d3d..d..1", ".d.1d..d.d...d.1d.51.1d.d.1d...d1...d....d1d..d.1d", "d5.d1.1d.5d.d5.d1.551d5.1dd5..d5...d5...1dd..d5.d1", "5555..55.55.5555...155...551..5551.555....5..5555.", "..................................................", ".................................................." } },
	{ 902, 0, 13, 159, 79, 114, 0, false, { "cccc.cccc..ccc..ccc...cc.c..ccc..ccccc.cccc.", "cc.c.cc.c.cc.cc.cc.c..cc.c.cc.c..cc.c..cc.c.", ".c.c..c.c1c...c..c.c..c..c.c..d1..c.c...c.c1", ".d.d.1d.d.d.3.d..d3d..d.3d.c135...d..1.1d.d.", "5dd..5dd11d3e3d.5ce3d.c.e5.d3e11.1cd...5dd11", "1d....d.d.d.3.d.1d3.d.d.3d.d13....d..1..d.d.", ".d.1.1d.d.d...d..d.1d.d.1d.51.1d..d.1d.1d.d.", "d5...d51d.55.55.d5.5..1dd5.551d5.d5.d1.d51d.", "555..55.5..555..5555...551..155..5555..55.5.", ".........5.................................5", ".........5.................................5" } },
	{ 902, 5, 0, 156, 46, 130, 0, false, { "ccc.cc.cc.c..ccc..ccc.ccc.", ".ccccc.cc.c.cc.cc.cc.cc.c.", "1c.c.c.c..c.cc....1c.c..d1", "1d.d.d.d.3d..dd....d1c135.", "151d.c.c.e5...dd..15.d3e11", ".d5c.d.d.3d....dd..d.d13..", "1d.d.d.d.1d.d..dd..d.51.1d", ".d...5.1dd5.55.55.d5.551d5", "555.55..551..555..555.155.", "..........................", ".........................." } },
	{ 506, 1, 0, 83, 61, 44, 0, false, { ".ccc..cc.cc..ccc...ccc...ccc..ccccc", "cc.c..cc.c..cc.cc.cc.cc.cc.cc.cc.c.", "c..d1..c.c..c...c.c...c.cc.....c.c.", "c135...d.d..d.3.d.d.3.d..dd....d..1", "d3e11.5dd55.d3e3d.d3e3d...dd..1cd..", "d13....d.d..d.3.d.d.3.d....dd..d..1", "51.1d..d.d..d...d.d...d.d..dd..d.1d", "551d5.d5.d..55.55.55.55.55.55.d5.d1", ".155..55.55..555...555...555..5555." } },
	{ 506, 1, 1, 146, 42, 124, 0, false, { "c..c..ccc..cc.c.cccc.", "c.cc.cc.cc.cc.c.cc.c.", "c.c1.c...c.c..c..c.c1", "d.d1.d.3.d.d.3d.1d.d.", "d.5..d3e3d.c.e5.5dd11", ".dd1.d.3.d.d.3d..d.d.", "1.d1.d...d.d.1d.1d.d.", "d1d..55.55.1dd5.d51d.", "d.5...555...551.55.5.", "d.d.................5", ".55.................5" } },
	{ 506, 1, 2, 220, 92, 174, 0, false, { ".ccc..cc.cc...c...cccc...c....ccc.cccc..ccccc.cccc.", "cc.c..cc.c....cc..cc.c...cc..cc.c.c.ccc.cc.c..cc.c.", "c..d1..c.c...c.c...c.c1.c.c..c..d1c.c.c..c.c...c.c1", "c135...d.d...d1d1.1d.d..d1d1.c135..1d.1..d..1.1d.d.", "d3e11.5dd55..5555.5dd11.5555.d3e1115c1..1cd...5dd11", "d13....d.d..1d5d...d.d.1d5d..d13...1d.1..d..1..d.d.", "51.1d..d.d...d.d..1d.d..d.d..51.1d..d1...d.1d.1d.d.", "551d5.d5.d...d.5d.d51d..d.5d.551d5.d5...d5.d1.d51d.", ".155..55.55.55.55.55.5.55.55..155..5551.5555..55.5.", "......................5...........................5", "......................5...........................5" } },
	{ 902, 1, 0, 157, 117, 94, 0, false, { "eee...eee..eeee.eee....e...ee..e.ee..e...eee...eee..eee..eeeee", "ee...ee.ee.ee.e.ee.....ee..eee.e.eee.e..ee.e..ee.ee.ee...ee.e.", "1e...e...e..e.e11e....e.e...ee.e..ee.e..e..a1.e...e.1e....e.e.", ".a1..a.5.a.1a.a..a1...a1a1..aa.a..aa.a..e152..a.5.a..a1...a..1", "22...a5c5a.2aa1112....2222..a.aa..a.aa..a5c11.a5c5a.22...1ea..", "1a11.a.5.a..a.a..a...1a2a...a.aa..a.aa..a15...a.5.a.1a11..a..1", ".a1..a...a.1a.a..a....a.a...a.aa..a.aa..21.1a.a...a..a1...a.1a", "a2.a.22.22.a21a.a2....a.2a.22..2.22..2..221a2.22.22.a2.a.a2.a1", "2222..222..22.2.222..22.22.22..2.22..2...122...222..2222.2222.", "...............2..............................................", "...............2.............................................." } },
	{ 902, 1, 1, 158, 148, 74, 0, false, { "eeee..eee..eeee..eeeee.eeee.eeee...eeeee.eee.eee...eee..ee.ee.eeee...e....eee..ee.ee", "ee.e.ee.ee.ee.e..ee.e..ee.e.e.eee..ee.e..ee.ee.ee.ee.e..ee.e..ee.e...ee..ee.e..ee.e.", ".e.e1e...e..e.e...e.e...e.e1e.e.e...e.e..1e.ee....e..a1..e.e...e.e..e.e..e..a1..e.e.", "1a.a.a.5.a..ae11..a..1.1a.a..1a.1...a..1..a1.aa...e152...a.a...ae11.a1a1.e152...a.a.", "2aa11a5c5a.2e.e..1ea...2aa1112e1...1ea...12...aa..a5c11.2aa22.2e.e..2222.a5c11.2aa22", ".a.a.a.5.a.1a..a..a..1..a.a..1a.1...a..1..a....aa.a15....a.a..1a..a1a2a..a15....a.a.", "1a.a.a...a..a.1a..a.1a.1a.a...a1....a.....a.a..aa.21.1a..a.a...a.1a.a.a..21.1a..a.a.", "a21a.22.22.a2..2.a2.a1.a21a..a2....a2....a2.22.22.221a2.a2.a..a2..2.a.2a.221a2.a2.a.", "22.2..222..2222..2222..22.2..2221..222...222.222...122..22.22.2222.22.22..122..22.22", "....2......................2........................................................", "....2......................2........................................................" } },
	{ 902, 1, 2, 158, 97, 106, 0, false, { ".eee...eee..eeee.eeeee.e..e...eee...eee..eee..eeeee", "ee.e..ee.ee.ee.e.ee.e..e.ee..ee.e..ee.ee.ee...ee.e.", "e..a1.e...e..e.e1.e.e..e.e1..e..a1.e...e.1e....e.e.", "e152..a.5.a.1a.a..a..1.a.a1..e152..a.5.a..a1...a..1", "a5c11.a5c5a.2aa111ea...a.2...a5c11.a5c5a.22...1ea..", "a15...a.5.a..a.a..a..1..aa1..a15...a.5.a.1a11..a..1", "21.1a.a...a.1a.a..a.1a.1.a1..21.1a.a...a..a1...a.1a", "221a2.22.22.a21a.a2.a1.a1a...221a2.22.22.a2.a.a2.a1", ".122...222..22.2.2222..a.2....122...222..2222.2222.", "................2......a.a.........................", "................2.......22........................." } },
	{ 902, 1, 3, 158, 116, 92, 0, false, { "ee.ee.eeeee.ee..e.ee..e..ee..e.eee.eee..ee.ee.eee.ee.e.e..e.eeeee", "ee.e..ee.e..eee.e.eee.e..eee.e.ee.ee.ee.ee.e..ee..ee.e.e.ee.ee.e.", ".e.e1..e.e...ee.e..ee.e...ee.e.1e.ee.....e.e..1e..e..e.e.e1..e.e.", "1aa.1..a..1..aa.a..aa.a...aa.a..a1.aa....a.a...a1.a.5a.a.a1..a..1", "2aa12.1ea....a.aa..a.aa...a.aa.12...aa..2aa22.12..e.c2.a.2..1ea..", "1aa.1..a..1..a.aa..a.aa...a.aa..a....aa..a.a...a..a.5a..aa1..a..1", ".a.a1..a.1a..a.aa..a.aa...a.aa..a.a..aa..a.a...a..a.1a.1.a1..a.1a", "a21a..a2.a1.22..2.22..2..22..2.a2.22.22.a2.a..a2..1aa2.a1a..a2.a1", "22.2..2222..22..2.22..2..22..2.222.222..22.22.222..221.a.2..2222.", "....2..................................................a.a.......", "....2...................................................22......." } },
	{ 902, 1, 4, 158, 91, 104, 0, false, { "eeeee.eeeee.eeeee.eeeee...eee..eeee..eee..ee..ee.eeeee", "e..e..ee.e..ee.e..ee.e...ee.e..ee.e.ee.ee.ee...e.ee.e.", "a..e...e.e...e.e...e.e...e..a1..e.e1e...e..e.a.e..e.e.", "25.1...a..1..a..1..a..1..e152..1a.a.a.5.a..a.2.a..a..1", ".55e..1ea...1ea...1ea....a5c11.2aa11a5c5a.2e.a.e.1ea..", "15.a...a..1..a..1..a..1..a15....a.a.a.5.a.1a.a.a..a..1", "...a...a.1a..a.....a.....21.1a.1a.a.a...a..a.21a..a.1a", "a..a1.a2.a1.a2....a2.....221a2.a21a.22.22..22222.a2.a1", "a2.a2.2222..222...222.....122..22.2..222...2..2..2222.", "a2a2...............................2..................", ".22................................2.................." } },
	{ 902, 2, 0, 157, 141, 74, 0, false, { "eeeee.eeeee.eeeee.eeeee...eee..eeee..eeeee.eeee.ee.ee.eeeee.ee..e..eee...eee..ee..e", "e..e..ee.e..ee.e..ee.e...ee.ee.e.eee.ee.e..ee.e.ee.e..ee.e..eee.e.ee.ee.ee.ee.eee.e", "a..e...e.e...e.e...e.e...ee....e.e.e..e.e...e.e..e.e...e.e...ee.e.ee....e...e..ee.e", "25.1...a..1..a..1..a..1...aa....1a.1..a..1..a.a..a.a...a..1..aa.a..aa...a.5.a..aa.a", ".55e..1ea...1ea...1ea......aa..12e1..1ea...2aa..2aa22.1ea....a.aa...aa..a5c5a..a.aa", "15.a...a..1..a..1..a..1.....aa..1a.1..a..1.1a....a.a...a..1..a.aa....aa.a.5.a..a.aa", "...a...a.1a..a.....a.....a..aa...a1...a.1a..a.1..a.a...a.1a..a.aa.a..aa.a...a..a.aa", "a..a1.a2.a1.a2....a2.....22.22..a2...a2.a1.a2...a2.a..a2.a1.22..2.22.22.22.22.22..2", "a2.a2.2222..222...222.....222...2221.2222..222..22.22.2222..22..2..222...222..22..2", "a2a2...............................................................................", ".22................................................................................" } },
	{ 902, 2, 1, 157, 131, 82, 0, false, { "eeee..eee..eeee..eeeee.eeee.eeee...ee.ee.eeeee.eee.eeee.eee.ee...e...ee..e", "ee.e.ee.ee.ee.e..ee.e..ee.e.e.eee..ee.e..ee.e..ee..e.eee.eeeee...ee..eee.e", ".e.e1e...e..e.e...e.e...e.e1e.e.e...e.e...e.e..1e..e.e.e1e.e.e..e.e...ee.e", "1a.a.a.5.a..ae11..a..1.1a.a..1a.1...a.a...a..1..a1..1a.11a.a.a..a1a1..aa.a", "2aa11a5c5a.2e.e..1ea...2aa1112e1...2aa22.1ea...12..12e1.121a.e..2222..a.aa", ".a.a.a.5.a.1a..a..a..1..a.a..1a.1...a.a...a..1..a...1a.1.a2e.a.1a2a...a.aa", "1a.a.a...a..a.1a..a.1a.1a.a...a1....a.a...a.1a..a....a1.1a.a.a..a.a...a.aa", "a21a.22.22.a2..2.a2.a1.a21a..a2....a2.a..a2.a1.a2...a2...a...2..a.2a.22..2", "22.2..222..2222..2222..22.2..2221..22.22.2222..222..2221222.22.22.22.22..2", "....2......................2..............................................", "....2......................2.............................................." } },
	{ 902, 2, 2, 158, 110, 98, 0, false, { "eeee..e...eeee..eee...eee....eee..ee.ee.eeeee.ee..e.eee.eee.", "ee.e..ee..ee.e..ee...ee.ee..ee.ee.ee.e..ee.e..eee.e.ee.ee.ee", ".e.e.e.e...e.e..1e...e...e..e......e.e...e.e...ee.e.1e.ee...", ".a.a.a1a1..ae11..a1..a.5.a..a15....a.a...a..1..aa.a..a1.aa..", "2aa..2222.2e.e..22...a5c5a..a5caa.2aa22.1ea....a.aa.12...aa.", "1a..1a2a..1a..a.1a11.a.5.a..a15.a..a.a...a..1..a.aa..a....aa", ".a.1.a.a...a.1a..a1..a...a..a...a..a.a...a.1a..a.aa..a.a..aa", "a2...a.2a.a2..2.a2.a.22.22..22.22.a2.a..a2.a1.22..2.a2.22.22", "222.22.22.2222..2222..222....222..22.22.2222..22..2.222.222.", "............................................................", "............................................................" } },
	{ 902, 2, 3, 158, 150, 74, 0, false, { ".eee..eeee..ee.e..e...eeee.eeee....eee...eee..eee..eee....eee..eeee..eeeee.eee.ee..e", "ee.ee.e.eee.ee.e..ee..ee.e.e.eee..ee.ee.ee.ee.ee...ee.e..ee.ee.e.eee.ee.e..ee..eee.e", "ee....e.e.e.e..e.e.e...e.e1e.e.e..e.....e...e.1e....e.e..ee....e.e.e..e.e..1e...ee.e", ".aa....1a.1.a.5a.a1a1.1a.a..1a.1..a15...a.5.a..a1...a5a...aa....1a.1..a..1..a1..aa.a", "..aa..12e1..e.c2.2222.2aa1112e1...a5caa.a5c5a.22...2ec5a...aa..12e1..1ea...12...a.aa", "...aa..1a.1.a.5a1a2a...a.a..1a.1..a15.a.a.5.a.1a11.1a5.a....aa..1a.1..a..1..a...a.aa", "a..aa...a1..a.1a.a.a..1a.a...a1...a...a.a...a..a1...a.1a.a..aa...a1...a.1a..a...a.aa", "22.22..a2...1aa2.a.2a.a21a..a2....22.22.22.22.a2.a.a2.2..22.22..a2...a2.a1.a2..22..2", ".222...2221..22122.22.22.2..2221...222...222..2222.2222...222...2221.2222..222.22..2", "..........................2.........................................................", "..........................2........................................................." } },
	{ 902, 3, 1, 158, 107, 100, 0, false, { "eeeee.eeeee.eeee.eeee..e..e..eee.ee..eee...eee..eeee.eeeee", "e..e..ee.e..ee.e.ee.e..e.ee...eeeee.ee.ee.ee.ee.ee.e.ee.e.", "a..e...e.e...e.e1.e.e1.e.e1..1e.e.e.e...e.e...e..e.e1.e.e.", "25.1...a..1.1a.a.1a.a..a.a1..1a.a.a.a.5.a.a.5.a.1a.a..a..1", ".55e..1ea...2aa112aa11.a.2...121a.e.a5c5a.a5c5a.2aa111ea..", "15.a...a..1..a.a..a.a...aa1...a2e.a.a.5.a.a.5.a..a.a..a..1", "...a...a.1a.1a.a.1a.a..1.a1..1a.a.a.a...a.a...a.1a.a..a.1a", "a..a1.a2.a1.a21a.a21a..a1a....a...2.22.22.22.22.a21a.a2.a1", "a2.a2.2222..22.2.22.2..a.2...222.22..222...222..22.2.2222.", "a2a2............2....2.a.a..........................2.....", ".22.............2....2..22..........................2....." } },
	{ 902, 3, 2, 158, 109, 100, 0, false, { "eee....e...eeee.eeee..e..e...eee...eee...eee..eeee..eeee.", "ee.....ee..ee.e.ee.e..e.ee..ee.ee.ee.e..ee.ee.e.eee.e.eee", "1e....e.e...e.e1.e.e1.e.e1..ee....e..a1.e...e.e.e.e.e.e.e", ".a1...a1a1.1a.a.1a.a..a.a1...aa...e152..a.5.a..1a.1..1a.1", "22....2222.2aa112aa11.a.2.....aa..a5c11.a5c5a.12e1..12e1.", "1a11.1a2a...a.a..a.a...aa1.....aa.a15...a.5.a..1a.1..1a.1", ".a1...a.a..1a.a.1a.a..1.a1..a..aa.21.1a.a...a...a1....a1.", "a2.a..a.2a.a21a.a21a..a1a...22.22.221a2.22.22..a2....a2..", "2222.22.22.22.2.22.2..a.2....222...122...222...2221..2221", "...............2....2.a.a................................", "...............2....2..22................................" } },
	{ 902, 3, 3, 158, 97, 104, 0, false, { "eeeee.eeeee.eeee.eeee..e..e...eee..ee.ee...e...ee..ee", "e..e..ee.e..ee.e.ee.e..e.ee..ee.ee.ee.e....ee..ee...e", "a..e...e.e...e.e1.e.e1.e.e1..ee.....e.e...e.e...e.a.e", "25.1...a..1.1a.a.1a.a..a.a1...aa....a.a...a1a1..a.2.a", ".55e..1ea...2aa112aa11.a.2.....aa..2aa22..2222.2e.a.e", "15.a...a..1..a.a..a.a...aa1.....aa..a.a..1a2a..1a.a.a", "...a...a.1a.1a.a.1a.a..1.a1..a..aa..a.a...a.a...a.21a", "a..a1.a2.a1.a21a.a21a..a1a...22.22.a2.a...a.2a..22222", "a2.a2.2222..22.2.22.2..a.2....222..22.22.22.22..2..2.", "a2a2............2....2.a.a...........................", ".22.............2....2..22..........................." } },
	{ 902, 4, 0, 106, 58, 74, 0, false, { ".eee..ee.e.eeee.ee.e.ee.ee..e..", "ee.ee.ee.e.ee.e.ee.e.ee.e...ee.", "e.....e..e..e.e1e..e..e.e1.e.e.", "a15...a.5a.1a.a.a.5a.1aa.1.a1a1", "a5caa.e.c2.2aa11e.c2.2aa12.2222", "a15.a.a.5a..a.a.a.5a.1aa.11a2a.", "a...a.a.1a.1a.a.a.1a..a.a1.a.a.", "22.22.1aa2.a21a.1aa2.a21a..a.2a", ".222...221.22.2..221.22.2.22.22", "...............2.........2.....", "...............2.........2....." } },
	{ 902, 4, 1, 159, 44, 132, 0, false, { ".eee..eee.ee..e..eee..ee.ee", "ee.ee.ee..eee.e.ee.ee.ee.e.", "ee....1e...ee.e.e......e.e.", ".aa....a1..aa.a.a15....a.a.", "..aa..12...a.aa.a5caa.2aa22", "...aa..a...a.aa.a15.a..a.a.", "a..aa..a...a.aa.a...a..a.a.", "22.22.a2..22..2.22.22.a2.a.", ".222..222.22..2..222..22.22", "...........................", "..........................." } },
	{ 902, 4, 2, 211, 58, 178, 0, false, { "ee.ee.ee.ee...e...eee...eee...e..", "ee.e..ee.e....ee..ee...ee.ee..ee.", ".e.e1..e.e...e.e..1e...ee....e.e.", "1aa.1..a.a...a1a1..a1...aa...a1a1", "2aa12.2aa22..2222.22.....aa..2222", "1aa.1..a.a..1a2a..1a11....aa1a2a.", ".a.a1..a.a...a.a...a1..a..aa.a.a.", "a21a..a2.a...a.2a.a2.a.22.22.a.2a", "22.2..22.22.22.22.2222..222.22.22", "....2............................", "....2............................" } },
	{ 902, 4, 3, 159, 113, 96, 0, false, { "ee.ee.eeeee.ee..e..ee..ee.eee.eee..eee..eee...e...eee.ee..eee.", "ee.e..ee.e..eee.e..ee...e.ee..ee...ee...ee....ee...eeeee.ee.ee", ".e.e1..e.e...ee.e...e.a.e.1e..1e...1e...1e...e.e..1e.e.e.ee...", "1aa.1..a..1..aa.a...a.2.a..a1..a1...a1...a1..a1a1.1a.a.a..aa..", "2aa12.1ea....a.aa..2e.a.e.12..22...22...12...2222.121a.e...aa.", "1aa.1..a..1..a.aa..1a.a.a..a..1a11.1a11..a..1a2a...a2e.a....aa", ".a.a1..a.1a..a.aa...a.21a..a...a1...a1...a...a.a..1a.a.a.a..aa", "a21a..a2.a1.22..2...22222.a2..a2.a.a2.a.a2...a.2a..a...2.22.22", "22.2..2222..22..2...2..2..222.2222.2222.222.22.22.222.22..222.", "....2.........................................................", "....2........................................................." } },
	{ 902, 5, 1, 159, 114, 96, 0, false, { "eee.ee...e...eeee.ee.ee...eee..eeeee.eee.eeee..eeeee.eeee.eeee.", ".eeeee...ee..ee.e.ee.e...ee.ee.ee.e..ee..ee.e..ee.e..ee.e.e.eee", "1e.e.e..e.e...e.e1.e.e1..ee.....e.e..1e...e.e...e.e...e.e1e.e.e", "1a.a.a..a1a1.1a.a.1aa.1...aa....a..1..a1..ae11..a..1.1a.a..1a.1", "121a.e..2222.2aa112aa12....aa..1ea...12..2e.e..1ea...2aa1112e1.", ".a2e.a.1a2a...a.a.1aa.1.....aa..a..1..a..1a..a..a..1..a.a..1a.1", "1a.a.a..a.a..1a.a..a.a1..a..aa..a.1a..a...a.1a..a.1a.1a.a...a1.", ".a...2..a.2a.a21a.a21a...22.22.a2.a1.a2..a2..2.a2.a1.a21a..a2..", "222.22.22.22.22.2.22.2....222..2222..222.2222..2222..22.2..2221", ".................2....2..................................2.....", ".................2....2..................................2....." } },
	{ 902, 5, 2, 158, 117, 96, 0, false, { ".eee..eee.ee..e.eee...e..e..ee..ee..e...eee..ee.ee.eeeee.eeee.", "ee.e..ee..eee.e.ee.e..e.ee..ee...e..ee..ee...ee.e..ee.e..ee.e.", "e..a1.1e...ee.e..e.e..e.e1...e.a.e.e.e..1e....e.e1..e.e...e.e1", "e152...a1..aa.a..a5a..a.a1...a.2.a.a1a1..a1..1aa.1..a..1.1a.a.", "a5c11.12...a.aa.2ec5a.a.2...2e.a.e.2222.22...2aa12.1ea...2aa11", "a15....a...a.aa.1a5.a..aa1..1a.a.a1a2a..1a11.1aa.1..a..1..a.a.", "21.1a..a...a.aa..a.1a.1.a1...a.21a.a.a...a1...a.a1..a.1a.1a.a.", "221a2.a2..22..2.a2.2..a1a....22222.a.2a.a2.a.a21a..a2.a1.a21a.", ".122..222.22..2.2222..a.2....2..2.22.22.2222.22.2..2222..22.2.", "......................a.a........................2...........2", ".......................22........................2...........2" } },
	{ 506, 1, 3, 65, 68, 26, 0, false, { "eeeee.eee.eee..ee.ee.eeee..eeeee.eeee.", "ee.e..ee.ee.ee.ee.e..e.eee.ee.e..ee.e.", ".e.e..1e.e......e.e..e.e.e..e.e...e.e1", ".a..1..a1a15....a.a...1a.1..a..1.1a.a.", "1ea...12.a5caa.2aa22.12e1..1ea...2aa11", ".a..1..a.a15.a..a.a...1a.1..a..1..a.a.", ".a.....a.a...a..a.a....a1...a.1a.1a.a.", "a2....a2.22.22.a2.a...a2...a2.a1.a21a.", "222...222.222..22.22..2221.2222..22.2.", ".....................................2", ".....................................2" } },
	{ 506, 1, 4, 159, 47, 132, 0, false, { "eee.ee...e....eee..eee.eee.", ".eeeee...ee..ee.ee.ee.ee.e.", "1e.e.e..e.e..e.....1e.e..a1", "1a.a.a..a1a1.a15....a1e152.", "121a.e..2222.a5caa.12.a5c11", ".a2e.a.1a2a..a15.a..a.a15..", "1a.a.a..a.a..a...a..a.21.1a", ".a...2..a.2a.22.22.a2.221a2", "222.22.22.22..222..222.122." } },
	{ 506, 1, 5, 252, 45, 224, 0, false, { "eeee..ee.ee.eee.eeeee.eeeee", "e.eee.ee.e..ee..ee.e..ee.e.", "e.e.e..e.e..1e...e.e...e.e.", ".1a.1..a.a...a1..a..1..a..1", "12e1..2aa22.12..1ea...1ea..", ".1a.1..a.a...a...a..1..a..1", "..a1...a.a...a...a.1a..a...", ".a2...a2.a..a2..a2.a1.a2...", ".2221.22.22.222.2222..222.." } },
	{ 506, 1, 6, 161, 40, 138, 0, false, { "ee.e..eee..eeeee.eeee.", "ee.e.ee.ee.ee.e..ee.e.", "e..e.ee.....e.e...e.e1", "a.5a..aa....a..1.1a.a.", "e.c2...aa..1ea...2aa11", "a.5a....aa..a..1..a.a.", "a.1a.a..aa..a.1a.1a.a.", "1aa2.22.22.a2.a1.a21a.", ".221..222..2222..22.2.", ".....................2", ".....................2" } },
	{ 902, 0, 1, 156, 17, 146, 0, true, { "cccc..c..c", "cc.c..c.cc", ".c.c..c.c1", ".dc11.d.d1", "5c.c..d.5.", "1d..d..dd1", ".d.1d.1.d1", "d5..5.d1d.", "5555..d.5.", "......d.d.", ".......55." } },
	{ 913, 1, 0, 50, 88, 14, 0, false, { "bbbbbbbb..........bbb.bbb..............", "8dddddd8..........8d8.8d8..............", "8dddddd8..........8d8.8d8..............", "8dd88888..........8d8.8d8..............", "8dd...............8d8.8d8..............", "8dd...............8dbbbd8..............", "8ddbbbbb.bbbbbb...8ddddd8.bbbbbb.bbbbbb", "8dddddd8.8dddd8...8ddddd8.8dddd8.8d88d8", "8dddddd8.8dddd8...888d888.8dddd8.8d88d8", "888888d8.8d88d8.....8d8...8d88d8.8d88d8", ".....8d8.8d88d8.....8d8...8d88d8.8d88d8", ".....8d8.8d88d8.....8d8...8d88d8.8d88d8", "bbbbb8d8.8dbbd8.....8d8...8dbbd8.8dbbd8", "8dddddd8.8dddd8.....8d8...8dddd8.8dddd8", "8dddddd8.8dddd8.....8d8...8dddd8.8dddd8", "b8888888.b88888.....b88...b88888.b88888", "88888888.888888.....888...888888.888888" } },
	{ 913, 1, 1, 127, 66, 98, 0, false, { ".............................", "bbb..bbb.....................", "8d8..8d8.....................", "8d8..8d8.....................", "8d8..8d8.....................", "8d8..8d8.....................", "8d8..8d8...bb...bbbbbb.bbbbbb", "8d8bb8d8..bbdb..8d88d8.8ddddd", "8d88d8d8..bd8d..8d88d8.888d88", "8d88d8d8.8d88d8.8d88d8...8d..", "8d88d8d8.8d88d8.8dd8d8...8d..", "8d88d8d8.8dbbd8.8ddbd8...8d..", "8d88d8d8.8dddd8.8d8dd8...8d..", "8dbbdbd8.8d88d8.8d8dd8...8d..", "8dddddd8.8d88d8.8d88d8...8d..", "bdddddd8.bd88d8.bd88d8..b8db.", "88888888.888888.888888..8888." } },
	{ 913, 1, 2, 209, 98, 162, 0, false, { "bbbbbbbb.........bbbbbb..........bbbbbbb", "8dddddd8.........8ddddd..........8ddddd8", "ddddddd..........8ddddd..........8ddddd.", "888dd88..........8d888d..........8d888d.", "..8dd............8d8.8d..........8d8.8d.", "..8dd............8d8.8d..........8d8.8d.", "..8dd..bbbbbbb...8d8.8d.bbbbbb...8d8.8d.", "..8dd..88dddd8...8dbbbd.8dddd8...8dbbbd.", "..8dd...8dddd....8dddd..8dddd8...8ddddd.", "..8dd...8d88d....8ddddd.8d8888...8ddddd.", "..8dd...8d.8d....8d888d.8dbb.....8d888d.", "..8dd...8d.8d....8d8.8d.8ddd.....8d8.8d.", "..8dd...8dbbd....8d8.8d.8d88.....8d8.8d.", "..8dd...8dddd....8dbbbd.8dbbb8...8d8.8d.", "..8dd...8dddd....8ddddd.8dddd8...8d8.8d.", "..b88b.b88888b...bddddd.bdddd8...bd8.8db", "..8888.8888888...888888.888888...8888888" } },
	{ 913, 1, 3, 286, 56, 248, 0, false, { "bbb.bbb.....................", "8d8.8d8.....................", "8d8.8d8.....................", "8d8.8d8.....................", "8d8.8d8.....................", "8d8.8d8.....................", "8dbbbd8.bbbbbb.bbbbbb.bbbbbb", "8ddddd8.8dddd8.8dddd8.8dddd8", "8ddddd8.8dddd8.8dddd8.8dddd8", "8d888d8.8d8888.8d88d8.8d88d8", "8d8.8d8.8dbb...8dbbd8.8d88d8", "8d8.8d8.8ddd...8dddd8.8d88d8", "8d8.8d8.8d88...8d8db..8dbbd8", "8d8.8d8.8dbbb8.8d88d..8dddd8", "8d8.8d8.8dddd8.8d88d..8dddd8", "bd8.bd8.bdddd8.bd88db.b88888", "888.888.888888.888888.888888" } },
	{ 902, 3, 0, 155, 7, 0, 0, false, {  } },
};

static const AgiDemakeQfg1Art *agiDemakeQfg1FindArt(GuiResourceId view, int16 loop, int16 cel) {
	if (g_sci->getGameId() != GID_QFG1)
		return nullptr;
	for (uint i = 0; i < ARRAYSIZE(kAgiDemakeQfg1Art); ++i)
		if (kAgiDemakeQfg1Art[i].view == view && kAgiDemakeQfg1Art[i].loop == loop && kAgiDemakeQfg1Art[i].cel == cel)
			return &kAgiDemakeQfg1Art[i];
	return nullptr;
}

// The cel's width once resized to its lettering: 2 pixels per fat pixel, plus 2 spare so a cel
// placed on an odd column still holds every fat pixel
static int agiDemakeQfg1ArtWidth(const AgiDemakeQfg1Art &art) {
	return art.rows[0] ? 2 * (int)strlen(art.rows[0]) + 2 : art.from;
}


GfxView::GfxView(ResourceManager *resMan, GfxScreen *screen, GfxPalette *palette, GuiResourceId resourceId)
	: _resMan(resMan), _screen(screen), _palette(palette), _resourceId(resourceId) {
	assert(resourceId != -1);
	_coordAdjuster = g_sci->_gfxCoordAdjuster;
	initData();
}

GfxView::~GfxView() {
	_loop.clear();
	_resMan->unlockResource(_resource);
}

static const byte EGAmappingStraight[SCI_VIEW_EGAMAPPING_SIZE] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

static const byte ViewInject_LauraBow2_Both[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x37,0x37,0x37,0x37,0x37,0x00,0x00,0x00,0x37,0x37,0x37,0x37,0x00,0x00,0x37,0x37,0x37,0x37,0x37,0x37,0x00,0x37,0x37,0x00,0x00,0x37,0x37,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x37,0x08,0x08,0x08,0x08,0x37,0x00,0x37,0x37,0x08,0x08,0x08,0x32,0x00,0x37,0x08,0x08,0x08,0x08,0x08,0x32,0x37,0x08,0x32,0x00,0x37,0x08,0x32,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x37,0x08,0x32,0x32,0x00,0x08,0x32,0x37,0x08,0x32,0x32,0x00,0x08,0x32,0x00,0x00,0x32,0x08,0x32,0x32,0x32,0x37,0x08,0x32,0x00,0x37,0x08,0x32,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x37,0x08,0x32,0x00,0x37,0x08,0x32,0x37,0x08,0x32,0x00,0x37,0x08,0x32,0x00,0x00,0x37,0x08,0x32,0x00,0x00,0x37,0x08,0x00,0x37,0x37,0x08,0x32,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x37,0x08,0x32,0x08,0x08,0x32,0x00,0x37,0x08,0x32,0x00,0x37,0x08,0x32,0x00,0x00,0x37,0x08,0x32,0x00,0x00,0x37,0x08,0x08,0x08,0x08,0x08,0x32,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x37,0x08,0x32,0x32,0x00,0x08,0x32,0x37,0x08,0x32,0x00,0x37,0x08,0x32,0x00,0x00,0x37,0x08,0x32,0x00,0x00,0x37,0x08,0x32,0x32,0x37,0x08,0x32,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x37,0x08,0x00,0x37,0x37,0x08,0x32,0x37,0x08,0x00,0x37,0x37,0x08,0x32,0x00,0x00,0x37,0x08,0x32,0x00,0x00,0x37,0x08,0x32,0x00,0x37,0x08,0x32,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x37,0x08,0x08,0x08,0x08,0x32,0x00,0x00,0x37,0x08,0x08,0x08,0x32,0x00,0x00,0x00,0x37,0x08,0x32,0x00,0x00,0x37,0x08,0x32,0x00,0x37,0x08,0x32,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x32,0x32,0x32,0x32,0x00,0x00,0x00,0x00,0x32,0x32,0x32,0x00,0x00,0x00,0x00,0x00,0x32,0x32,0x00,0x00,0x00,0x32,0x32,0x00,0x00,0x32,0x32,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static const byte ViewInject_KingsQuest6_Both1[] = {
	0x17,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x13,
	0x17,0x17,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x13,0x11,
	0x16,0x17,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x11,0x11,
	0x16,0x16,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x17,0x16,0x16,0x16,0x16,0x13,0x13,0x13,0x17,0x16,0x16,0x16,0x13,0x13,0x17,0x16,0x16,0x16,0x16,0x16,0x13,0x17,0x16,0x13,0x13,0x17,0x16,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x11,0x11,
	0x16,0x16,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x16,0x10,0x10,0x10,0x10,0x16,0x13,0x16,0x16,0x10,0x10,0x10,0x11,0x13,0x16,0x10,0x10,0x10,0x10,0x10,0x11,0x16,0x10,0x11,0x13,0x16,0x10,0x11,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x11,0x11,
	0x16,0x16,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x16,0x10,0x11,0x11,0x13,0x10,0x11,0x16,0x10,0x11,0x11,0x13,0x10,0x11,0x13,0x13,0x11,0x10,0x11,0x11,0x11,0x16,0x10,0x11,0x13,0x16,0x10,0x11,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x11,0x11,
	0x16,0x16,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x16,0x10,0x11,0x13,0x16,0x10,0x11,0x16,0x10,0x11,0x13,0x16,0x10,0x11,0x13,0x13,0x16,0x10,0x11,0x13,0x13,0x16,0x10,0x13,0x16,0x16,0x10,0x11,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x11,0x11,
	0x16,0x16,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x16,0x10,0x11,0x10,0x10,0x11,0x13,0x16,0x10,0x11,0x13,0x16,0x10,0x11,0x13,0x13,0x16,0x10,0x11,0x13,0x13,0x16,0x10,0x10,0x10,0x10,0x10,0x11,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x11,0x11,
	0x16,0x16,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x16,0x10,0x11,0x11,0x13,0x10,0x11,0x16,0x10,0x11,0x13,0x16,0x10,0x11,0x13,0x13,0x16,0x10,0x11,0x13,0x13,0x16,0x10,0x11,0x11,0x13,0x10,0x11,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x11,0x11,
	0x16,0x16,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x16,0x10,0x13,0x16,0x16,0x10,0x11,0x16,0x10,0x13,0x16,0x16,0x10,0x11,0x13,0x13,0x16,0x10,0x11,0x13,0x13,0x16,0x10,0x11,0x13,0x16,0x10,0x11,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x11,0x11,
	0x16,0x16,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x16,0x10,0x10,0x10,0x10,0x11,0x13,0x13,0x16,0x10,0x10,0x10,0x11,0x13,0x13,0x13,0x16,0x10,0x11,0x13,0x13,0x16,0x10,0x11,0x13,0x16,0x10,0x11,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x11,0x11,
	0x16,0x16,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x11,0x11,0x11,0x11,0x13,0x13,0x13,0x13,0x11,0x11,0x11,0x13,0x13,0x13,0x13,0x13,0x11,0x11,0x13,0x13,0x13,0x11,0x11,0x13,0x13,0x11,0x11,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x11,0x11,
	0x16,0x16,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x11,0x11,
	0x16,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
	0x13,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x11
};

static const byte ViewInject_KingsQuest6_Both2[] = {
	0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
	0x10,0x16,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x11,0x10,
	0x10,0x13,0x16,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x11,0x10,0x10,
	0x10,0x13,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x16,0x13,0x13,0x13,0x13,0x11,0x11,0x11,0x16,0x13,0x13,0x13,0x11,0x11,0x16,0x13,0x13,0x13,0x13,0x13,0x11,0x16,0x13,0x11,0x11,0x16,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,0x10,
	0x10,0x13,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x13,0x16,0x16,0x16,0x16,0x13,0x11,0x13,0x13,0x16,0x16,0x16,0x13,0x11,0x13,0x16,0x16,0x16,0x16,0x16,0x10,0x13,0x16,0x10,0x11,0x13,0x16,0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,0x10,
	0x10,0x13,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x13,0x16,0x10,0x10,0x11,0x16,0x10,0x13,0x16,0x11,0x10,0x13,0x16,0x10,0x11,0x11,0x10,0x16,0x10,0x10,0x10,0x13,0x16,0x10,0x11,0x13,0x16,0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,0x10,
	0x10,0x13,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x13,0x16,0x10,0x11,0x13,0x16,0x10,0x13,0x16,0x10,0x11,0x13,0x16,0x10,0x11,0x11,0x13,0x16,0x10,0x11,0x11,0x13,0x16,0x11,0x13,0x13,0x16,0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,0x10,
	0x10,0x13,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x13,0x16,0x10,0x16,0x16,0x10,0x10,0x13,0x16,0x10,0x11,0x13,0x16,0x10,0x11,0x11,0x13,0x16,0x10,0x11,0x11,0x13,0x16,0x16,0x16,0x16,0x16,0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,0x10,
	0x10,0x13,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x13,0x16,0x10,0x10,0x11,0x16,0x10,0x13,0x16,0x10,0x11,0x13,0x16,0x10,0x11,0x11,0x13,0x16,0x10,0x11,0x11,0x13,0x16,0x10,0x10,0x11,0x16,0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,0x10,
	0x10,0x13,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x13,0x16,0x11,0x13,0x13,0x16,0x10,0x13,0x16,0x11,0x13,0x13,0x16,0x10,0x11,0x11,0x13,0x16,0x10,0x11,0x11,0x13,0x16,0x10,0x11,0x13,0x16,0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,0x10,
	0x10,0x13,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x13,0x16,0x16,0x16,0x16,0x10,0x11,0x11,0x13,0x16,0x16,0x16,0x10,0x11,0x11,0x11,0x13,0x16,0x10,0x11,0x11,0x13,0x16,0x10,0x11,0x13,0x16,0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,0x10,
	0x10,0x13,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,0x10,0x10,0x10,0x11,0x11,0x11,0x11,0x10,0x10,0x10,0x11,0x11,0x11,0x11,0x11,0x10,0x10,0x11,0x11,0x11,0x10,0x10,0x11,0x11,0x10,0x10,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,0x10,
	0x10,0x13,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,0x10,
	0x10,0x11,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
	0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10
};

void GfxView::initData() {
	_resource = _resMan->findResource(ResourceId(kResourceTypeView, _resourceId), true);
	if (!_resource) {
		error("view resource %d not found", _resourceId);
	}

	_loop.resize(0);
	_embeddedPal = false;
	_EGAmapping.clear();
	_isScaleable = true;

	// we adjust inside getCelRect for SCI0EARLY (that version didn't have the +1 when calculating bottom)
	_adjustForSci0Early = getSciVersion() == SCI_VERSION_0_EARLY ? -1 : 0;

	// If we find an SCI1/SCI1.1 view (not amiga), we switch to that type for
	// EGA. This could get used to make view patches for EGA games, where the
	// new views include more colors. Users could manually adjust old views to
	// make them look better (like removing dithered colors that aren't caught
	// by our undithering or even improve the graphics overall).
	ViewType curViewType = _resMan->getViewType();
	if (curViewType == kViewEga) {
		if (_resource->getUint8At(1) == 0x80) {
			curViewType = kViewVga;
		} else if (_resource->getUint16LEAt(4) == 1) {
			curViewType = kViewVga11;
		}
	}

	bool isEGA = false;
	switch (curViewType) {
	case kViewEga: // SCI0 (and Amiga 16 colors)
		isEGA = true;
		// fall through
	case kViewAmiga: // Amiga ECS (32 colors)
	case kViewAmiga64: // Amiga AGA (64 colors)
	case kViewVga: { // View-format SCI1
		// LoopCount:BYTE Flags:BYTE MirrorMask:WORD Version:WORD PaletteOffset:WORD LoopOffset0:WORD LoopOffset1:WORD...

		_loop.resize(_resource->getUint8At(0));
		// bit 0x8000 of _resourceData[1] means palette is set
		bool isCompressed = true;
		if (_resource->getUint8At(1) & 0x40)
			isCompressed = false;
		uint16 mirrorBits = _resource->getUint16LEAt(2);
		uint16 palOffset = _resource->getUint16LEAt(6);

		if (palOffset && palOffset != 0x100) {
			// Some SCI0/SCI01 games also have an offset set. It seems that it
			// points to a 16-byte mapping table but on those games using that
			// mapping will actually screw things up. On the other side: VGA
			// SCI1 games have this pointing to a VGA palette and EGA SCI1 games
			// have this pointing to a 8x16 byte mapping table that needs to get
			// applied then.
			if (!isEGA) {
				_palette->createFromData(_resource->subspan(palOffset), &_viewPalette);
				_embeddedPal = true;
			} else {
				// Only use the EGA-mapping, when being SCI1 EGA
				//  SCI1 VGA conversion games (which will get detected as SCI1EARLY/MIDDLE/LATE) have some views
				//  with broken mapping tables. I guess those games won't use the mapping, so I rather disable it
				//  for them
				if (getSciVersion() == SCI_VERSION_1_EGA_ONLY) {
					uint EGAmapNr;
					for (EGAmapNr = 0; EGAmapNr < SCI_VIEW_EGAMAPPING_COUNT; EGAmapNr++) {
						const SciSpan<const byte> mapping = _resource->subspan(palOffset + EGAmapNr * SCI_VIEW_EGAMAPPING_SIZE, SCI_VIEW_EGAMAPPING_SIZE);
						if (memcmp(mapping.getUnsafeDataAt(0, SCI_VIEW_EGAMAPPING_SIZE), EGAmappingStraight, SCI_VIEW_EGAMAPPING_SIZE) != 0)
							break;
					}
					// If all mappings are "straight", then we actually ignore the mapping
					if (EGAmapNr == SCI_VIEW_EGAMAPPING_COUNT)
						_EGAmapping.clear();
					else
						_EGAmapping = _resource->subspan(palOffset, SCI_VIEW_EGAMAPPING_COUNT * SCI_VIEW_EGAMAPPING_SIZE);
				}
			}
		}

		for (uint loopNo = 0; loopNo < _loop.size(); loopNo++) {
			SciSpan<const byte> loopData = _resource->subspan(_resource->getUint16LEAt(8 + loopNo * 2));
			// CelCount:WORD Unknown:WORD CelOffset0:WORD CelOffset1:WORD...

			uint16 celCount = loopData.getUint16LEAt(0);
			_loop[loopNo].cel.resize(celCount);
			_loop[loopNo].mirrorFlag = mirrorBits & 1 ? true : false;
			mirrorBits >>= 1;

			// read cel info
			for (uint celNo = 0; celNo < celCount; celNo++) {
				uint16 celOffset = loopData.getUint16LEAt(4 + celNo * 2);
				SciSpan<const byte> celData = _resource->subspan(celOffset);

				// For VGA
				// Width:WORD Height:WORD DisplaceX:BYTE DisplaceY:BYTE ClearKey:BYTE Unknown:BYTE RLEData starts now directly
				// For EGA
				// Width:WORD Height:WORD DisplaceX:BYTE DisplaceY:BYTE ClearKey:BYTE EGAData starts now directly
				CelInfo *cel = &_loop[loopNo].cel[celNo];
				cel->scriptWidth = cel->width = celData.getUint16LEAt(0);
				cel->scriptHeight = cel->height = celData.getUint16LEAt(2);
				cel->displaceX = (signed char)celData[4];
				cel->displaceY = celData[5];
				cel->clearKey = celData[6];

				// HACK: Fix Ego's odd displacement in the QFG3 demo, scene 740.
				// For some reason, ego jumps above the rope, so we fix his rope
				// hanging view by displacing it down by 40 pixels. Fixes bug
				// #5009.
				// FIXME: Remove this once we figure out why Ego jumps so high.
				// Likely culprits include kInitBresen, kDoBresen and kCantBeHere.
				// The scripts have the y offset that hero reaches (11) hardcoded,
				// so it might be collision detection. However, since this requires
				// extensive work to fix properly for very little gain, this hack
				// here will suffice until the actual issue is found.
				if (g_sci->getGameId() == GID_QFG3 && g_sci->isDemo() && _resourceId == 39)
					cel->displaceY = 98;

				if (isEGA) {
					cel->offsetEGA = celOffset + 7;
					cel->offsetRLE = 0;
					cel->offsetLiteral = 0;
				} else {
					cel->offsetEGA = 0;
					if (isCompressed) {
						cel->offsetRLE = celOffset + 8;
						cel->offsetLiteral = 0;
					} else {
						cel->offsetRLE = 0;
						cel->offsetLiteral = celOffset + 8;
					}
				}
				cel->rawBitmap.clear();
				if (_loop[loopNo].mirrorFlag)
					cel->displaceX = -cel->displaceX;
			}
		}
		break;
	}

	case kViewVga11: { // View-format SCI1.1+
		// HeaderSize:WORD LoopCount:BYTE Flags:BYTE Version:WORD Unknown:WORD PaletteOffset:WORD
		uint16 headerSize = _resource->getUint16SEAt(0) + 2; // headerSize is not part of the header, so it's added
		assert(headerSize >= 16);
		const uint8 loopCount = _resource->getUint8At(2);
		assert(loopCount);
		uint32 palOffset = _resource->getUint32SEAt(8);

		// flags is actually a bit-mask
		//  it seems it was only used for some early sci1.1 games (or even just laura bow 2)
		//  later interpreters dont support it at all anymore
		// we assume that if flags is 0h the view does not support flags and default to scalable
		// if it's 1h then we assume that the view is not to be scaled
		// if it's 40h then we assume that the view is scalable
		switch (_resource->getUint8At(3)) {
		case 1:
			_isScaleable = false;
			break;
		case 0x40:
		case 0:
			break; // don't do anything, we already have _isScaleable set
		default:
			error("unsupported flags byte (%d) inside sci1.1 view", _resource->getUint8At(3));
			break;
		}

		uint16 loopSize = _resource->getUint8At(12);
		assert(loopSize >= 16);
		uint16 celSize = _resource->getUint8At(13);
		assert(celSize >= 32);

		if (palOffset) {
			_palette->createFromData(_resource->subspan(palOffset), &_viewPalette);
			_embeddedPal = true;
		}

		_loop.resize(loopCount);
		for (uint loopNo = 0; loopNo < loopCount; loopNo++) {
			SciSpan<const byte> loopData = _resource->subspan(headerSize + (loopNo * loopSize));

			byte seekEntry = loopData[0];
			if (seekEntry != 255) {
				_loop[loopNo].mirrorFlag = true;

				// use the root loop for mirroring. this handles rare loops that
				//  mirror loops that mirror loops. (FPFP view 844, bug #10953)
				do {
					if (seekEntry >= loopCount)
						error("Bad loop-pointer in sci 1.1 view");
					loopData = _resource->subspan(headerSize + (seekEntry * loopSize));
				} while ((seekEntry = loopData[0]) != 255);
			} else {
				_loop[loopNo].mirrorFlag = false;
			}

			uint16 celCount = loopData[2];
			_loop[loopNo].cel.resize(celCount);

			const uint32 celDataOffset = loopData.getUint32SEAt(12);

			// read cel info
			for (uint celNo = 0; celNo < celCount; celNo++) {
				SciSpan<const byte> celData = _resource->subspan(celDataOffset + celNo * celSize, celSize);

				CelInfo *cel = &_loop[loopNo].cel[celNo];
				cel->scriptWidth = cel->width = celData.getInt16SEAt(0);
				cel->scriptHeight = cel->height = celData.getInt16SEAt(2);
				cel->displaceX = celData.getInt16SEAt(4);
				cel->displaceY = celData.getInt16SEAt(6);
				if (cel->displaceY < 0)
					cel->displaceY += 255; // sierra did this adjust in their sci1.1 getCelRect() - not sure about sci32

				assert(cel->width && cel->height);

				cel->clearKey = celData[8];
				cel->offsetEGA = 0;
				cel->offsetRLE = celData.getUint32SEAt(24);
				cel->offsetLiteral = celData.getUint32SEAt(28);

				// GK1-hires content is actually uncompressed, we need to swap both so that we process it as such
				if ((cel->offsetRLE) && (!cel->offsetLiteral))
					SWAP(cel->offsetRLE, cel->offsetLiteral);

				cel->rawBitmap.clear();
				if (_loop[loopNo].mirrorFlag)
					cel->displaceX = -cel->displaceX;
			}
		}
		break;
	}

	default:
		error("ViewType was not detected, can't continue");
	}

	// Inject our own views
	//  Currently only used for Dual mode (speech + text) for games, that do not have a "BOTH" icon already
	//  Which is Laura Bow 2 + King's Quest 6
	switch (g_sci->getGameId()) {
	case GID_LAURABOW2:
		// View 995, Loop 13, Cel 0 = "TEXT"
		// View 995, Loop 13, Cel 1 = "SPEECH"
		// View 995, Loop 13, Cel 2 = "BOTH" (<- our injected view)
		if (g_sci->isCD() && _resourceId == 995) {
			// security checks
			if (_loop.size() >= 14 &&
				_loop[13].cel.size() == 2 &&
				_loop[13].cel[0].width == 46 &&
				_loop[13].cel[0].height == 11) {

				_loop[13].cel.resize(3);
				// Duplicate cel 0 to cel 2
				_loop[13].cel[2] = _loop[13].cel[0];
				// use our data (which is uncompressed bitmap data)
				_loop[13].cel[2].rawBitmap->allocateFromSpan(SciSpan<const byte>(ViewInject_LauraBow2_Both, sizeof(ViewInject_LauraBow2_Both)));
			}
		}
		break;
	case GID_KQ6:
		// View 947, Loop 8, Cel 0 = "SPEECH" (not pressed)
		// View 947, Loop 8, Cel 1 = "SPEECH" (pressed)
		// View 947, Loop 9, Cel 0 = "TEXT" (not pressed)
		// View 947, Loop 9, Cel 1 = "TEXT" (pressed)
		// View 947, Loop 12, Cel 0 = "BOTH" (not pressed) (<- our injected view)
		// View 947, Loop 12, Cel 1 = "BOTH" (pressed) (<- our injected view)
		if (g_sci->isCD() && _resourceId == 947) {
			// security checks
			if (_loop.size() == 12 &&
				_loop[8].cel.size() == 2 &&
				_loop[8].cel[0].width == 50 &&
				_loop[8].cel[0].height == 15) {

				// add another loop
				_loop.resize(_loop.size() + 1);
				// copy loop 8 to loop 12
				_loop[12] = _loop[8];
				// use our data (which is uncompressed bitmap data)
				_loop[12].cel[0].rawBitmap->allocateFromSpan(SciSpan<const byte>(ViewInject_KingsQuest6_Both1, sizeof(ViewInject_KingsQuest6_Both1)));
				_loop[12].cel[1].rawBitmap->allocateFromSpan(SciSpan<const byte>(ViewInject_KingsQuest6_Both2, sizeof(ViewInject_KingsQuest6_Both2)));
			}
		}
		break;
	default:
		break;
	}

	// AGI demake, QfG1 Yorick's room (view 96): EXIT (loop 7 cel 3) 6 pixels longer on the left, so the
	// word fits; drawn in drawAgiDemake
	if (_screen->agiDemake() && g_sci->getGameId() == GID_QFG1 && _resourceId == 96 && _loop.size() > 7) {
		struct W { int loop, cel, from, to, move; };
		static const W kWiden[] = { { 7, 3, 33, 39, -3 } };
		for (uint i = 0; i < ARRAYSIZE(kWiden); ++i) {
			const W &w = kWiden[i];
			if ((int)_loop[w.loop].cel.size() <= w.cel || _loop[w.loop].cel[w.cel].width != w.from)
				continue;
			CelInfo &cel = _loop[w.loop].cel[w.cel];
			cel.width = w.to;
			cel.scriptWidth = w.to;
			cel.displaceX += w.move;
			SciSpan<byte> bits = cel.rawBitmap->allocate(cel.width * cel.height, "AGI demake Yorick sign");
			for (int j = 0; j < cel.width * cel.height; ++j)
				bits[j] = cel.clearKey;
		}
	}

	// AGI demake, QfG1 intro lettering (credits view 902, character select view 506 loop 1, PRESENTS view
	// 908 loop 1, title view 913 loop 1): the redrawn or re-spaced words are wider than Sierra's cels, so
	// those cels are widened (Sierra's picture copied in where it was on screen). A cel sits centred on
	// its x, so a word on its own stays centred. Words that share a line with another (WRITTEN & DIRECTED,
	// ANIMATION &, BACKGROUND SCENES, GAME DEVELOPMENT, CHOOSE YOUR CHARACTER) grow away from their
	// neighbour instead. Drawn in drawAgiDemake
	if (_screen->agiDemake() && g_sci->getGameId() == GID_QFG1 &&
			(_resourceId == 902 || _resourceId == 506 || _resourceId == 908 || _resourceId == 913)) {
		struct W { int view, loop, cel, from, to; char anchor; };
		static const W kWiden[] = {
			{ 908, 1, 0, 62, 66, 'C' },
			{ 908, 1, 1, 62, 66, 'C' },
			{ 908, 1, 2, 62, 66, 'C' },
			{ 908, 1, 3, 60, 64, 'C' },
			{ 908, 1, 4, 60, 64, 'C' },
			{ 908, 1, 5, 60, 64, 'C' },
			{ 908, 1, 6, 60, 66, 'C' },
			{ 908, 1, 7, 60, 80, 'C' },
		};
		for (uint i = 0; i < ARRAYSIZE(kWiden); ++i) {
			const W &w = kWiden[i];
			if (w.view != _resourceId || (int)_loop.size() <= w.loop || (int)_loop[w.loop].cel.size() <= w.cel ||
					_loop[w.loop].cel[w.cel].width != w.from)
				continue;
			const SciSpan<const byte> &orig = getBitmap(w.loop, w.cel);
			CelInfo &cel = _loop[w.loop].cel[w.cel];
			Common::Array<byte> copy(orig.getUnsafeDataAt(0, cel.width * cel.height), cel.width * cel.height);
			const int oldW = cel.width;
			// 'C' grows both sides (stays centred), 'R' only on the left (keeps its right edge), 'L' only on
			// the right (keeps its left edge): words side by side on one line keep Sierra's gaps between them
			const int grow = w.to - w.from;
			const int shift = (w.anchor == 'R') ? grow : (w.anchor == 'L') ? 0 : grow / 2;
			if (w.anchor == 'R')
				cel.displaceX -= grow / 2;
			else if (w.anchor == 'L')
				cel.displaceX += grow / 2;
			cel.width = w.to;
			cel.scriptWidth = w.to;
			// the cel was unpacked above to copy it, and a span can only be allocated when empty
			cel.rawBitmap.clear();
			SciSpan<byte> bits = cel.rawBitmap->allocate(cel.width * cel.height, "AGI demake QfG1 lettering");
			for (int y = 0; y < cel.height; ++y)
				for (int x = 0; x < cel.width; ++x) {
					const int sx = x - shift;
					bits[y * cel.width + x] = (sx >= 0 && sx < oldW) ? copy[y * oldW + sx] : cel.clearKey;
				}
		}
	}

	// AGI demake, QfG1 intro lettering drawn ready (kAgiDemakeQfg1Art): each cel is resized to its
	// lettering and moved to where it goes on screen. Its own picture is blanked, the lettering is
	// drawn in drawAgiDemake
	if (_screen->agiDemake() && g_sci->getGameId() == GID_QFG1 &&
			(_resourceId == 902 || _resourceId == 506 || _resourceId == 913)) {
		for (int l = 0; l < (int)_loop.size(); ++l)
			for (int c = 0; c < (int)_loop[l].cel.size(); ++c) {
				const AgiDemakeQfg1Art *art = agiDemakeQfg1FindArt(_resourceId, l, c);
				CelInfo &cel = _loop[l].cel[c];
				if (!art || cel.width != art->from)
					continue;
				const int to = agiDemakeQfg1ArtWidth(*art);
				cel.width = to;
				cel.scriptWidth = to;
				cel.displaceX = art->left - art->posnX + (to >> 1);
				cel.displaceY += art->down;
				cel.rawBitmap.clear();
				SciSpan<byte> bits = cel.rawBitmap->allocate(cel.width * cel.height, "AGI demake QfG1 lettering");
				for (int i = 0; i < cel.width * cel.height; ++i)
					bits[i] = cel.clearKey;
			}
	}

	// AGI demake, QfG1 dining hall sign (view 95 loop 0 cel 0): "BRIGANDS UBER ALLES!" doesn't fit on it in
	// fat letters, so the cel is made 6 rows taller (growing downwards) for two lines; the sign is drawn
	// in drawAgiDemake
	if (_screen->agiDemake() && g_sci->getGameId() == GID_QFG1 && _resourceId == 95 && _loop.size() > 0 &&
			_loop[0].cel.size() > 0 && _loop[0].cel[0].width == 100 && _loop[0].cel[0].height == 11) {
		CelInfo &cel = _loop[0].cel[0];
		cel.height = 17;
		cel.scriptHeight = 17;
		cel.displaceY += 6;
		SciSpan<byte> bits = cel.rawBitmap->allocate(cel.width * cel.height, "AGI demake brigand sign");
		for (int i = 0; i < cel.width * cel.height; ++i)
			bits[i] = cel.clearKey;
	}

	// AGI demake, QfG1 knife throwing (view 341 loop 5): the turn highlights laid over THIEF and CHIEF
	// are only as wide as Sierra's small letters. In fat letters the names are 34 pixels wide, so these
	// cels are made that wide (blank, the word is drawn in drawAgiDemake) and moved so they sit exactly
	// over the names on the panel
	if (_screen->agiDemake() && g_sci->getGameId() == GID_QFG1 && _resourceId == 341 && _loop.size() > 5 &&
			_loop[5].cel.size() >= 2 && _loop[5].cel[0].width == 20 && _loop[5].cel[1].width == 19) {
		static const int16 kMove[2] = { -2, 1 };
		for (int c = 0; c < 2; ++c) {
			CelInfo &cel = _loop[5].cel[c];
			cel.width = 34;
			cel.scriptWidth = 34;
			cel.displaceX += kMove[c];
			SciSpan<byte> bits = cel.rawBitmap->allocate(cel.width * cel.height, "AGI demake knife highlight");
			for (int i = 0; i < cel.width * cel.height; ++i)
				bits[i] = cel.clearKey;
		}
	}
}

GuiResourceId GfxView::getResourceId() const {
	return _resourceId;
}

int16 GfxView::getWidth(int16 loopNo, int16 celNo) const {
	return _loop.size() ? getCelInfo(loopNo, celNo)->width : 0;
}

int16 GfxView::getHeight(int16 loopNo, int16 celNo) const {
	return _loop.size() ? getCelInfo(loopNo, celNo)->height : 0;
}

const CelInfo *GfxView::getCelInfo(int16 loopNo, int16 celNo) const {
	assert(_loop.size());
	loopNo = CLIP<int16>(loopNo, 0, _loop.size() - 1);
	celNo = CLIP<int16>(celNo, 0, _loop[loopNo].cel.size() - 1);
	return &_loop[loopNo].cel[celNo];
}

uint16 GfxView::getCelCount(int16 loopNo) const {
	assert(_loop.size());
	loopNo = CLIP<int16>(loopNo, 0, _loop.size() - 1);
	return _loop[loopNo].cel.size();
}

Palette *GfxView::getPalette() {
	return _embeddedPal ? &_viewPalette : nullptr;
}

bool GfxView::isScaleable() {
	return _isScaleable;
}

void GfxView::getCelRect(int16 loopNo, int16 celNo, int16 x, int16 y, int16 z, Common::Rect &outRect) const {
	const CelInfo *celInfo = getCelInfo(loopNo, celNo);
	outRect.left = x + celInfo->displaceX - (celInfo->width >> 1);
	outRect.right = outRect.left + celInfo->width;
	outRect.bottom = y + celInfo->displaceY - z + 1 + _adjustForSci0Early;
	outRect.top = outRect.bottom - celInfo->height;
}

void GfxView::getCelSpecialHoyle4Rect(int16 loopNo, int16 celNo, int16 x, int16 y, int16 z, Common::Rect &outRect) const {
	const CelInfo *celInfo = getCelInfo(loopNo, celNo);
	int16 adjustY = y + celInfo->displaceY - celInfo->height + 1;
	int16 adjustX = x + celInfo->displaceX - ((celInfo->width - 1) >> 1);
	outRect.translate(adjustX, adjustY);
}

void GfxView::getCelScaledRect(int16 loopNo, int16 celNo, int16 x, int16 y, int16 z, int16 scaleX, int16 scaleY, Common::Rect &outRect) const {
	const CelInfo *celInfo = getCelInfo(loopNo, celNo);

	// Scaling displaceX/Y, Width/Height
	int16 scaledDisplaceX = (celInfo->displaceX * scaleX) / 128;
	int16 scaledDisplaceY = (celInfo->displaceY * scaleY) / 128;
	int16 scaledWidth = (celInfo->width * scaleX) >> 7;
	int16 scaledHeight = (celInfo->height * scaleY) >> 7;
	scaledWidth = CLIP<int16>(scaledWidth, 0, _screen->getWidth());
	scaledHeight = CLIP<int16>(scaledHeight, 0, _screen->getHeight());

	outRect.left = x + scaledDisplaceX - (scaledWidth >> 1);
	outRect.right = outRect.left + scaledWidth;
	outRect.bottom = y + scaledDisplaceY - z + 1;
	outRect.top = outRect.bottom - scaledHeight;
}

void unpackCelData(const SciSpan<const byte> &inBuffer, SciSpan<byte> &celBitmap, byte clearColor, int rlePos, int literalPos, ViewType viewType, uint16 width, bool isMacSci11View) {
	const int pixelCount = celBitmap.size();
	byte *outPtr = celBitmap.getUnsafeDataAt(0);
	byte curByte, runLength;
	// TODO: Calculate correct maximum dimensions
	const byte *rlePtr = inBuffer.getUnsafeDataAt(rlePos);
	// The existence of a literal position pointer signifies data with two
	// separate streams, most likely a SCI1.1 view
	const byte *literalPtr = inBuffer.getUnsafeDataAt(literalPos, inBuffer.size() - literalPos);
	const byte *const endOfResource = inBuffer.getUnsafeDataAt(inBuffer.size(), 0);
	int pixelNr = 0;
	(void)endOfResource;

	memset(celBitmap.getUnsafeDataAt(0), clearColor, celBitmap.size());

	// View unpacking:
	//
	// EGA:
	// Each byte is like XXXXYYYY (XXXX: 0 - 15, YYYY: 0 - 15)
	// Set the next XXXX pixels to YYYY
	//
	// Amiga:
	// Each byte is like XXXXXYYY (XXXXX: 0 - 31, YYY: 0 - 7)
	// - Case A: YYY != 0
	//   Set the next YYY pixels to XXXXX
	// - Case B: YYY == 0
	//   Skip the next XXXXX pixels (i.e. transparency)
	//
	// Amiga 64:
	// Each byte is like XXYYYYYY (XX: 0 - 3, YYYYYY: 0 - 63)
	// - Case A: XX != 0
	//   Set the next XX pixels to YYYYYY
	// - Case B: XX == 0
	//   Skip the next YYYYYY pixels (i.e. transparency)
	//
	// VGA:
	// Each byte is like XXYYYYYY (YYYYY: 0 - 63)
	// - Case A: XX == 00 (binary)
	//   Copy next YYYYYY bytes as-is
	// - Case B: XX == 01 (binary)
	//   Same as above, copy YYYYYY + 64 bytes as-is
	// - Case C: XX == 10 (binary)
	//   Set the next YYYYY pixels to the next byte value
	// - Case D: XX == 11 (binary)
	//   Skip the next YYYYY pixels (i.e. transparency)

	if (literalPos && isMacSci11View) {
		// KQ6/Freddy Pharkas/Slater use byte lengths, all others use uint16
		// The SCI devs must have realized that a max of 255 pixels wide
		// was not very good for 320 or 640 width games.
		bool hasByteLengths =
			g_sci->getGameId() == GID_KQ6 ||
			g_sci->getGameId() == GID_FREDDYPHARKAS ||
			g_sci->getGameId() == GID_SLATER;

		// compression for SCI1.1+ Mac
		while (pixelNr < pixelCount) {
			uint32 pixelLine = pixelNr;

			if (hasByteLengths) {
				assert (rlePtr + 2 <= endOfResource);
				pixelNr += *rlePtr++;
				runLength = *rlePtr++;
			} else {
				assert (rlePtr + 4 <= endOfResource);
				pixelNr += READ_BE_UINT16(rlePtr);
				runLength = READ_BE_UINT16(rlePtr + 2);
				rlePtr += 4;
			}

			assert(literalPtr + MIN<int>(runLength, pixelCount - pixelNr) <= endOfResource);
			while (runLength-- && pixelNr < pixelCount)
				outPtr[pixelNr++] = *literalPtr++;

			pixelNr = pixelLine + width;
		}
		return;
	}

	switch (viewType) {
	case kViewEga:
		while (pixelNr < pixelCount) {
			curByte = *rlePtr++;
			runLength = curByte >> 4;
			memset(outPtr + pixelNr, curByte & 0x0F, MIN<uint16>(runLength, pixelCount - pixelNr));
			pixelNr += runLength;
		}
		break;
	case kViewAmiga:
		while (pixelNr < pixelCount) {
			curByte = *rlePtr++;
			if (curByte & 0x07) { // fill with color
				runLength = curByte & 0x07;
				curByte = curByte >> 3;
				memset(outPtr + pixelNr, curByte, MIN<uint16>(runLength, pixelCount - pixelNr));
			} else { // skip the next pixels (transparency)
				runLength = curByte >> 3;
			}
			pixelNr += runLength;
		}
		break;
	case kViewAmiga64:
		while (pixelNr < pixelCount) {
			curByte = *rlePtr++;
			if (curByte & 0xC0) { // fill with color
				runLength = curByte >> 6;
				curByte = curByte & 0x3F;
				memset(outPtr + pixelNr, curByte, MIN<uint16>(runLength, pixelCount - pixelNr));
			} else { // skip the next pixels (transparency)
				runLength = curByte & 0x3F;
			}
			pixelNr += runLength;
		}
		break;
	case kViewVga:
	case kViewVga11:
		// If we have no RLE data, the image is just uncompressed
		if (rlePos == 0) {
			memcpy(outPtr, literalPtr, pixelCount);
			break;
		}

		while (pixelNr < pixelCount) {
			curByte = *rlePtr++;
			runLength = curByte & 0x3F;

			switch (curByte & 0xC0) {
			case 0x40: // copy bytes as is (In copy case, runLength can go up to 127 i.e. pixel & 0x40). Fixes bug #5551.
				runLength += 64;
				// fall through
			case 0x00: // copy bytes as-is
				if (!literalPos) {
					memcpy(outPtr + pixelNr,        rlePtr, MIN<uint16>(runLength, pixelCount - pixelNr));
					rlePtr += runLength;
				} else {
					memcpy(outPtr + pixelNr,    literalPtr, MIN<uint16>(runLength, pixelCount - pixelNr));
					literalPtr += runLength;
				}
				break;
			case 0x80: // fill with color
				if (!literalPos)
					memset(outPtr + pixelNr,     *rlePtr++, MIN<uint16>(runLength, pixelCount - pixelNr));
				else
					memset(outPtr + pixelNr, *literalPtr++, MIN<uint16>(runLength, pixelCount - pixelNr));
				break;
			case 0xC0: // skip the next pixels (transparency)
			default:
				break;
			}

			pixelNr += runLength;
		}
		break;
	default:
		error("Unsupported picture viewtype");
	}
}

void GfxView::unpackCel(int16 loopNo, int16 celNo, SciSpan<byte> &outPtr) {
	const CelInfo *celInfo = getCelInfo(loopNo, celNo);

	if (celInfo->offsetEGA) {
		// decompression for EGA views
		unpackCelData(*_resource, outPtr, 0, celInfo->offsetEGA, 0, _resMan->getViewType(), celInfo->width, false);
	} else {
		// We fill the buffer with transparent pixels, so that we can later skip
		//  over pixels to automatically have them transparent
		// Also some RLE compressed cels are possibly ending with the last
		// non-transparent pixel (is this even possible with the current code?)
		byte clearColor = _loop[loopNo].cel[celNo].clearKey;

		// Since Mac OS required palette index 0 to be white and 0xff to be black, the
		// Mac SCI devs decided that rather than change scripts and various pieces of
		// code, that they would just put a little snippet of code to swap these colors
		// in various places around the SCI codebase. We figured that it would be less
		// hacky to swap pixels instead and run the Mac games with a PC palette.
		bool isMacSci11View = g_sci->getPlatform() == Common::kPlatformMacintosh && getSciVersion() == SCI_VERSION_1_1;
		if (isMacSci11View) {
			// clearColor is based on PC palette, but the literal data is not.
			// We flip clearColor here to make it match the literal data. All
			// these pixels will be flipped back again below.
			if (clearColor == 0)
				clearColor = 0xff;
			else if (clearColor == 0xff)
				clearColor = 0;
		}

		unpackCelData(*_resource, outPtr, clearColor, celInfo->offsetRLE, celInfo->offsetLiteral, _resMan->getViewType(), celInfo->width, isMacSci11View);

		// Swap 0 and 0xff pixels for Mac SCI1.1+ games (see above)
		if (isMacSci11View) {
			for (uint32 i = 0; i < outPtr.size(); i++) {
				if (outPtr[i] == 0)
					outPtr[i] = 0xff;
				else if (outPtr[i] == 0xff)
					outPtr[i] = 0;
			}
		}
	}
}

const SciSpan<const byte> &GfxView::getBitmap(int16 loopNo, int16 celNo) {
	loopNo = CLIP<int16>(loopNo, 0, _loop.size() - 1);
	celNo = CLIP<int16>(celNo, 0, _loop[loopNo].cel.size() - 1);

	CelInfo &cel = _loop[loopNo].cel[celNo];

	if (cel.rawBitmap)
		return *cel.rawBitmap;

	const uint16 width = cel.width;
	const uint16 height = cel.height;
	const uint pixelCount = width * height;
	const Common::String sourceName = Common::String::format("%s loop %d cel %d", _resource->name().c_str(), loopNo, celNo);

	SciSpan<byte> outBitmap = cel.rawBitmap->allocate(pixelCount, sourceName);

	// unpack the actual cel bitmap data
	unpackCel(loopNo, celNo, outBitmap);

	if (_resMan->getViewType() == kViewEga)
		unditherBitmap(outBitmap, width, height, _loop[loopNo].cel[celNo].clearKey);

	// mirroring the cel if needed
	if (_loop[loopNo].mirrorFlag) {
		byte *pBitmap = outBitmap.getUnsafeDataAt(0, width * height);
		for (int i = 0; i < height; i++, pBitmap += width)
			for (int j = 0; j < width / 2; j++)
				SWAP(pBitmap[j], pBitmap[width - j - 1]);
	}

	return *cel.rawBitmap;
}

/**
 * Called after unpacking an EGA cel, this will try to undither (parts) of the
 * cel if the dithering in here matches dithering used by the current picture.
 */
void GfxView::unditherBitmap(SciSpan<byte> &bitmapPtr, int16 width, int16 height, byte clearKey) {
	int16 *ditheredPicColors = _screen->unditherGetDitheredBgColors();

	// It makes no sense to go further, if there isn't any dithered color data
	// available for the current picture
	if (!ditheredPicColors)
		return;

	// We need at least a 4x2 bitmap for this algorithm to work
	if (width < 4 || height < 2)
		return;

	// If EGA mapping is used for this view, dont do undithering as well
	if (_EGAmapping)
		return;

	// Walk through the bitmap and remember all combinations of colors
	int16 ditheredBitmapColors[DITHERED_BG_COLORS_SIZE];

	memset(&ditheredBitmapColors, 0, sizeof(ditheredBitmapColors));

	// Count all seemingly dithered pixel-combinations as soon as at least 4
	// pixels are adjacent and check pixels in the following line as well to
	// be the reverse pixel combination
	int16 checkHeight = height - 1;
	byte *curPtr = bitmapPtr.getUnsafeDataAt(0, checkHeight * width);
	const byte *nextPtr = bitmapPtr.getUnsafeDataAt(width, checkHeight * width);
	for (int16 y = 0; y < checkHeight; y++) {
		byte color1 = curPtr[0];
		byte color2 = (curPtr[1] << 4) | curPtr[2];
		byte nextColor1 = nextPtr[0] << 4;
		byte nextColor2 = (nextPtr[2] << 4) | nextPtr[1];
		curPtr += 3;
		nextPtr += 3;
		for (int16 x = 3; x < width; x++) {
			color1 = (color1 << 4) | (color2 >> 4);
			color2 = (color2 << 4) | *curPtr++;
			nextColor1 = (nextColor1 >> 4) | (nextColor2 << 4);
			nextColor2 = (nextColor2 >> 4) | *nextPtr++ << 4;
			if ((color1 == color2) && (color1 == nextColor1) && (color1 == nextColor2))
				ditheredBitmapColors[color1]++;
		}
	}

	// Now compare both dither color tables to find out matching dithered color
	// combinations
	bool unditherTable[DITHERED_BG_COLORS_SIZE];
	byte unditherCount = 0;
	memset(&unditherTable, false, sizeof(unditherTable));
	for (byte color = 0; color < 255; color++) {
		if ((ditheredBitmapColors[color] > 5) && (ditheredPicColors[color] > 200)) {
			// match found, check if colorKey is contained -> if so, we ignore
			// of course
			byte color1 = color & 0x0F;
			byte color2 = color >> 4;
			if ((color1 != clearKey) && (color2 != clearKey) && (color1 != color2)) {
				// so set this and the reversed color-combination for undithering
				unditherTable[color] = true;
				unditherTable[(color1 << 4) | color2] = true;
				unditherCount++;
			}
		}
	}

	// Nothing found to undither -> exit straight away
	if (!unditherCount)
		return;

	// We now need to replace color-combinations
	curPtr = bitmapPtr.getUnsafeDataAt(0, height * width);
	for (int16 y = 0; y < height; y++) {
		byte color = curPtr[0];
		for (int16 x = 1; x < width; x++) {
			color = (color << 4) | curPtr[1];
			if (unditherTable[color]) {
				// Some color with black? Turn colors around, otherwise it won't
				// be the right color at all.
				byte unditheredColor = color;
				if ((color & 0xF0) == 0)
					unditheredColor = (color << 4) | (color >> 4);
				curPtr[0] = unditheredColor;
				curPtr[1] = unditheredColor;
			}
			curPtr++;
		}
		curPtr++;
	}
}

byte GfxView::getMappedColor(byte color, uint16 scaleSignal, const Palette *palette, int x2, int y2) {
	byte outputColor = palette->mapping[color];
	// SCI16 remapping (QFG4 demo)
	if (g_sci->_gfxRemap16 && g_sci->_gfxRemap16->isRemapped(outputColor))
		outputColor = g_sci->_gfxRemap16->remapColor(outputColor, _screen->getVisual(x2, y2));
	// SCI11+ remapping (Catdate)
	if ((scaleSignal & 0xFF00) && g_sci->_gfxRemap16 && _resMan->testResource(ResourceId(kResourceTypeVocab, 184))) {
		if ((scaleSignal >> 8) == 1) // all black
			outputColor = 0;
		else if ((scaleSignal >> 8) == 2) // darken
			outputColor = g_sci->_gfxRemap16->remapColor(253, outputColor);
		else if ((scaleSignal >> 8) == 3) // shadow
			outputColor = g_sci->_gfxRemap16->remapColor(253, _screen->getVisual(x2, y2));
	}
	return outputColor;
}

void GfxView::draw(const Common::Rect &rect, const Common::Rect &clipRect, const Common::Rect &clipRectTranslated,
			int16 loopNo, int16 celNo, byte priority, uint16 EGAmappingNr, bool upscaledHires, uint16 scaleSignal) {
	// AGI demake: PQ2 police computer (room 8), view 9 is the terminal's cursor block (loop 0) and
	// selection frame (loop 1), both placed by the program for Sierra's narrow font
	if (_screen->agiDemake() && g_sci->getGameId() == GID_PQ2 && _resourceId == 9 && g_sci->getEngineState() &&
			g_sci->getEngineState()->currentRoomNumber() == 8) {
		const Palette *pal = _embeddedPal ? &_viewPalette : &_palette->_sysPalette;
		if (loopNo == 0 && rect.top < 24 && rect.left > 73 && _screen->_agiTermPromptGeneration != _screen->_agiPicGeneration)
			return;	// no prompt on this screen yet: nowhere sensible to put the cursor
		if (loopNo == 0 && rect.top < 24 && rect.left > 73) {
			// cursor: drawn where the next typed character will actually go
			const int16 delta = _screen->agiTermScreenX(rect.left) - rect.left;
			if (delta != 0) {
				Common::Rect r = rect, c = clipRect, ct = clipRectTranslated;
				r.translate(delta, 0);
				c.translate(delta, 0);
				ct.translate(delta, 0);
				drawAgiDemake(r, c, ct, loopNo, celNo, priority, scaleSignal, pal);
				// the game only refreshes the sprite's own (unmoved) area, so show the moved one too
				_screen->copyRectToScreen(ct);
				return;
			}
		}
		if (loopNo == 1) {
			// selection frame: drawn around the name as it actually appears. Left column: from the
			// game's position to just past the widest left-hand name. Right column: from just before
			// the "|" to just past the widest right-hand name. The previous frame is wiped first, since
			// the game only clears the frame's original area.
			const int portX = clipRectTranslated.left - clipRect.left, portY = clipRectTranslated.top - clipRect.top;
			const CelInfo *ci = getCelInfo(loopNo, celNo);
			int16 left, right;
			if (rect.left < 150) {
				left = rect.left;
				right = MAX<int16>(left + 20, _screen->_agiTermCol1Right + 3);
				if (_screen->_agiTermCol2X > 0)
					right = MIN<int16>(right, _screen->_agiTermCol2X - 2);
			} else {
				left = (_screen->_agiTermCol2X > 0) ? _screen->_agiTermCol2X - 2 : rect.left;
				right = MIN<int16>(252, MAX<int16>(left + 20, _screen->_agiTermCol2Right + 3));
			}
			const Common::Rect frame(left + portX, rect.top + portY, right + portX, rect.top + portY + ci->height);
			const byte oldMapValue = _screen->getCurPaletteMapValue();
			static uint32 lastFrameGeneration = 0xFFFFFFFF;
			if (lastFrameGeneration != _screen->_agiPicGeneration)
				_screen->_agiTermLastFrame = Common::Rect();
			lastFrameGeneration = _screen->_agiPicGeneration;
			const Common::Rect &last = _screen->_agiTermLastFrame;
			if (!last.isEmpty() && last != frame) {
				_screen->setCurPaletteMapValue(0);
				for (int x = last.left; x < last.right; ++x) {
					_screen->putPixel(x, last.top, GFX_SCREEN_MASK_VISUAL, 0, 0, 0);
					_screen->putPixel(x, last.bottom - 1, GFX_SCREEN_MASK_VISUAL, 0, 0, 0);
				}
				for (int y = last.top; y < last.bottom; ++y) {
					_screen->putPixel(last.left, y, GFX_SCREEN_MASK_VISUAL, 0, 0, 0);
					_screen->putPixel(last.right - 1, y, GFX_SCREEN_MASK_VISUAL, 0, 0, 0);
				}
				_screen->copyRectToScreen(last);
			}
			_screen->setCurPaletteMapValue(2);
			const byte white = _screen->getColorWhite();
			for (int x = frame.left; x < frame.right; ++x) {
				_screen->putPixel(x, frame.top, GFX_SCREEN_MASK_VISUAL, white, 0, 0);
				_screen->putPixel(x, frame.bottom - 1, GFX_SCREEN_MASK_VISUAL, white, 0, 0);
			}
			for (int y = frame.top; y < frame.bottom; ++y) {
				_screen->putPixel(frame.left, y, GFX_SCREEN_MASK_VISUAL, white, 0, 0);
				_screen->putPixel(frame.right - 1, y, GFX_SCREEN_MASK_VISUAL, white, 0, 0);
			}
			_screen->setCurPaletteMapValue(oldMapValue);
			_screen->copyRectToScreen(frame);
			_screen->_agiTermLastFrame = frame;
			return;
		}
	}
	const Palette *palette = _embeddedPal ? &_viewPalette : &_palette->_sysPalette;
	const CelInfo *celInfo = getCelInfo(loopNo, celNo);
	const SciSpan<const byte> &bitmap = getBitmap(loopNo, celNo);
	const int16 celHeight = celInfo->height;
	const int16 celWidth = celInfo->width;
	const byte clearKey = celInfo->clearKey;
	const byte drawMask = priority > 15 ? GFX_SCREEN_MASK_VISUAL : GFX_SCREEN_MASK_VISUAL|GFX_SCREEN_MASK_PRIORITY;

	if (_embeddedPal)
		// Merge view palette in...
		_palette->set(&_viewPalette, false);

	const int16 width = MIN(clipRect.width(), celWidth);
	const int16 height = MIN(clipRect.height(), celHeight);

	if (!width || !height) {
		return;
	}

	const byte *bitmapData = bitmap.getUnsafeDataAt((clipRect.top - rect.top) * celWidth + (clipRect.left - rect.left), celWidth * (height - 1) + width);

	// Set up custom per-view palette mod
	byte oldpalvalue = _screen->getCurPaletteMapValue();
	doCustomViewPalette(_screen, _resourceId, loopNo, celNo);

	if (_EGAmapping) {
		const SciSpan<const byte> EGAmapping = _EGAmapping.subspan(EGAmappingNr * SCI_VIEW_EGAMAPPING_SIZE, SCI_VIEW_EGAMAPPING_SIZE);
		for (int y = 0; y < height; y++, bitmapData += celWidth) {
			for (int x = 0; x < width; x++) {
				const byte color = EGAmapping[bitmapData[x]];
				const int x2 = clipRectTranslated.left + x;
				const int y2 = clipRectTranslated.top + y;
				if (color != clearKey && priority >= _screen->getPriority(x2, y2))
					_screen->putPixel(x2, y2, drawMask, color, priority, 0);
			}
		}
	} else if (upscaledHires) {
		// upscaledHires means view is hires and needs no scaling
		_screen->copyHiResRectToScreen(bitmapData, celWidth, clipRect.left, clipRect.top, width, height, palette->mapping);
	} else if (_screen->agiDemake() && g_sci->getGameId() == GID_PQ2 && _resourceId == 60 && loopNo == 2 && (celNo == 0 || celNo == 1)) {	// HOMICIDE, NARCOTICS
		drawAgiDemakePlate(rect, clipRect, clipRectTranslated, loopNo, celNo, priority, scaleSignal, palette);
	} else if (_screen->agiDemake() && agiDemakeLabelWord(loopNo)) {
		drawAgiDemakeLabel(rect, clipRect, clipRectTranslated, loopNo, celNo, priority, scaleSignal, palette);
	} else if (_screen->agiDemake()) {
		drawAgiDemake(rect, clipRect, clipRectTranslated, loopNo, celNo, priority, scaleSignal, palette);
	} else {
		for (int y = 0; y < height; y++, bitmapData += celWidth) {
			for (int x = 0; x < width; x++) {
				const byte color = bitmapData[x];
				if (color != clearKey) {
					const int x2 = clipRectTranslated.left + x;
					const int y2 = clipRectTranslated.top + y;
					if (priority >= _screen->getPriority(x2, y2)) {
						_screen->putPixel(x2, y2, drawMask, getMappedColor(color, scaleSignal, palette, x2, y2), priority, 0);
					}
				}
			}
		}
	}

	// Reset custom per-view palette mod
	_screen->setCurPaletteMapValue(oldpalvalue);
}

// AGI demake: lettering painted into a sprite, replaced by AGI font text. The painted letters
// (one colour, inside the given areas) are turned into the background colour before the fat
// pixel collapse, then each word is drawn at full resolution, flagged as text, on a backing box.
struct AgiDemakeTextArea {
	int16 top, bottom, left, right;	// inclusive, cel coordinates
};
struct AgiDemakeTextItem {
	const char *word;
	int16 x, y;			// cel coordinates of the word's top left, or top right when alignRight
	bool alignRight;
};
struct AgiDemakePaint {
	int16 y, x0, x1;		// horizontal run of pixels to add, inclusive, cel coordinates (x0 > x1 ends the list)
	byte color;
};
struct AgiDemakeTextOverride {
	SciGameId game;
	int16 view, loop, cel;
	byte letterColor, background, textColor, backing;	// backing 0xFF: no backing box
	AgiDemakeTextArea areas[4];
	AgiDemakeTextItem items[6];
	AgiDemakePaint paint[8];	// extra pixels painted in before the fat pixel collapse
};
static const AgiDemakeTextOverride kAgiDemakeTextOverrides[] = {
	// PQ2 scuba tank 1 (dive shop tank selection): the 1 painted on the tank loses the right half
	// of its base in the collapse. One extra base pixel brings it back, like tanks 2 3 and 4
	{ GID_PQ2, 96, 2, 1, 0, 0, 0, 0xFF,
		{ { 0, -1, 0, -1 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 } },
		{ { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false } },
		{ { 37, 10, 10, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 } } },
	// PQ2 gun sight adjustment close-up: ELEVATION SCREW (top left) and WINDAGE SCREW (bottom right)
	{ GID_PQ2, 70, 4, 0, 14, 0, 14, 0,
		{ { 18, 29, 5, 44 }, { 57, 68, 76, 113 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 } },
		{ { "ELEVATION", 6, 6, false }, { "SCREW", 6, 15, false }, { "WINDAGE", 113, 56, true }, { "SCREW", 113, 65, true },
		  { nullptr, 0, 0, false }, { nullptr, 0, 0, false } },
		// ELEVATION pointer line (2 pixels wide, 2 across per row) extended up to 2 pixels under SCREW
		{ { 24, 19, 20, 4 }, { 25, 21, 22, 4 }, { 26, 23, 24, 4 }, { 27, 25, 26, 4 }, { 28, 27, 28, 4 },
		  { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 } } },
	// PQ2 filing cabinet folders: surname on each tab (full names do not fit in the 8x8 font)
	{ GID_PQ2, 60, 0, 0, 0, 10, 0, 0xFF,
		{ { 2, 5, 13, 72 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 } },
		{ { "Bains", 26, 1, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false } },
		{ { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 } } },
	{ GID_PQ2, 60, 0, 1, 0, 10, 0, 0xFF,
		{ { 2, 5, 51, 110 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 } },
		{ { "Botts", 64, 1, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false } },
		{ { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 } } },
	{ GID_PQ2, 60, 0, 2, 0, 10, 0, 0xFF,
		{ { 2, 5, 10, 69 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 } },
		{ { "Loofin", 20, 1, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false } },
		{ { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 } } },
	{ GID_PQ2, 60, 0, 3, 0, 10, 0, 0xFF,
		{ { 2, 5, 35, 94 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 } },
		{ { "Martinez", 37, 1, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false } },
		{ { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 } } },
	{ GID_PQ2, 60, 0, 4, 0, 10, 0, 0xFF,
		{ { 2, 5, 11, 70 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 } },
		{ { "South", 23, 1, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false } },
		{ { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 } } },
	{ GID_PQ2, 60, 0, 5, 0, 10, 0, 0xFF,
		{ { 2, 5, 32, 91 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 } },
		{ { "Taselli", 40, 1, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false } },
		{ { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 } } },
	{ GID_PQ2, 60, 0, 6, 0, 10, 0, 0xFF,
		{ { 2, 5, 10, 69 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 } },
		{ { "West", 26, 1, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false } },
		{ { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 } } },
	// PQ2 Narcotics filing cabinet folders (loop 1): surname on each tab, as in Homicide. The tabs read
	// DONALD COLBY, MOFFET DICKEY, ROBIN JONES, VICTOR SIMMS, WILMA SNIDER, GEORGE SNOW, JOSE VALENCIA
	{ GID_PQ2, 60, 1, 0, 0, 10, 0, 0xFF,
		{ { 2, 5, 13, 71 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 } },
		{ { "Colby", 26, 1, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false } },
		{ { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 } } },
	{ GID_PQ2, 60, 1, 1, 0, 10, 0, 0xFF,
		{ { 2, 5, 51, 109 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 } },
		{ { "Dickey", 60, 1, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false } },
		{ { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 } } },
	{ GID_PQ2, 60, 1, 2, 0, 10, 0, 0xFF,
		{ { 2, 5, 10, 68 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 } },
		{ { "Jones", 22, 1, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false } },
		{ { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 } } },
	{ GID_PQ2, 60, 1, 3, 0, 10, 0, 0xFF,
		{ { 2, 5, 11, 69 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 } },
		{ { "Simms", 24, 1, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false } },
		{ { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 } } },
	{ GID_PQ2, 60, 1, 4, 0, 10, 0, 0xFF,
		{ { 2, 5, 35, 91 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 } },
		{ { "Snider", 43, 1, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false } },
		{ { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 } } },
	{ GID_PQ2, 60, 1, 5, 0, 10, 0, 0xFF,
		{ { 2, 5, 32, 90 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 } },
		{ { "Snow", 48, 1, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false } },
		{ { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 } } },
	{ GID_PQ2, 60, 1, 6, 0, 10, 0, 0xFF,
		{ { 2, 5, 10, 68 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 }, { 0, -1, 0, -1 } },
		{ { "Valencia", 13, 1, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false }, { nullptr, 0, 0, false } },
		{ { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 }, { 0, 1, 0, 0 } } },
};

static int agiDemakeWordWidth(const char *word);

// PQ2 file mugshots (view 204 Homicide, view 205 Narcotics, 73x52): the booking number is painted on
// two small placards in a 3x5 font. It is read off the left placard, and both placards become one
// black bar under the two photos with the number once, in the AGI font. All digits were taken from
// the game's own placards (9 comes with an open and a closed foot). Anything that does not match
// exactly leaves the placards as they are.
// Personnel file photos (view 204, 39x52, e.g. Pate's file) have one placard with a name such as
// "PATE, L" in the same 3x5 font. The surname (letters before the comma) is read off it and drawn
// on a black bar the same way. Letters P A T E L were taken from Pate's placard. Anything else
// leaves the placard as it is.
// The two mugshot inventory items (views 112 and 123, same 73x52 layout) carry the same placards
// with black digits on grey. They are read the same way and get the same black bar.
static const AgiDemakeTextOverride *agiDemakeMugshotPlacard(const SciSpan<const byte> &bitmap, int w, int h, byte digitColor = 15) {
	static const struct { char ch; const char *rows; } kDigits[] = {
		{ '1', ".#./##./.#./.#./###" }, { '2', "###/..#/###/#../###" }, { '4', "#.#/#.#/#.#/###/..#" },
		{ '5', "###/#../###/..#/###" }, { '6', "###/#../###/#.#/###" }, { '7', "###/..#/..#/..#/..#" },
		{ '9', "###/#.#/###/..#/..#" }, { '0', "###/#.#/#.#/#.#/###" }, { '3', "###/..#/.##/..#/###" },
		{ '8', "###/#.#/###/#.#/###" }, { '9', "###/#.#/###/..#/###" }, { '.', "./././#/#" }
	};
	static const struct { char ch; const char *rows; } kLetters[] = {
		{ 'P', "###/#.#/###/#../#.." }, { 'A', ".##./#..#/#..#/####/#..#" }, { 'T', "###/.#./.#./.#./.#." },
		{ 'E', "###/#../##./#../###" }, { 'L', "#../#../#../#../###" }, { '.', "./././#/#" }
	};
	static AgiDemakeTextOverride placard;
	static char number[16];
	const bool single = (w == 39 && h == 52);
	if (!single && (w != 73 || h != 52))
		return nullptr;
	const byte white = digitColor;	// the digits' colour: white on the station files, black on the inventory copies
	// two-photo file: left placard, digits in rows 43-47, columns 8-29
	// personnel file: one placard, letters in rows 44-48, columns 6-33
	const int top = single ? 44 : 43;
	const int xStart = single ? 6 : 8, xEnd = single ? 34 : 30;
	Common::String found;
	for (int x = xStart; x < xEnd; ) {
		bool ink = false;
		for (int y = top; y <= top + 4; ++y)
			if (bitmap[y * w + x] == white)
				ink = true;
		if (!ink) {
			++x;
			continue;
		}
		int e = x;
		for (;;) {
			bool next = false;
			if (e + 1 < xEnd)
				for (int y = top; y <= top + 4; ++y)
					if (bitmap[y * w + e + 1] == white)
						next = true;
			if (!next)
				break;
			++e;
		}
		Common::String pattern;
		for (int y = top; y <= top + 4; ++y) {
			if (y > top)
				pattern += '/';
			for (int i = x; i <= e; ++i)
				pattern += (bitmap[y * w + i] == white) ? '#' : '.';
		}
		char ch = 0;
		if (single) {
			for (uint d = 0; d < ARRAYSIZE(kLetters); ++d)
				if (pattern == kLetters[d].rows)
					ch = kLetters[d].ch;
		} else {
			for (uint d = 0; d < ARRAYSIZE(kDigits); ++d)
				if (pattern == kDigits[d].rows)
					ch = kDigits[d].ch;
		}
		if (!ch || found.size() >= sizeof(number) - 1)
			return nullptr;
		if (single && ch == '.')
			break;	// the comma after the surname
		found += ch;
		x = e + 1;
	}
	if (found.empty())
		return nullptr;
	Common::strlcpy(number, found.c_str(), sizeof(number));

	placard.game = GID_PQ2;
	placard.view = 204;
	placard.loop = placard.cel = -1;
	placard.letterColor = white;
	placard.background = 0;
	placard.textColor = 15;	// the AGI number is always white on the black bar
	placard.backing = 0xFF;
	for (int i = 0; i < 4; ++i)
		placard.areas[i] = AgiDemakeTextArea{ 0, -1, 0, -1 };
	for (int i = 0; i < 6; ++i)
		placard.items[i] = AgiDemakeTextItem{ nullptr, 0, 0, false };
	// one black bar across both placards, rows 42-48, columns 7-64, or across the single
	// placard, rows 43-49, columns 3-34 (the photo's width inside its frame, whole fat pixels)
	const int barTop = single ? 43 : 42, barL = single ? 3 : 7, barR = single ? 34 : 64;
	for (int i = 0; i < 7; ++i)
		placard.paint[i] = AgiDemakePaint{ (int16)(barTop + i), (int16)barL, (int16)barR, 0 };
	placard.paint[7] = AgiDemakePaint{ 0, 1, 0, 0 };
	const int textW = agiDemakeWordWidth(number);
	if (textW > barR - barL + 1)
		return nullptr;
	placard.items[0] = AgiDemakeTextItem{ number, (int16)(barL + (barR - barL + 1 - textW) / 2), (int16)barTop, false };
	return &placard;
}

static const AgiDemakeTextOverride *agiDemakeFindTextOverride(GuiResourceId view, int16 loop, int16 cel) {
	for (uint i = 0; i < ARRAYSIZE(kAgiDemakeTextOverrides); ++i) {
		const AgiDemakeTextOverride &o = kAgiDemakeTextOverrides[i];
		if (o.game == g_sci->getGameId() && o.view == view && o.loop == loop && o.cel == cel)
			return &o;
	}
	return nullptr;
}

// Glyph columns actually used in the 8x8 PC BIOS font (letters are spaced by their own width)
static void agiDemakeGlyphCols(byte ch, int &a, int &b) {
	const uint8 *g = Graphics::DosFont::fontData_PCBIOS + ch * 8;
	a = 8;
	b = -1;
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
}

static int agiDemakeWordWidth(const char *word) {
	int w = 0;
	for (const char *ch = word; *ch; ++ch) {
		int a, b;
		agiDemakeGlyphCols((byte)*ch, a, b);
		w += b - a + 2;
	}
	return w - 1;
}

// AGI demake: draws a rectangular frame cel at a narrower width (its last column becomes the
// right edge), flagged as frame pixels so its thin lines stay whole fat pixels
void GfxView::drawAgiDemakeFrame(const Common::Rect &rect, const Common::Rect &clipRect, const Common::Rect &clipRectTranslated,
			int16 loopNo, int16 celNo, byte priority, uint16 scaleSignal, const Palette *palette, int newWidth) {
	const CelInfo *celInfo = getCelInfo(loopNo, celNo);
	const SciSpan<const byte> &bitmap = getBitmap(loopNo, celNo);
	const int w = celInfo->width, h = celInfo->height;
	const byte clearKey = celInfo->clearKey;
	const byte drawMask = priority > 15 ? GFX_SCREEN_MASK_VISUAL : GFX_SCREEN_MASK_VISUAL|GFX_SCREEN_MASK_PRIORITY;
	newWidth = MIN(newWidth, w);
	// port to screen offset
	const int portX = clipRectTranslated.left - clipRect.left, portY = clipRectTranslated.top - clipRect.top;
	const byte oldMapValue = _screen->getCurPaletteMapValue();
	_screen->setCurPaletteMapValue(2);
	for (int y = 0; y < h; ++y) {
		for (int j = 0; j < newWidth; ++j) {
			const int srcX = (j < newWidth - 1) ? j : w - 1;
			const byte c = bitmap[y * w + srcX];
			if (c == clearKey)
				continue;
			const int xs = rect.left + portX + j, ys = rect.top + portY + y;
			if (xs < clipRectTranslated.left || xs >= clipRectTranslated.right || ys < clipRectTranslated.top || ys >= clipRectTranslated.bottom)
				continue;
			if (priority >= _screen->getPriority(xs, ys))
				_screen->putPixel(xs, ys, drawMask, getMappedColor(c, scaleSignal, palette, xs, ys), priority, 0);
		}
	}
	_screen->setCurPaletteMapValue(oldMapValue);
}

// AGI demake: PQ2 filing cabinet drawer plate (view 60 loop 2), "HOMICIDE" in a tiny font.
// The plate is made wider so the word fits on one line in the AGI font (tight spacing): its plain
// middle, border and all, is repeated using its clean first middle column, and the wider plate is
// centred on the original. The plate is static scenery, so drawing past its own cel is safe.
void GfxView::drawAgiDemakePlate(const Common::Rect &rect, const Common::Rect &clipRect, const Common::Rect &clipRectTranslated,
			int16 loopNo, int16 celNo, byte priority, uint16 scaleSignal, const Palette *palette) {
	const CelInfo *celInfo = getCelInfo(loopNo, celNo);
	const SciSpan<const byte> &bitmap = getBitmap(loopNo, celNo);
	const int w = celInfo->width, h = celInfo->height;
	const byte clearKey = celInfo->clearKey;
	const byte drawMask = priority > 15 ? GFX_SCREEN_MASK_VISUAL : GFX_SCREEN_MASK_VISUAL|GFX_SCREEN_MASK_PRIORITY;
	// one entry per drawer: the word, the plate's lettered middle (stretched to fit the word), a
	// clean column copied across it, and the top row for the AGI lettering
	static const struct { int16 cel; const char *word; int16 midLeft, midRight, cleanCol, textY; } kPlates[] = {
		{ 0, "HOMICIDE", 7, 44, 7, 5 },
		{ 1, "NARCOTICS", 5, 48, 5, 8 },
	};
	uint plateNo = 0;
	while (plateNo + 1 < ARRAYSIZE(kPlates) && kPlates[plateNo].cel != celNo)
		++plateNo;
	const char *const word = kPlates[plateNo].word;
	const int midLeft = kPlates[plateNo].midLeft, midRight = kPlates[plateNo].midRight;
	const int cleanCol = kPlates[plateNo].cleanCol, textY = kPlates[plateNo].textY;
	const int textW = agiDemakeWordWidth(word);
	const int newMid = textW + 6;
	const int extra = MAX(0, newMid - (midRight - midLeft + 1));
	const int newW = w + extra;
	const int celScreenX = clipRectTranslated.left - (clipRect.left - rect.left) - extra / 2;
	const int celScreenY = clipRectTranslated.top - (clipRect.top - rect.top);
	const byte oldMapValue = _screen->getCurPaletteMapValue();

	// the holder drawn behind the plate in the picture: rows just above and below the plate are
	// stretched the same way (once per picture, so repeated redraws do not keep widening it)
	_screen->setCurPaletteMapValue(0);
	{
		static uint32 lastGeneration = 0xFFFFFFFF;
		static int lastX = -9999, lastY = -9999;
		const int origX = celScreenX + extra / 2;
		if (_screen->_agiPicGeneration != lastGeneration || origX != lastX || celScreenY != lastY) {
			lastGeneration = _screen->_agiPicGeneration;
			lastX = origX;
			lastY = celScreenY;
			const int margin = 6;
			const int x0 = origX - margin, x1 = origX + w + margin;
			Common::Array<byte> row(x1 - x0);
			for (int y = celScreenY - margin; y < celScreenY + h + margin; ++y) {
				if ((y >= celScreenY && y < celScreenY + h) || y < 0 || y >= _screen->getHeight())
					continue;
				for (int x = x0; x < x1; ++x)
					row[x - x0] = (x >= 0 && x < _screen->getWidth()) ? _screen->getVisual(x, y) : 0;
				for (int x = x0; x < x1; ++x) {
					int nx;
					if (x < origX + midLeft)
						nx = x - extra / 2;
					else if (x > origX + midRight)
						nx = x + extra - extra / 2;
					else
						continue;
					if (nx >= 0 && nx < _screen->getWidth())
						_screen->putPixel(nx, y, GFX_SCREEN_MASK_VISUAL, row[x - x0], 0, 0);
				}
				for (int j = 0; j < newMid; ++j) {
					const int nx = origX + midLeft - extra / 2 + j;
					if (nx >= 0 && nx < _screen->getWidth())
						_screen->putPixel(nx, y, GFX_SCREEN_MASK_VISUAL, row[origX + cleanCol - x0], 0, 0);
				}
			}
		}
	}

	// the widened plate (old lettering removed)
	for (int y = 0; y < h; ++y) {
		for (int j = 0; j < newW; ++j) {
			const int srcX = (j < midLeft) ? j : ((j < midLeft + newMid) ? cleanCol : j - extra);
			byte c = bitmap[y * w + srcX];
			if (c == clearKey)
				continue;
			const int x2 = celScreenX + j, y2 = celScreenY + y;
			if (x2 < 0 || x2 >= _screen->getWidth() || y2 < 0 || y2 >= _screen->getHeight())
				continue;
			if (priority >= _screen->getPriority(x2, y2))
				_screen->putPixel(x2, y2, drawMask, getMappedColor(c, scaleSignal, palette, x2, y2), priority, 0);
		}
	}

	// the word, centred in the widened white area, flagged as text so it stays at full resolution
	int tx = celScreenX + midLeft + (newMid - textW) / 2;
	_screen->setCurPaletteMapValue(1);
	for (const char *ch = word; *ch; ++ch) {
		const uint8 *g = Graphics::DosFont::fontData_PCBIOS + (byte)*ch * 8;
		int a, b;
		agiDemakeGlyphCols((byte)*ch, a, b);
		for (int gy = 0; gy < 7; ++gy)
			for (int gx = a; gx <= b; ++gx)
				if (g[gy] & (0x80 >> gx)) {
					const int x2 = tx + gx - a, y2 = celScreenY + textY + gy;
					if (x2 < 0 || x2 >= _screen->getWidth() || y2 < 0 || y2 >= _screen->getHeight())
						continue;
					if (priority >= _screen->getPriority(x2, y2))
						_screen->putPixel(x2, y2, drawMask, getMappedColor(0, scaleSignal, palette, x2, y2), priority, 0);
				}
		tx += b - a + 2;
	}
	_screen->setCurPaletteMapValue(oldMapValue);
}

// AGI demake: name labels drawn in a tiny font. PQ2 view 180 holds the radio speaker labels,
// loop 1 "KEITH:" and loop 0 "DISPATCH:". They are redrawn as a plain box with the name centred
// in the AGI 8x8 font (no colon, which does not fit DISPATCH).
const char *GfxView::agiDemakeLabelWord(int16 loopNo) const {
	if (g_sci->getGameId() != GID_PQ2 || _resourceId != 180)
		return nullptr;
	if (loopNo == 1)
		return "KEITH";
	if (loopNo == 0)
		return "DISPATCH";
	return nullptr;
}

// Plain label: the white line and the second decorative line colour (found just inside the
// bottom left) become the background colour (top left pixel), the drop shadow stays, the box is
// left to the driver's fat pixel collapse, and the word is drawn on top at full resolution
// flagged as text, centred in the background area. Letters are spaced by their own width.
void GfxView::drawAgiDemakeLabel(const Common::Rect &rect, const Common::Rect &clipRect, const Common::Rect &clipRectTranslated,
			int16 loopNo, int16 celNo, byte priority, uint16 scaleSignal, const Palette *palette) {
	const CelInfo *celInfo = getCelInfo(loopNo, celNo);
	const SciSpan<const byte> &bitmap = getBitmap(loopNo, celNo);
	const int w = celInfo->width, h = celInfo->height;
	const byte clearKey = celInfo->clearKey;
	const byte white = _screen->getColorWhite();
	const byte bg = bitmap[0];
	const byte deco = (h >= 2 && w >= 2) ? bitmap[(h - 2) * w + 1] : bg;
	const byte drawMask = priority > 15 ? GFX_SCREEN_MASK_VISUAL : GFX_SCREEN_MASK_VISUAL|GFX_SCREEN_MASK_PRIORITY;
	const char *word = agiDemakeLabelWord(loopNo);

	Common::Array<byte> pix(w * h);
	int top = h, bottom = -1, left = w, right = -1;
	for (int y = 0; y < h; ++y)
		for (int x = 0; x < w; ++x) {
			byte c = bitmap[y * w + x];
			if (c == white || c == deco)
				c = bg;
			pix[y * w + x] = c;
			if (c == bg) {
				top = MIN(top, y);
				bottom = MAX(bottom, y);
				left = MIN(left, x);
				right = MAX(right, x);
			}
		}

	// Word layout in the 8x8 PC BIOS font: each glyph trimmed to its used columns, 1 pixel gap
	int textW = 0;
	for (const char *ch = word; *ch; ++ch) {
		const uint8 *g = Graphics::DosFont::fontData_PCBIOS + (byte)*ch * 8;
		int a = 8, b = -1;
		for (int gx = 0; gx < 8; ++gx)
			for (int gy = 0; gy < 8; ++gy)
				if (g[gy] & (0x80 >> gx)) {
					a = MIN(a, gx);
					b = MAX(b, gx);
				}
		textW += b - a + 2;
	}
	textW -= 1;
	Common::Array<bool> text(w * h, false);
	int tx = left + (right - left + 1 - textW) / 2;
	const int ty = top + (bottom - top + 1 - 7) / 2;
	for (const char *ch = word; *ch; ++ch) {
		const uint8 *g = Graphics::DosFont::fontData_PCBIOS + (byte)*ch * 8;
		int a = 8, b = -1;
		for (int gx = 0; gx < 8; ++gx)
			for (int gy = 0; gy < 8; ++gy)
				if (g[gy] & (0x80 >> gx)) {
					a = MIN(a, gx);
					b = MAX(b, gx);
				}
		for (int gy = 0; gy < 7; ++gy)
			for (int gx = a; gx <= b; ++gx)
				if (g[gy] & (0x80 >> gx)) {
					const int x = tx + gx - a, y = ty + gy;
					if (x >= 0 && x < w && y >= 0 && y < h)
						text[y * w + x] = true;
				}
		tx += b - a + 2;
	}

	const int offX = clipRect.left - rect.left, offY = clipRect.top - rect.top;
	const int16 width = MIN<int>(clipRect.width(), w), height = MIN<int>(clipRect.height(), h);
	const byte oldMapValue = _screen->getCurPaletteMapValue();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			const int cx = offX + x, cy = offY + y;
			if (cx >= w || cy >= h)
				continue;
			const bool isText = text[cy * w + cx];
			const byte c = isText ? white : pix[cy * w + cx];
			if (c == clearKey)
				continue;
			const int x2 = clipRectTranslated.left + x;
			const int y2 = clipRectTranslated.top + y;
			if (priority >= _screen->getPriority(x2, y2)) {
				_screen->setCurPaletteMapValue(isText ? 1 : 0);
				_screen->putPixel(x2, y2, drawMask, getMappedColor(c, scaleSignal, palette, x2, y2), priority, 0);
			}
		}
	}
	_screen->setCurPaletteMapValue(oldMapValue);
}

// AGI demake helpers
static const byte kAgiDemakeLuma[16] = { 0, 19, 100, 119, 51, 70, 101, 170, 85, 104, 185, 204, 136, 155, 236, 255 };

static byte agiDemakePickLighter(byte a, byte b, byte clearKey) {
	if (a == clearKey)
		return b;
	if (b == clearKey || a == b || a > 15 || b > 15)
		return a;
	return (kAgiDemakeLuma[b] > kAgiDemakeLuma[a]) ? b : a;
}

static byte agiDemakePick(byte a, byte b, byte clearKey) {
	if (a == clearKey)
		return b;
	if (b == clearKey || a == b || a > 15 || b > 15)
		return a;
	return (kAgiDemakeLuma[b] < kAgiDemakeLuma[a]) ? b : a;
}

// QfG1 hero (view 0 walking, view 4 standing, every loop). Sierra drew him with a 1 pixel dark
// grey outline, which as fat pixels becomes a thick grey border. Dark grey pixels touching
// transparency (or the cel's edge)
// are dropped before the collapse.
static bool agiDemakeIsQfgHero(int view, int loop = 0) {
	// view 300 loop 8: his head turned left or right, drawn over the standing-away frame when he
	// looks around at the start of the game (room 300). View 301 loop 6: sitting down at the inn.
	// View 83 loop 8: sitting on a chair facing right (room 83). View 508: seen from behind in the
	// stable. View 140: raking in the stable (his rake keeps its grey). Views 503, 519 and 550: at the
	// brigand fortress and other scenes, facing you. View 97 loops 6 and 7: Yorick walking in and
	// standing in Elsa's room. View 122 loop 0: raising the magic mirror in Baba Yaga's hut (loop 1 is
	// the reflection). View 139 loop 4: facing you at the award ceremony at the end
	return g_sci->getGameId() == GID_QFG1 && (view == 0 || view == 4 || (view == 300 && loop == 8) ||
		(view == 301 && loop == 6) || (view == 83 && loop == 8) || view == 508 || view == 140 ||
		view == 503 || view == 519 || view == 550 || (view == 97 && (loop == 6 || loop == 7)) ||
		(view == 122 && loop == 0) || (view == 139 && loop == 4));
}

static void agiDemakeHeroSource(int view, int loop, const byte *src, int w, int h, byte clearKey, byte *out) {
	const byte darkGrey = 8;
	for (int i = 0; i < w * h; ++i)
		out[i] = src[i];
	if (!agiDemakeIsQfgHero(view, loop))
		return;
	// sitting down (view 301): only his head is outlined, and the stool beside him is dark grey too,
	// so only the top third of the cel is touched
	// sitting facing right (view 83): his head only (top 11 rows), and the chair back on his left (first 3 columns) is
	// left alone too
	const int rows = (view == 301) ? h / 3 : (view == 83) ? 11 : (view == 97) ? 9 : h;	// view 97: Yorick's head only, his cloak keeps its grey
	const int firstX = (view == 83) ? 3 : 0;
	// raking (view 140): the rake is grey too, so it is left alone. Its head is found by the light grey
	// at its tip in the outermost 3 columns (bottom half): those rows, one either side, within 10
	// pixels of that side. Its handle's dark edge is the grey beside brown running on into brown or red
	int headSide = 0, headTop = -1, headBottom = -1;
	if (view == 140) {
		for (int y = h / 2; y < h; ++y)
			for (int x = 0; x < w; ++x)
				if (src[y * w + x] == 7 && (x < 3 || x >= w - 3)) {
					headSide = (x < 3) ? -1 : 1;
					if (headTop < 0)
						headTop = y;
					headBottom = y;
				}
	}
	auto isRake = [&](int x, int y) {
		if (view != 140)
			return false;
		if (headSide && y >= headTop - 1 && y <= headBottom + 1 && ((headSide > 0 && x >= w - 10) || (headSide < 0 && x < 10)))
			return true;
		auto at = [&](int xx) { return (xx >= 0 && xx < w) ? src[y * w + xx] : clearKey; };
		const byte brown = 6, red = 4;
		return (at(x + 1) == brown && (at(x + 2) == brown || at(x + 2) == red)) ||
			(at(x - 1) == brown && (at(x - 2) == brown || at(x - 2) == red));
	};
	for (int y = 0; y < rows; ++y)
		for (int x = firstX; x < w; ++x) {
			if (src[y * w + x] != darkGrey || isRake(x, y))
				continue;
			const bool edge = x == 0 || y == 0 || x == w - 1 || y == h - 1 ||
				src[y * w + x - 1] == clearKey || src[y * w + x + 1] == clearKey ||
				src[(y - 1) * w + x] == clearKey || src[(y + 1) * w + x] == clearKey;
			if (edge)
				out[y * w + x] = clearKey;
		}
}

// QfG1 "Spielburg" sign outside the town gate (view 165): the letters are black strokes with a
// light red and red highlight beside them, which collapse to a mix of black and pink. Light red and
// red pixels touching black (left, right, above or below) are made black, so the letters come out
// solid black. The sign's frame down its left edge (first 3 columns) is left alone
static bool agiDemakeIsQfgSign(int view, int loop) {
	return g_sci->getGameId() == GID_QFG1 && view == 165 && loop == 0;
}

static void agiDemakeSignLetters(const byte *src, int w, int h, byte *out) {
	const byte black = 0, red = 4, lightRed = 12;
	for (int i = 0; i < w * h; ++i)
		out[i] = src[i];
	for (int y = 0; y < h; ++y)
		for (int x = 3; x < w; ++x) {
			if (src[y * w + x] != lightRed && src[y * w + x] != red)
				continue;
			if ((x > 0 && src[y * w + x - 1] == black) || (x + 1 < w && src[y * w + x + 1] == black) ||
					(y > 0 && src[(y - 1) * w + x] == black) || (y + 1 < h && src[(y + 1) * w + x] == black))
				out[y * w + x] = black;
		}
}

// QfG1 Katta behind the inn's bar (view 302 loops 2, 4 and 5) and the Katta waitress (views 303
// and 304, every loop): her eyes are light green and green
// pixels beside black, and the collapse turns them black. On the first head row with light green,
// her eyes go one fat pixel in from each side of her face (two eyes), or where it is (one eye,
// when she turns her head)
static void agiDemakeKattaEyes(int view, int loop, const SciSpan<const byte> &bitmap, int w, int h, byte clearKey, byte *fat, int fatW, int startX, int pad, int numPairs) {
	if (g_sci->getGameId() != GID_QFG1)
		return;
	if (!((view == 302 && (loop == 2 || loop == 4 || loop == 5)) || view == 303 || view == 304))
		return;
	const byte green = 2, lightGreen = 10;
	for (int y = 0; y < h && y < 12; ++y) {
		const byte *row = bitmap.getUnsafeDataAt(y * w, w);
		bool hasLight = false;
		for (int x = 0; x < w; ++x)
			if (row[x] == lightGreen)
				hasLight = true;
		if (!hasLight)
			continue;
		// her eyes on this row: light green, and the green beside it
		int first = -1, last = -1;
		for (int x = 0; x < w; ++x)
			if (row[x] == lightGreen || row[x] == green) {
				if (first < 0)
					first = x;
				last = x;
			}
		if (first < 0)
			continue;
		byte *f = fat + y * fatW + pad;
		if (last - first >= 3) {
			int left = 0, right = numPairs - 1;
			while (left < numPairs && f[left] == clearKey)
				++left;
			while (right >= 0 && f[right] == clearKey)
				--right;
			if (right - left >= 3) {
				f[left + 1] = lightGreen;
				f[right - 1] = lightGreen;
			}
		} else {
			const int p = (first - startX) >> 1;
			if (p >= 0 && p < numPairs && f[p] != clearKey)
				f[p] = lightGreen;
		}
		return;
	}
}

// QfG1 hero sitting down (view 301 loop 6): with the outline gone his head is a fat pixel too wide,
// so the rightmost column his head reaches (rows down to the last row with skin, in the top third)
// is cleared where it is his hair (yellow or brown) or grey
static void agiDemakeHeroSittingHead(int view, int loop, int cel, int h, byte clearKey, byte *fat, int fatW, int pad, int numPairs) {
	// cel 1 is left as it is (looks right already)
	if (g_sci->getGameId() != GID_QFG1 || view != 301 || loop != 6 || cel == 1)
		return;
	const byte skin = 12;
	int lastSkin = -1;
	for (int y = 0; y < h / 3; ++y)
		for (int p = 0; p < numPairs; ++p)
			if (fat[y * fatW + pad + p] == skin)
				lastSkin = y;
	int right = -1;
	for (int y = 0; y <= lastSkin; ++y)
		for (int p = numPairs - 1; p > right; --p)
			if (fat[y * fatW + pad + p] != clearKey) {
				right = p;
				break;
			}
	for (int y = 0; y <= lastSkin && right >= 0; ++y) {
		byte &v = fat[y * fatW + pad + right];
		if (v == 6 || v == 14 || v == 8)
			v = clearKey;
	}
	// the brown on the top row of his head moves one fat pixel left
	const byte brown = 6;
	for (int y = 0; y <= lastSkin; ++y) {
		byte *f = fat + y * fatW + pad;
		bool any = false;
		for (int p = 0; p < numPairs; ++p)
			if (f[p] != clearKey)
				any = true;
		if (!any)
			continue;
		Common::Array<bool> isBrown(numPairs, false);
		for (int p = 0; p < numPairs; ++p)
			if (f[p] == brown) {
				isBrown[p] = true;
				f[p] = clearKey;
			}
		for (int p = 1; p < numPairs; ++p)
			if (isBrown[p] && f[p - 1] == clearKey)
				f[p - 1] = brown;
		break;
	}
}

// QfG1 ice man (view 59, every loop): his two eyes are single light red pixels between black ones,
// and the collapse turns them black. On the first head row with light red, each eye gets its fat
// pixel back in light red, with at least one pixel between them
static void agiDemakeIceManEyes(int view, const SciSpan<const byte> &bitmap, int w, int h, byte clearKey, byte *fat, int fatW, int startX, int pad, int numPairs) {
	if (g_sci->getGameId() != GID_QFG1 || view != 59)
		return;
	const byte lightRed = 12;
	for (int y = 0; y < h && y < 20; ++y) {
		const byte *row = bitmap.getUnsafeDataAt(y * w, w);
		int eyes[2], n = 0;
		for (int x = 0; x < w && n < 2; ++x)
			if (row[x] == lightRed)
				eyes[n++] = x;
		if (!n)
			continue;
		byte *f = fat + y * fatW + pad;
		int p0 = (eyes[0] - startX) >> 1;
		int p1 = (n == 2) ? ((eyes[1] - startX) >> 1) : -1;
		if (p1 >= 0 && p1 < p0 + 2)
			p1 = p0 + 2;
		if (p0 >= 0 && p0 < numPairs && f[p0] != clearKey)
			f[p0] = lightRed;
		if (p1 >= 0 && p1 < numPairs && f[p1] != clearKey)
			f[p1] = lightRed;
		return;
	}
}

// QfG1 hero sitting facing right (view 83 loop 8): as when sitting down at the inn, with the
// outline gone his head is a fat pixel too wide, so the column at the back of his head (the
// leftmost one his head reaches, from fat pixel 2 so the chair back is left alone, down to the last
// row with skin) is cleared where it is his hair (yellow or brown) or grey
static void agiDemakeHeroSittingRightHead(int view, int loop, int h, byte clearKey, byte *fat, int fatW, int pad, int numPairs) {
	if (g_sci->getGameId() != GID_QFG1 || view != 83 || loop != 8)
		return;
	const byte skin = 12;
	const int firstP = 2;
	int lastSkin = -1;
	for (int y = 0; y < h && y < 11; ++y)
		for (int p = firstP + 1; p < numPairs; ++p)
			if (fat[y * fatW + pad + p] == skin)
				lastSkin = y;
	int left = numPairs;
	for (int y = 0; y <= lastSkin; ++y)
		for (int p = firstP; p < left; ++p)
			if (fat[y * fatW + pad + p] != clearKey) {
				left = p;
				break;
			}
	for (int y = 0; y <= lastSkin && left < numPairs; ++y) {
		byte &v = fat[y * fatW + pad + left];
		if (v == 6 || v == 14 || v == 8)
			v = clearKey;
	}
}

// QfG1 fight screen labels (view 803 loop 0, the 15x9 boxes left of the HP, SP and MP bars, the
// enemy's too): the 3 pixel letters collapse to a few dots, so the box is redrawn with 3 wide fat
// letters and a gap. That uses the box's left frame line, so the frame is drawn one fat pixel
// further left, outside the sprite (see the draw loop). The first letter is read off Sierra's
// pixels: H (a full crossbar on row 4), S (a top bar starting one in), otherwise M
static bool agiDemakeIsFightLabel(int view, int loop, int w, int h) {
	return g_sci->getGameId() == GID_QFG1 && view == 803 && loop == 0 && (w == 15 || w == 25) && h == 9;
}

static void agiDemakeFightLabel(int view, int loop, const SciSpan<const byte> &bitmap, int w, int h, byte *fat, int fatW, int pad, int numPairs) {
	if (!agiDemakeIsFightLabel(view, loop, w, h) || numPairs < 8)
		return;
	const byte black = 0, frame = 9, ink = 13;
	// the Mage's Maze "TIME" box (25 wide): T, a narrow I, M and a narrow E, frame kept on the right
	if (w == 25) {
		static const char *const kTime[5] = { "111.1.1.1.11", ".1..1.111.1.", ".1..1.1.1.11", ".1..1.1.1.1.", ".1..1.1.1.11" };
		for (int y = 1; y < 8 && y < h; ++y) {
			byte *f = fat + y * fatW + pad;
			for (int x = 0; x < 12 && x < numPairs - 1; ++x)
				f[x] = black;
		}
		for (int y = 0; y < 5; ++y)
			for (int x = 0; x < 12 && x < numPairs - 1; ++x)
				if (kTime[y][x] == '1')
					fat[(2 + y) * fatW + pad + x] = ink;
		return;
	}
	const byte *r2 = bitmap.getUnsafeDataAt(2 * w, w);
	const byte *r4 = bitmap.getUnsafeDataAt(4 * w, w);
	const char *first;
	if (r4[2] == ink && r4[3] == ink && r4[4] == ink && r4[5] == ink)
		first = "1.1" "1.1" "111" "1.1" "1.1";
	else if (r2[2] == black && r2[3] == ink && r2[4] == ink && r2[5] == ink)
		first = "111" "1.." "111" "..1" "111";
	else
		first = "1.1" "111" "1.1" "1.1" "1.1";
	const char *p = "111" "1.1" "111" "1.." "1..";
	for (int y = 1; y < 8 && y < h; ++y) {
		byte *f = fat + y * fatW + pad;
		for (int x = 0; x < 7; ++x)
			f[x] = black;
		f[7] = frame;
	}
	for (int y = 0; y < 5; ++y) {
		byte *f = fat + (2 + y) * fatW + pad;
		for (int x = 0; x < 3; ++x) {
			if (first[y * 3 + x] == '1')
				f[x] = ink;
			if (p[y * 3 + x] == '1')
				f[4 + x] = ink;
		}
	}
}

// QfG1 green meeps (views 60 and 61): the first fat pixel of each row, when it is green fur, is made
// light green, so his left edge shows against the grass. The fur clump he throws (61 loop 8) is all
// light green, with a black shadow under it (drawn in the draw loop)
static void agiDemakeMeepFur(int view, int loop, int h, byte clearKey, byte *fat, int fatW, int pad, int numPairs) {
	if (g_sci->getGameId() != GID_QFG1 || (view != 60 && view != 61))
		return;
	const byte green = 2, lightGreen = 10;
	const bool clump = (view == 61 && loop == 8);
	for (int y = 0; y < h; ++y) {
		byte *f = fat + y * fatW + pad;
		if (clump) {
			for (int p = 0; p < numPairs; ++p)
				if (f[p] == green)
					f[p] = lightGreen;
			continue;
		}
		for (int p = 0; p < numPairs; ++p)
			if (f[p] != clearKey) {
				if (f[p] == green)
					f[p] = lightGreen;
				break;
			}
	}
}

// QfG1 skull gate (view 23). The gate (loop 0 cel 0): its bars are checkerboard dithers (black with
// dark grey, and a brown dither), which collapse to black and grey dashes down each bar. Each fat
// pixel whose 2x2 block of Sierra's pixels is all bar colours (black, dark grey, brown or a dither
// pair) and not all one colour takes the darkest of them, so the bars come out solid.
// The talking skull (loop 2): its frames are the skull on a dark square of gate, which doesn't match
// the gate behind, so the dark surround (black, brown and dark grey reached from the edge) is left
// out and the gate shows round his face
static void agiDemakeSkullGate(int view, int loop, int cel, const SciSpan<const byte> &bitmap, int w, int h, byte clearKey, byte *fat, int fatW, int startX, int pad, int numPairs) {
	if (g_sci->getGameId() != GID_QFG1 || view != 23)
		return;
	if (loop == 0 && cel == 0) {
		auto isBar = [](byte c) { return c == 0 || c == 8 || c == 6 || c >= 16; };
		auto luma = [](byte c) { return c >= 16 ? kAgiDemakeLuma[6] : kAgiDemakeLuma[c]; };
		Common::Array<byte> out(h * numPairs);
		for (int y = 0; y < h; ++y)
			for (int p = 0; p < numPairs; ++p) {
				byte cur = fat[y * fatW + pad + p];
				out[y * numPairs + p] = cur;
				if (cur == clearKey || !isBar(cur))
					continue;
				const int y2 = ((y ^ 1) < h) ? (y ^ 1) : y;
				byte best = 0xFF;
				bool all = true, mixed = false;
				int n = 0;
				byte first = 0;
				for (int r = 0; r < 2 && all; ++r)
					for (int i = 0; i < 2; ++i) {
						const int x = startX + 2 * p + i;
						if (x < 0 || x >= w)
							continue;
						const byte c = bitmap[(r ? y2 : y) * w + x];
						if (c == clearKey)
							continue;
						if (!isBar(c)) {
							all = false;
							break;
						}
						if (n++ == 0)
							first = c;
						else if (c != first)
							mixed = true;
						if (best == 0xFF || luma(c) < luma(best))
							best = c;
					}
				if (all && mixed && best != 0xFF)
					out[y * numPairs + p] = best;
			}
		for (int y = 0; y < h; ++y)
			for (int p = 0; p < numPairs; ++p)
				fat[y * fatW + pad + p] = out[y * numPairs + p];
	} else if (loop == 2) {
		auto dark = [](byte c) { return c == 0 || c == 6 || c == 8; };
		Common::Array<bool> seen(h * numPairs, false);
		Common::Array<int> queue;
		for (int y = 0; y < h; ++y)
			for (int p = 0; p < numPairs; ++p)
				if ((y == 0 || y == h - 1 || p == 0 || p == numPairs - 1) && dark(fat[y * fatW + pad + p])) {
					seen[y * numPairs + p] = true;
					queue.push_back(y * numPairs + p);
				}
		for (uint i = 0; i < queue.size(); ++i) {
			const int y = queue[i] / numPairs, p = queue[i] % numPairs;
			static const int kD[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
			for (int d = 0; d < 4; ++d) {
				const int yy = y + kD[d][0], pp = p + kD[d][1];
				if (yy < 0 || yy >= h || pp < 0 || pp >= numPairs || seen[yy * numPairs + pp] || !dark(fat[yy * fatW + pad + pp]))
					continue;
				seen[yy * numPairs + pp] = true;
				queue.push_back(yy * numPairs + pp);
			}
		}
		for (uint i = 0; i < queue.size(); ++i)
			fat[(queue[i] / numPairs) * fatW + pad + queue[i] % numPairs] = clearKey;
	}
}

// QfG1 Mage's Maze spell buttons (view 32 loops 0, 1, 2 and 7: FETCH, OPEN, TRIGGER, FLAME): Sierra's
// small letters collapse to dots, so the middle of the button is redrawn: its colour, the dither strip
// either side as a fat checkerboard, and the word in 3x5 fat letters in Sierra's text colour, centred.
// The pressed frame (cel 1) gets the same, one row lower inside its shadowed edges
static void agiDemakeMazeButton(int view, int loop, int cel, int h, byte *fat, int fatW, int pad, int numPairs) {
	if (g_sci->getGameId() != GID_QFG1 || view != 32 || h != 11 || numPairs < 30)
		return;
	// cel 1 is the button pressed: its top and right edges are in shadow and its face one row lower
	const bool pressed = (cel == 1);
	static const struct { int loop; const char *word; byte fill, ink, dither; } kButtons[] = {
		{ 0, "FETCH", 10, 8, 3 }, { 1, "OPEN", 3, 0, 13 }, { 2, "TRIGGER", 13, 1, 5 }, { 7, "FLAME", 12, 0, 3 }
	};
	static const struct { char ch; const char *rows[5]; } kGlyphs[] = {
		{ 'F', { "111", "1..", "11.", "1..", "1.." } }, { 'E', { "111", "1..", "11.", "1..", "111" } },
		{ 'T', { "111", ".1.", ".1.", ".1.", ".1." } }, { 'C', { "111", "1..", "1..", "1..", "111" } },
		{ 'H', { "1.1", "1.1", "111", "1.1", "1.1" } }, { 'O', { "111", "1.1", "1.1", "1.1", "111" } },
		{ 'P', { "111", "1.1", "111", "1..", "1.." } }, { 'N', { "1...1", "11..1", "1.1.1", "1..11", "1...1" } },
		{ 'R', { "11.", "1.1", "11.", "1.1", "1.1" } }, { 'I', { "1", "1", "1", "1", "1" } },
		{ 'G', { "111", "1..", "1.1", "1.1", "111" } }, { 'L', { "1..", "1..", "1..", "1..", "111" } },
		{ 'A', { "111", "1.1", "111", "1.1", "1.1" } }, { 'M', { "1...1", "11.11", "1.1.1", "1...1", "1...1" } }
	};
	int b = -1;
	for (uint i = 0; i < ARRAYSIZE(kButtons); ++i)
		if (kButtons[i].loop == loop)
			b = i;
	if (b < 0)
		return;
	Common::String art[5];
	for (const char *c = kButtons[b].word; *c; ++c)
		for (uint g = 0; g < ARRAYSIZE(kGlyphs); ++g)
			if (kGlyphs[g].ch == *c)
				for (int y = 0; y < 5; ++y)
					art[y] += Common::String(art[y].empty() ? "" : ".") + kGlyphs[g].rows[y];
	const int aw = art[0].size();
	// the button ends one fat pixel in from its last column, so its frame is as far in on the right
	// as on the left: the last column is left clear and the one before it is the black frame
	const int last = numPairs - 2;
	const int down = pressed ? 1 : 0;
	const byte clearKey = fat[pad - 1];
	const byte shadow = fat[1 * fatW + pad + 1];	// pressed: the dark top edge, also down the right side
	for (int y = 0; y < h; ++y) {
		byte *f = fat + y * fatW + pad;
		if (y > 0 && y < h - 1)
			f[last - 1] = pressed ? shadow : f[last - 2];
		f[0] = 0;
		f[last] = 0;
		f[last + 1] = clearKey;
	}
	// the bottom edge runs the full width (no stray highlight in its first fat pixel)
	fat[(h - 2) * fatW + pad + 1] = fat[(h - 2) * fatW + pad + 2];
	const int inner = last - down;	// first column past the face
	const int x0 = 1 + (inner - 1 - aw) / 2;
	for (int y = 2 + down; y <= 8; ++y)
		for (int x = 1; x < inner; ++x) {
			byte v = kButtons[b].fill;
			if (y >= 4 + down && y <= 6 + down && (x < x0 - 1 || x >= x0 + aw + 1) && ((x + y - down) & 1) == 0)
				v = kButtons[b].dither;
			fat[y * fatW + pad + x] = v;
		}
	for (int y = 0; y < 5; ++y)
		for (int x = 0; x < aw; ++x)
			if (art[y][x] == '1')
				fat[(3 + down + y) * fatW + pad + x0 + x] = kButtons[b].ink;
}

// AGI demake fat-pixel lettering: one 3 wide letter (I 1, M and N 5) per character with a fat pixel
// between, 5 rows tall, or 8 rows tall for the big panel titles. A shadow colour, if given, goes one
// row under every letter pixel that has nothing of the letter below it
static const char *agiDemakeGlyph(char ch, bool tall, int row) {
	static const struct { char ch; const char *r[5]; } kSmall[] = {
		{ 'A', { "111", "1.1", "111", "1.1", "1.1" } }, { 'C', { "111", "1..", "1..", "1..", "111" } },
		{ 'D', { "11.", "1.1", "1.1", "1.1", "11." } }, { 'E', { "111", "1..", "11.", "1..", "111" } },
		{ 'F', { "111", "1..", "11.", "1..", "1.." } }, { 'G', { "111", "1..", "1.1", "1.1", "111" } },
		{ 'H', { "1.1", "1.1", "111", "1.1", "1.1" } }, { 'I', { "1", "1", "1", "1", "1" } },
		{ 'L', { "1..", "1..", "1..", "1..", "111" } }, { 'M', { "1...1", "11.11", "1.1.1", "1...1", "1...1" } },
		{ 'N', { "1...1", "11..1", "1.1.1", "1..11", "1...1" } }, { 'O', { "111", "1.1", "1.1", "1.1", "111" } },
		{ 'P', { "111", "1.1", "111", "1..", "1.." } }, { 'R', { "11.", "1.1", "11.", "1.1", "1.1" } },
		{ 'S', { "111", "1..", "111", "..1", "111" } }, { 'T', { "111", ".1.", ".1.", ".1.", ".1." } },
		{ 'Y', { "1.1", "1.1", ".1.", ".1.", ".1." } }, { ' ', { ".", ".", ".", ".", "." } },
		{ 'B', { "11.", "1.1", "11.", "1.1", "11." } }, { 'U', { "1.1", "1.1", "1.1", "1.1", "111" } },
		{ '!', { "1", "1", "1", ".", "1" } }
	};
	static const struct { char ch; const char *r[8]; } kTall[] = {
		{ 'A', { "111", "1.1", "1.1", "111", "1.1", "1.1", "1.1", "1.1" } },
		{ 'C', { "111", "1..", "1..", "1..", "1..", "1..", "1..", "111" } },
		{ 'E', { "111", "1..", "1..", "11.", "1..", "1..", "1..", "111" } },
		{ 'F', { "111", "1..", "1..", "11.", "1..", "1..", "1..", "1.." } },
		{ 'G', { "111", "1..", "1..", "1..", "1.1", "1.1", "1.1", "111" } },
		{ 'L', { "1..", "1..", "1..", "1..", "1..", "1..", "1..", "111" } },
		{ 'N', { "1...1", "11..1", "11..1", "1.1.1", "1.1.1", "1..11", "1..11", "1...1" } },
		{ 'O', { "111", "1.1", "1.1", "1.1", "1.1", "1.1", "1.1", "111" } },
		{ 'R', { "11.", "1.1", "1.1", "11.", "1.1", "1.1", "1.1", "1.1" } }
	};
	if (tall) {
		for (uint i = 0; i < ARRAYSIZE(kTall); ++i)
			if (kTall[i].ch == ch)
				return kTall[i].r[row];
	} else {
		for (uint i = 0; i < ARRAYSIZE(kSmall); ++i)
			if (kSmall[i].ch == ch)
				return kSmall[i].r[row];
	}
	return "";
}

static int agiDemakeWordWidth(const char *word, bool tall) {
	int w = 0;
	for (const char *c = word; *c; ++c)
		w += strlen(agiDemakeGlyph(*c, tall, 0)) + (c[1] ? 1 : 0);
	return w;
}

static void agiDemakeFatWord(byte *fat, int fatW, int pad, int numPairs, int h, int top, int left, const char *word, bool tall, byte ink, int shadow) {
	const int rows = tall ? 8 : 5;
	const int width = agiDemakeWordWidth(word, tall);
	Common::Array<bool> on(rows * width, false);
	int x = 0;
	for (const char *c = word; *c; ++c) {
		for (int y = 0; y < rows; ++y) {
			const char *g = agiDemakeGlyph(*c, tall, y);
			for (int i = 0; g[i]; ++i)
				if (g[i] == '1')
					on[y * width + x + i] = true;
		}
		x += strlen(agiDemakeGlyph(*c, tall, 0)) + 1;
	}
	for (int y = 0; y < rows; ++y)
		for (int i = 0; i < width; ++i) {
			const int fx = left + i, fy = top + y;
			if (!on[y * width + i] || fx < 0 || fx >= numPairs || fy < 0 || fy >= h)
				continue;
			fat[fy * fatW + pad + fx] = ink;
			const bool below = (y + 1 < rows) && on[(y + 1) * width + i];
			if (shadow >= 0 && !below && fy + 1 < h)
				fat[(fy + 1) * fatW + pad + fx] = shadow;
		}
}

// QfG1 knife throwing at the Thieves' Guild (view 341): its labels in fat letters, in Sierra's colours
// and places. The points panel (loop 1): POINT TALLY, THIEF, CHIEF. The FORCE panel (loop 2 cel 0) with
// HARD and SOFT, the ANGLE panel (loop 2 cel 1). The turn highlights (loop 5, widened in initData)
static void agiDemakeKnifeLabels(int view, int loop, int cel, int h, byte clearKey, byte *fat, int fatW, int pad, int numPairs) {
	if (g_sci->getGameId() != GID_QFG1 || view != 341)
		return;
	auto clearArea = [&](int y0, int y1, int x0, int x1, int only) {
		for (int y = y0; y <= y1 && y < h; ++y)
			for (int x = x0; x <= x1 && x < numPairs; ++x) {
				byte &v = fat[y * fatW + pad + x];
				if (only < 0 || v == only)
					v = clearKey;
			}
	};
	if (loop == 1 && cel == 0 && numPairs == 61) {
		clearArea(0, 5, 0, 60, -1);
		agiDemakeFatWord(fat, fatW, pad, numPairs, h, 0, (61 - agiDemakeWordWidth("POINT TALLY", false)) / 2, "POINT TALLY", false, 12, 0);
		clearArea(12, 17, 0, 18, -1);
		agiDemakeFatWord(fat, fatW, pad, numPairs, h, 12, 1, "THIEF", false, 14, 8);
		clearArea(12, 17, 43, 60, -1);
		agiDemakeFatWord(fat, fatW, pad, numPairs, h, 12, 44, "CHIEF", false, 11, 8);
	} else if (loop == 2 && cel == 0 && numPairs == 34) {
		clearArea(0, 8, 0, 33, -1);
		agiDemakeFatWord(fat, fatW, pad, numPairs, h, 0, (34 - agiDemakeWordWidth("FORCE", true)) / 2, "FORCE", true, 9, 0);
		clearArea(14, 18, 13, 27, 0);
		agiDemakeFatWord(fat, fatW, pad, numPairs, h, 14, 13, "HARD", false, 0, -1);
		clearArea(20, 24, 6, 20, 0);
		agiDemakeFatWord(fat, fatW, pad, numPairs, h, 20, 6, "SOFT", false, 0, -1);
	} else if (loop == 2 && cel == 1 && numPairs == 32) {
		clearArea(0, 8, 0, 31, -1);
		agiDemakeFatWord(fat, fatW, pad, numPairs, h, 0, (32 - agiDemakeWordWidth("ANGLE", true)) / 2, "ANGLE", true, 9, 0);
	} else if (loop == 5 && (cel == 0 || cel == 1)) {
		clearArea(0, h - 1, 0, numPairs - 1, -1);
		agiDemakeFatWord(fat, fatW, pad, numPairs, h, 0, 0, cel ? "CHIEF" : "THIEF", false, cel ? 9 : 6, 8);
	}
}

// QfG1 brigand fortress gate (view 93 loop 0): wide, so it was collapsed lighter-wins, which wiped out
// the black and red lines between its planks. Each fat pixel takes black if the pair has it, then
// red, then brown, then the darker of anything else
static void agiDemakeGatePlanks(int view, int loop, const SciSpan<const byte> &bitmap, int w, int h, byte clearKey, byte *fat, int fatW, int startX, int pad, int numPairs) {
	if (g_sci->getGameId() != GID_QFG1 || view != 93 || loop != 0)
		return;
	auto rank = [](byte c) { return c == 0 ? 0 : c == 4 ? 1 : c == 6 ? 2 : 3 + kAgiDemakeLuma[c & 15]; };
	for (int y = 0; y < h; ++y)
		for (int p = 0; p < numPairs; ++p) {
			byte best = clearKey;
			for (int i = 0; i < 2; ++i) {
				const int x = startX + 2 * p + i;
				if (x < 0 || x >= w)
					continue;
				const byte c = bitmap[y * w + x];
				if (c == clearKey || c > 15)
					continue;
				if (best == clearKey || rank(c) < rank(best))
					best = c;
			}
			fat[y * fatW + pad + p] = best;
		}
}

// QfG1 dining hall sign (view 95 loop 0 cel 0, made 17 rows tall in initData): brown, with its dark grey
// bottom edge, and "BRIGANDS" / "UBER ALLES!" in black fat letters
static void agiDemakeBrigandSign(int view, int loop, int cel, int h, byte *fat, int fatW, int pad, int numPairs) {
	if (g_sci->getGameId() != GID_QFG1 || view != 95 || loop != 0 || cel != 0 || h != 17 || numPairs != 50)
		return;
	for (int y = 0; y < h; ++y)
		for (int x = 0; x < numPairs; ++x) {
			byte v = 6;
			if (y == h - 1 || (y == h - 2 && (x == 0 || x == numPairs - 1)))
				v = 8;
			fat[y * fatW + pad + x] = v;
		}
	agiDemakeFatWord(fat, fatW, pad, numPairs, h, 2, (numPairs - agiDemakeWordWidth("BRIGANDS", false)) / 2, "BRIGANDS", false, 0, -1);
	agiDemakeFatWord(fat, fatW, pad, numPairs, h, 8, (numPairs - agiDemakeWordWidth("UBER ALLES!", false)) / 2, "UBER ALLES!", false, 0, -1);
}

// QfG1 Yorick's room (view 96): its writing and the fake door, redrawn. Each table is the whole cel in
// fat pixels ('.' clear), used when the cel is the size it was drawn for. EXIT (loop 7 cel 3) is made
// 39 wide in initData; SECRET DOOR (loop 7 cel 2) gets its letters at full resolution, drawn after
// the fat pixels (row, column in the cel). BOX (loop 4) keeps Sierra's sign, its letters stepping up
static const char *const kAgiYorickDoor[] = {
	"00...",
	"0.000",
	"0...0",
	"0...0",
	".0..0",
	"..00.",
	".....",
	".0...",
	"0.00.",
	"0...0",
	"0...0",
	".0..0",
	"..00.",
	".....",
	".0...",
	"0.00.",
	"0...0",
	"0...0",
	".0..0",
	"..00.",
	".....",
	"00...",
	"..000",
	"..0.0",
	".00.0",
	"0.0.0",
	"...0.",
	"....."
};
static const char *const kAgiYorickMe[] = {
	"............",
	"............",
	"............",
	".d...d.ddd..",
	".dd.dd.d....",
	".d.d.d.dd...",
	".d...d.d....",
	".d...d.ddd..",
	"............"
};
static const char *const kAgiYorickExit[] = {
	".............6......",
	"............06c.....",
	"............06c.....",
	"............06c.....",
	"............06c.....",
	"............06c.....",
	"............06c.....",
	"............06c.....",
	"............06c.....",
	"............06c.....",
	".....877777706c77.77",
	".....8fffffffffffff7",
	".....8f0ff0f0f0f0007",
	"....87f0ff0f0f0fff07",
	"....8ff0ff0ff0fff007",
	"....8ff0ff0f0f0fff07",
	".77777000f0f0f0f0007",
	"88888ffffffffffffff7",
	"88888ffffffffffffff7",
	".7000000000000000000"
};
static const char *const kAgiYorickFakeDoor[] = {
	"100000000000000000",
	"000000000000000000",
	"088888888888888888",
	"8888cccccccccccc90",
	"899c9989998999cc90",
	"899c9989998999cc90",
	"899c9989998999cc90",
	"899c9989998999cc90",
	"899c9989998999cc90",
	"899c9989998999cc90",
	"899c9989998999cc90",
	"899c9989998999cc90",
	"899c9989998999cc90",
	"899c9989998999cc90",
	"899c9989998999cc90",
	"899c9989998999cc90",
	"899c9989998999cc90",
	"899c9989998999cc90",
	"899c9989998999cc90",
	"899c9989998999cc90",
	"899c9989998999cc90",
	"899c9989998999cc90",
	"899c998999ccc9cc90",
	"899c998999ccc9cc90",
	"89cc998999c9c9cc90",
	"89cc998999ccc9cc90",
	"89cc998999ccc9c990",
	"89cc9989998999c990",
	"89cc9989998998c990",
	"89c89989998998c990",
	"89c89989998998c990",
	"89c89989998998c990",
	"89c89989998998c990",
	"89c89989998998c990",
	"89c89989998998c990",
	"89c89989998998c990",
	"89c89989998998c990",
	"89c89989998998c990",
	"89c89989998998c990",
	"89c89989998998c990",
	"89c89989998998c990",
	"89c89989998998c990",
	"89cccc89998998c990",
	"89cccccccccc98c990",
	"89989989ccccccc990",
	"89989989998cccc998",
	"899899899989989990",
	"899899899989989990",
	"899899899989989990",
	"899899899989989998",
	"899899899989989998",
	"899899899989989998",
	"899899899989989998",
	"899899899989989998",
	"899899899989989988",
	"c88888888888888888"
};
static const char *const kAgiYorickBox[] = {
	"......778",
	"...888ee8",
	"..87eeee8",
	"87eee0e08",
	"0eeee0e08",
	"0eeeee0e8",
	"0eeee0e08",
	"0eeee0e08",
	"0eeeeeee8",
	"0ee000ee8",
	"0ee0e0ee8",
	"0ee0e0ee8",
	"0ee0e0ee8",
	"0ee000ee8",
	"000eeeee8",
	"00e0eeee8",
	"000eeeee8",
	"00e0ee88.",
	"000ee88..",
	"0eee8....",
	"0e88.....",
	"088......",
	"08......."
};
static const int8 kAgiYorickSecretInk[][2] = {
	{ 17, 2 }, { 17, 3 }, { 17, 4 }, { 18, 1 }, { 18, 5 }, { 19, 1 }, { 20, 2 }, { 20, 3 }, { 20, 4 }, { 21, 5 }, { 22, 1 }, { 22, 5 }, { 23, 2 }, { 23, 3 }, { 23, 4 }, { 16, 7 }, { 16, 8 }, { 16, 9 }, { 16, 10 }, { 16, 11 }, { 17, 7 }, { 18, 7 }, { 19, 7 }, { 19, 8 }, { 19, 9 }, { 19, 10 }, { 20, 7 }, { 21, 7 }, { 22, 7 }, { 22, 8 }, { 22, 9 }, { 22, 10 }, { 22, 11 }, { 14, 14 }, { 14, 15 }, { 14, 16 }, { 15, 13 }, { 15, 17 }, { 16, 13 }, { 17, 13 }, { 18, 13 }, { 19, 13 }, { 20, 13 }, { 20, 17 }, { 21, 14 }, { 21, 15 }, { 21, 16 }, { 12, 19 }, { 12, 20 }, { 12, 21 }, { 12, 22 }, { 13, 19 }, { 13, 23 }, { 14, 19 }, { 14, 23 }, { 15, 19 }, { 15, 20 }, { 15, 21 }, { 15, 22 }, { 16, 19 }, { 16, 20 }, { 16, 21 }, { 16, 22 }, { 17, 19 }, { 17, 21 }, { 18, 19 }, { 18, 22 }, { 19, 19 }, { 19, 23 }, { 10, 25 }, { 10, 26 }, { 10, 27 }, { 10, 28 }, { 10, 29 }, { 11, 25 }, { 12, 25 }, { 13, 25 }, { 14, 25 }, { 14, 26 }, { 14, 27 }, { 14, 28 }, { 15, 25 }, { 16, 25 }, { 17, 25 }, { 18, 25 }, { 18, 26 }, { 18, 27 }, { 18, 28 }, { 18, 29 }, { 8, 31 }, { 8, 32 }, { 8, 33 }, { 8, 34 }, { 8, 35 }, { 8, 36 }, { 9, 33 }, { 9, 34 }, { 10, 33 }, { 10, 34 }, { 11, 33 }, { 11, 34 }, { 12, 33 }, { 12, 34 }, { 13, 33 }, { 13, 34 }, { 14, 33 }, { 14, 34 }, { 15, 33 }, { 15, 34 }, { 16, 33 }, { 16, 34 }, { 5, 41 }, { 5, 42 }, { 5, 43 }, { 5, 44 }, { 5, 45 }, { 6, 41 }, { 6, 46 }, { 7, 41 }, { 7, 46 }, { 8, 41 }, { 8, 46 }, { 9, 41 }, { 9, 46 }, { 10, 41 }, { 10, 46 }, { 11, 41 }, { 11, 46 }, { 12, 41 }, { 12, 46 }, { 13, 41 }, { 13, 46 }, { 14, 41 }, { 14, 42 }, { 14, 43 }, { 14, 44 }, { 14, 45 }, { 3, 49 }, { 3, 50 }, { 3, 51 }, { 3, 52 }, { 4, 48 }, { 4, 53 }, { 5, 48 }, { 5, 53 }, { 6, 48 }, { 6, 53 }, { 7, 48 }, { 7, 53 }, { 8, 48 }, { 8, 53 }, { 9, 48 }, { 9, 53 }, { 10, 48 }, { 10, 53 }, { 11, 48 }, { 11, 53 }, { 12, 49 }, { 12, 50 }, { 12, 51 }, { 12, 52 }, { 1, 56 }, { 1, 57 }, { 1, 58 }, { 1, 59 }, { 2, 55 }, { 2, 60 }, { 3, 55 }, { 3, 60 }, { 4, 55 }, { 4, 60 }, { 5, 55 }, { 5, 60 }, { 6, 55 }, { 6, 60 }, { 7, 55 }, { 7, 60 }, { 8, 55 }, { 8, 60 }, { 9, 55 }, { 9, 60 }, { 10, 55 }, { 10, 60 }, { 11, 56 }, { 11, 57 }, { 11, 58 }, { 11, 59 }, { -1, 62 }, { -1, 63 }, { -1, 64 }, { -1, 65 }, { -1, 66 }, { 0, 62 }, { 0, 67 }, { 1, 62 }, { 1, 67 }, { 2, 62 }, { 2, 67 }, { 3, 62 }, { 3, 67 }, { 4, 62 }, { 4, 63 }, { 4, 64 }, { 4, 65 }, { 4, 66 }, { 5, 62 }, { 5, 64 }, { 5, 65 }, { 6, 62 }, { 6, 64 }, { 6, 65 }, { 7, 62 }, { 7, 66 }, { 8, 62 }, { 8, 66 }, { 9, 62 }, { 9, 67 }
};

static const char *const *agiDemakeYorickTable(int view, int loop, int cel, int &rows, int &cols) {
	rows = cols = 0;
	if (g_sci->getGameId() != GID_QFG1 || view != 96)
		return nullptr;
	struct T { int loop, cel; const char *const *t; int rows, cols; };
	static const T kTables[] = {
		{ 7, 0, kAgiYorickDoor, ARRAYSIZE(kAgiYorickDoor), 5 },
		{ 7, 1, kAgiYorickMe, ARRAYSIZE(kAgiYorickMe), 12 },
		{ 7, 3, kAgiYorickExit, ARRAYSIZE(kAgiYorickExit), 20 },
		{ 1, 0, kAgiYorickFakeDoor, ARRAYSIZE(kAgiYorickFakeDoor), 18 },
		{ 4, 0, kAgiYorickBox, ARRAYSIZE(kAgiYorickBox), 9 }
	};
	for (uint i = 0; i < ARRAYSIZE(kTables); ++i)
		if (kTables[i].loop == loop && kTables[i].cel == cel) {
			rows = kTables[i].rows;
			cols = kTables[i].cols;
			return kTables[i].t;
		}
	return nullptr;
}

static void agiDemakeYorick(int view, int loop, int cel, int h, byte clearKey, byte *fat, int fatW, int pad, int numPairs) {
	int rows, cols;
	const char *const *t = agiDemakeYorickTable(view, loop, cel, rows, cols);
	if (g_sci->getGameId() == GID_QFG1 && view == 96 && loop == 7 && cel == 2) {
		// SECRET DOOR: nothing in fat pixels, the letters are all drawn at full resolution
		for (int y = 0; y < h; ++y)
			for (int x = 0; x < numPairs; ++x)
				fat[y * fatW + pad + x] = clearKey;
		return;
	}
	if (!t || rows != h || cols != numPairs)
		return;
	for (int y = 0; y < h; ++y)
		for (int x = 0; x < numPairs; ++x) {
			const char ch = t[y][x];
			fat[y * fatW + pad + x] = (ch == '.') ? clearKey : (byte)((ch >= 'a') ? ch - 'a' + 10 : ch - '0');
		}
}

// QfG1 Yorick in Elsa's room (view 97 loops 6 and 7, walking in and standing): the back column of his
// yellow hair (the rightmost fat pixels of yellow in his top six rows) is cleared, so his hair stops at
// the back of his head
static void agiDemakeYorickHair(int view, int loop, int h, byte clearKey, byte *fat, int fatW, int pad, int numPairs) {
	if (g_sci->getGameId() != GID_QFG1 || view != 97 || (loop != 6 && loop != 7))
		return;
	const int rows = MIN(6, h);
	int last = -1;
	for (int y = 0; y < rows; ++y)
		for (int x = 0; x < numPairs; ++x)
			if (fat[y * fatW + pad + x] == 14 && x > last)
				last = x;
	if (last < 0)
		return;
	for (int y = 0; y < rows; ++y)
		if (fat[y * fatW + pad + last] == 14)
			fat[y * fatW + pad + last] = clearKey;
}

// QfG1 hero facing you: his eyes (black, the left one black and cyan) sit 2 to 3 pixels apart with
// red or grey shading beside them, so the general eye rule misses them and they merge into one
// fat block. On the first row with two short dark runs and face between them, each eye gets its own
// fat pixel, one to the right of where the collapse puts them, with face colour between and beside.
static void agiDemakeHeroEyes(int view, const SciSpan<const byte> &bitmap, int w, int h, byte clearKey, byte *fat, int fatW, int startX, int pad, int numPairs, int loop = 0) {
	if (!agiDemakeIsQfgHero(view, loop))
		return;
	const byte black = 0, cyan = 3, skin = 12;
	for (int y = 0; y < h / 3; ++y) {
		const byte *row = bitmap.getUnsafeDataAt(y * w, w);
		int runs[2][2], n = 0;
		for (int x = 1; x + 1 < w && n <= 2; ) {
			if (row[x] != black && row[x] != cyan) {
				++x;
				continue;
			}
			int e = x;
			while (e + 1 < w && (row[e + 1] == black || row[e + 1] == cyan))
				++e;
			const bool flanked = e - x < 2 && e + 1 < w && row[x - 1] != clearKey && row[e + 1] != clearKey;
			if (flanked) {
				if (n < 2) {
					runs[n][0] = x;
					runs[n][1] = e;
				}
				++n;
			}
			x = e + 1;
		}
		if (n != 2 || runs[1][0] - runs[0][1] < 2)
			continue;
		bool face = false;
		for (int x = runs[0][1] + 1; x < runs[1][0]; ++x)
			if (row[x] == skin)
				face = true;
		if (!face)
			continue;
		// both eyes then go one fat pixel right, which centres them on his face
		const int shift = 1;
		const int pl = (runs[0][0] - startX) >> 1, pr = (runs[1][1] - startX) >> 1;
		if (pr - pl < 2 || pl < 0 || pr + shift >= numPairs)
			return;
		byte *f = fat + y * fatW + pad;
		if (f[pr + shift] == clearKey)
			return;
		for (int p = pl; p <= pr + shift; ++p)
			f[p] = skin;
		f[pl + shift] = black;
		f[pr + shift] = black;
		return;
	}
}

// Still images: an eye white is a run of 1 or 2 white pixels, at least 3 pixels in from the
// picture's sides (so the photo border never counts), with a non-white, non-transparent pixel
// on both sides
static bool agiDemakeIsEyeWhite(const SciSpan<const byte> &bitmap, int w, byte clearKey, byte white, int y, int x) {
	if (x < 0 || x >= w || bitmap[y * w + x] != white)
		return false;
	int s = x, e = x;
	while (s > 0 && bitmap[y * w + s - 1] == white)
		--s;
	while (e < w - 1 && bitmap[y * w + e + 1] == white)
		++e;
	if (e - s + 1 > 2 || s < 3 || e > w - 4)
		return false;
	const byte l = bitmap[y * w + s - 1], r = bitmap[y * w + e + 1];
	return l != clearKey && r != clearKey;
}

// Still images: collapse one row. Eye whites and pupils (1 to 3 non-white pixels with an eye
// white on both sides) beat other pixels. When an eye white and a pupil share a pair, the one
// that has no later chance to show wins, so every white and pupil keeps at least one fat pixel
// where the space allows. Otherwise the darker pixel wins.
static void agiDemakeEyeMasks(const SciSpan<const byte> &bitmap, int w, byte clearKey, byte white, int y, Common::Array<bool> &eye, Common::Array<bool> &pupil) {
	eye.clear();
	pupil.clear();
	eye.resize(w, false);
	pupil.resize(w, false);
	const byte *row = bitmap.getUnsafeDataAt(y * w, w);
	for (int x = 0; x < w; ++x)
		eye[x] = agiDemakeIsEyeWhite(bitmap, w, clearKey, white, y, x);
	for (int x = 0; x < w; ) {
		if (row[x] == white || row[x] == clearKey) {
			++x;
			continue;
		}
		const int s = x;
		while (x < w && row[x] != white && row[x] != clearKey)
			++x;
		const int e = x - 1;
		if (e - s + 1 <= 3 && s >= 1 && e <= w - 2 && eye[s - 1] && eye[e + 1])
			for (int i = s; i <= e; ++i)
				pupil[i] = true;
	}
}

// wide: pairs with no eye pixels use the wide-sprite rule (outline pixel wins, else lighter)
// instead of darker-wins
static void agiDemakeStillRow(const SciSpan<const byte> &bitmap, int w, byte clearKey, byte white, int y, int startX, Common::Array<byte> &out, bool wide = false) {
	Common::Array<int> runId(w, -1), runLast;
	Common::Array<bool> eye, pupil;
	const byte *row = bitmap.getUnsafeDataAt(y * w, w);
	agiDemakeEyeMasks(bitmap, w, clearKey, white, y, eye, pupil);
	for (int x = 0; x < w; ) {
		if (!eye[x] && !pupil[x]) {
			++x;
			continue;
		}
		const bool kind = eye[x];
		const int id = runLast.size();
		while (x < w && (kind ? eye[x] : pupil[x]))
			runId[x++] = id;
		runLast.push_back(x - 1);
	}
	Common::Array<bool> shown(runLast.size(), false);
	out.clear();
	for (int cx = startX; cx < w; cx += 2) {
		const byte a = (cx >= 0) ? row[cx] : clearKey;
		const byte b = (cx + 1 < w) ? row[cx + 1] : clearKey;
		if (a == clearKey) {
			out.push_back(b);
			continue;
		}
		if (b == clearKey || a == b) {
			out.push_back(a);
			if (cx >= 0 && runId[cx] >= 0)
				shown[runId[cx]] = true;
			if (cx + 1 < w && runId[cx + 1] >= 0)
				shown[runId[cx + 1]] = true;
			continue;
		}
		const int ra = (cx >= 0) ? runId[cx] : -1;
		const int rb = (cx + 1 < w) ? runId[cx + 1] : -1;
		const byte dark = agiDemakePick(a, b, clearKey);
		byte c;
		if (ra < 0 && rb < 0 && wide) {
			const bool aEdge = (cx == 0 || (cx > 0 && row[cx - 1] == clearKey));
			const bool bEdge = (cx + 1 == w - 1 || (cx + 2 < w && row[cx + 2] == clearKey));
			if (bEdge && !aEdge)
				c = b;
			else if (aEdge && !bEdge)
				c = a;
			else
				c = agiDemakePickLighter(a, b, clearKey);
		} else if (ra < 0 && rb < 0)
			c = dark;
		else if (ra < 0)
			c = b;
		else if (rb < 0)
			c = a;
		else if (ra == rb)
			c = dark;
		else {
			const bool aNeeds = !shown[ra] && runLast[ra] <= cx;
			const bool bNeeds = !shown[rb] && runLast[rb] <= cx + 1;
			if (aNeeds && !bNeeds)
				c = a;
			else if (bNeeds && !aNeeds)
				c = b;
			else if (aNeeds && bNeeds)
				c = dark;
			else if (!shown[ra] && shown[rb])
				c = a;
			else if (!shown[rb] && shown[ra])
				c = b;
			else
				c = dark;
		}
		if (c == a && ra >= 0)
			shown[ra] = true;
		if (c == b && rb >= 0)
			shown[rb] = true;
		out.push_back(c);
	}
}

// Still images: an eye that is two rows tall can come out as a checkerboard (white/pupil over
// pupil/white). Both rows then get the same layout, with the pupil on the side nearer the middle
// of the picture. Only applies where the white cells come from eye whites in the source.
static void agiDemakeFixEyeChecker(const SciSpan<const byte> &bitmap, int w, int h, byte clearKey, byte white, int startX, byte *fat, int fatW, int pad, int n) {
	Common::Array<Common::Array<bool> > eye(h), pupil(h);
	for (int y = 0; y < h; ++y)
		agiDemakeEyeMasks(bitmap, w, clearKey, white, y, eye[y], pupil[y]);
	for (int y = 0; y + 1 < h; ++y) {
		byte *r0 = fat + y * fatW + pad;
		byte *r1 = fat + (y + 1) * fatW + pad;
		for (int x = 0; x + 1 < n; ++x) {
			const byte a = r0[x], b = r0[x + 1], c = r1[x], d = r1[x + 1];
			const bool pattern1 = (a == white && d == white && b == c && b != white && b != clearKey);
			const bool pattern2 = (b == white && c == white && a == d && a != white && a != clearKey);
			if (!pattern1 && !pattern2)
				continue;
			const byte k = pattern1 ? b : a;
			bool ok = true;
			for (int yy = y; yy <= y + 1 && ok; ++yy) {
				const byte *rr = (yy == y) ? r0 : r1;
				for (int ff = x; ff <= x + 1 && ok; ++ff) {
					if (rr[ff] != white)
						continue;
					bool eyeish = false;
					for (int i = 0; i < 2; ++i) {
						const int col = startX + 2 * ff + i;
						if (col >= 0 && col < w && (eye[yy][col] || pupil[yy][col]))
							eyeish = true;
					}
					if (!eyeish)
						ok = false;
				}
			}
			if (!ok)
				continue;
			const bool leftHalf = (2 * x + 3 <= n);
			const byte p0 = leftHalf ? white : k;
			const byte p1 = leftHalf ? k : white;
			r0[x] = r1[x] = p0;
			r0[x + 1] = r1[x + 1] = p1;
		}
	}
}

// Still images (dialog icons such as portraits): the pairing start (cel column 0 or -1) is chosen
// per image to keep the most eye whites, counted as white runs inside the collapsed picture with
// a non-white pixel on both sides. Ties go to column -1.
static int agiDemakeEyeScore(const SciSpan<const byte> &bitmap, int w, int h, byte clearKey, int startX, byte white) {
	int score = 0;
	Common::Array<byte> row;
	for (int y = 0; y < h; ++y) {
		agiDemakeStillRow(bitmap, w, clearKey, white, y, startX, row);
		const int n = row.size();
		for (int x = 0; x < n; ) {
			if (row[x] != white) {
				++x;
				continue;
			}
			const int s = x;
			while (x < n && row[x] == white)
				++x;
			const int e = x - 1;
			if (s >= 3 && e <= n - 4 && row[s - 1] != clearKey && row[e + 1] != clearKey)
				++score;
		}
	}
	return score;
}

// Pairing phase for a loop, relative to the object's x anchor so every frame pairs the
// same way. Picks the phase that changes the fewest pixels, counting silhouette growth
// double, so 1 pixel protrusions (noses, hands) keep their own fat pixel.
int GfxView::agiDemakePhase(int16 loopNo) {
	loopNo = CLIP<int16>(loopNo, 0, _loop.size() - 1);
	if (_agiDemakePhase.size() != _loop.size()) {
		_agiDemakePhase.clear();
		_agiDemakePhase.resize(_loop.size());
		for (uint i = 0; i < _agiDemakePhase.size(); ++i)
			_agiDemakePhase[i] = -1;
	}
	if (_agiDemakePhase[loopNo] >= 0)
		return _agiDemakePhase[loopNo];

	int score[2] = { 0, 0 };
	for (int16 celNo = 0; celNo < (int16)getCelCount(loopNo); ++celNo) {
		const CelInfo *celInfo = getCelInfo(loopNo, celNo);
		const SciSpan<const byte> &bitmap = getBitmap(loopNo, celNo);
		const int w = celInfo->width;
		const byte clearKey = celInfo->clearKey;
		const int anchor = (w >> 1) - celInfo->displaceX;
		for (int phase = 0; phase < 2; ++phase) {
			const int startX = ((anchor + phase) & 1) ? -1 : 0;
			for (int y = 0; y < celInfo->height; ++y) {
				const byte *row = bitmap.getUnsafeDataAt(y * w, w);
				for (int cx = startX; cx < w; cx += 2) {
					byte a = (cx >= 0) ? row[cx] : clearKey;
					byte b = (cx + 1 < w) ? row[cx + 1] : clearKey;
					byte c = agiDemakePick(a, b, clearKey);
					const byte px[2] = { a, b };
					for (int i = 0; i < 2; ++i) {
						if ((px[i] == clearKey) != (c == clearKey))
							score[phase] += 2;
						else if (px[i] != c)
							score[phase] += 1;
					}
				}
			}
		}
	}
	_agiDemakePhase[loopNo] = (score[1] < score[0]) ? 1 : 0;
	return _agiDemakePhase[loopNo];
}

// Outermost opaque column of a row, or -1 if the row is empty
static int agiDemakeEdge(const byte *row, int w, byte clearKey, bool right) {
	if (right) {
		for (int x = w - 1; x >= 0; --x)
			if (row[x] != clearKey)
				return x;
	} else {
		for (int x = 0; x < w; ++x)
			if (row[x] != clearKey)
				return x;
	}
	return -1;
}

// Restore 1 to 3 row silhouette bumps (noses, hands, chins) that the collapse flattened,
// by pushing the bump one fat pixel past the rows above and below it
static void agiDemakeRepairBumps(const SciSpan<const byte> &bitmap, int w, int h, byte clearKey, byte *fat, int fatW, int startX) {
	Common::Array<int> src(h), out(h);
	for (int side = 0; side < 2; ++side) {
		const bool right = (side == 0);
		const int sgn = right ? 1 : -1;
		for (int y = 0; y < h; ++y) {
			src[y] = agiDemakeEdge(bitmap.getUnsafeDataAt(y * w, w), w, clearKey, right);
			out[y] = agiDemakeEdge(fat + y * fatW, fatW, clearKey, right);
		}
		for (int r0 = 1; r0 < h; ++r0) {
			for (int len = 1; len <= 3; ++len) {
				const int r1 = r0 + len - 1;
				if (r1 + 1 >= h)
					break;
				const int a = src[r0 - 1], b = src[r1 + 1];
				if (a < 0 || b < 0)
					continue;
				const int lim = right ? MAX(a, b) : MIN(a, b);
				bool bump = true;
				for (int k = r0; k <= r1; ++k)
					if (src[k] < 0 || sgn * src[k] <= sgn * lim)
						bump = false;
				if (!bump)
					continue;
				const int olim = right ? MAX(out[r0 - 1], out[r1 + 1]) : MIN(out[r0 - 1], out[r1 + 1]);
				for (int k = r0; k <= r1; ++k) {
					if (sgn * out[k] <= sgn * olim) {
						const int f = olim + sgn;
						if (f >= 0 && f < fatW)
							fat[k * fatW + f] = bitmap[k * w + src[k]];
					}
				}
				break;
			}
		}
	}
}

// Away-facing loop only: rebuild the head (top of sprite down to the neck) centred on the body's
// axis. Each row rounds up to the next width the fat grid allows, then rows where the original
// steps in are stepped in too, but never narrower than the original row.
static void agiDemakeAwayHead(const SciSpan<const byte> &bitmap, int w, int h, byte clearKey, byte *fat, int fatW, int startX, int pad) {
	Common::Array<int> rowL(h, -1), rowR(h, -1), width(h, 0);
	for (int y = 0; y < h; ++y) {
		const byte *row = bitmap.getUnsafeDataAt(y * w, w);
		rowL[y] = agiDemakeEdge(row, w, clearKey, false);
		rowR[y] = agiDemakeEdge(row, w, clearKey, true);
		if (rowL[y] >= 0)
			width[y] = rowR[y] - rowL[y] + 1;
	}
	int top = -1;
	for (int y = 0; y < h && top < 0; ++y)
		if (width[y] > 0)
			top = y;
	if (top < 0)
		return;
	// Neck: first local minimum in width after the head has widened
	int neck = -1;
	bool grew = false;
	for (int y = top + 1; y < h - 1; ++y) {
		if (width[y] > width[y - 1])
			grew = true;
		if (grew && width[y] < width[y - 1] && width[y + 1] > width[y]) {
			neck = y;
			break;
		}
	}
	if (neck < 0)
		return;
	for (int y = top; y <= neck; ++y) {
		const byte *row = bitmap.getUnsafeDataAt(y * w, w);
		int mismatches = 0;
		for (int x = rowL[y]; x <= rowR[y]; ++x)
			if (row[x] != row[rowL[y] + rowR[y] - x])
				++mismatches;
		if (mismatches > 2)
			return;
	}
	// Body axis: centre of the widest collapsed row below the neck
	int bestY = -1, bestL = 0, bestR = 0;
	for (int y = neck + 1; y < h; ++y) {
		const int l = agiDemakeEdge(fat + y * fatW, fatW, clearKey, false);
		const int r = agiDemakeEdge(fat + y * fatW, fatW, clearKey, true);
		if (l >= 0 && (bestY < 0 || r - l > bestR - bestL)) {
			bestY = y;
			bestL = l;
			bestR = r;
		}
	}
	if (bestY < 0)
		return;
	const int eL = 2 * (bestL - pad) + startX;
	const int eR = 2 * (bestR - pad) + startX + 2;
	const int X = (eL + eR) / 2;
	const int parity = ((X - startX) & 1) ? 1 : 0;

	Common::Array<int> n(h, 0);
	for (int y = top; y <= neck; ++y) {
		int v = (width[y] + 1) / 2;
		if ((v & 1) != parity)
			++v;
		n[y] = v;
	}
	for (int pass = 0; pass < 4; ++pass) {
		bool changed = false;
		for (int y = top; y < neck; ++y) {
			if (width[y] != width[y + 1] && n[y] == n[y + 1]) {
				const int s = (width[y] < width[y + 1]) ? y : y + 1;
				if (n[s] - 2 >= 1 && 2 * (n[s] - 2) >= width[s]) {
					n[s] -= 2;
					changed = true;
				}
			}
		}
		if (!changed)
			break;
	}
	for (int y = top; y <= neck; ++y) {
		const byte *row = bitmap.getUnsafeDataAt(y * w, w);
		const int E0 = X - n[y];
		const int p0 = (E0 - startX) / 2 + pad;
		if (p0 < 0 || p0 + n[y] > fatW)
			continue;
		byte *o = fat + y * fatW;
		for (int f = 0; f < fatW; ++f)
			o[f] = clearKey;
		for (int j = 0; j < (n[y] + 1) / 2; ++j) {
			byte v[2];
			for (int i = 0; i < 2; ++i) {
				const int c = E0 + 2 * j + i;
				v[i] = (c < rowL[y]) ? row[rowL[y]] : ((c > rowR[y]) ? row[rowR[y]] : row[c]);
			}
			o[p0 + j] = o[p0 + n[y] - 1 - j] = agiDemakePick(v[0], v[1], clearKey);
		}
	}
}

// Towards-facing loop only: the eyes are the topmost pair of single black pixels with opaque,
// non-black pixels either side, at least one of them skin (light red or brown), 2 to 4 pixels
// apart on one row in the top third (plus the rows straight below with a pair in the same
// columns). If the collapse put them in neighbouring fat
// pixels, one eye moves out by one fat pixel onto the face, never onto the head's outline (the
// pixel it moves onto must have another opaque pixel beyond it), and the face colour it covered
// fills the gap. If both sides have room, the gap goes nearer the middle of the source eyes.
static void agiDemakeSpreadEyes(const SciSpan<const byte> &bitmap, int w, int h, byte clearKey, byte *fat, int fatW, int startX, int pad) {
	const byte black = 0, skinLight = 12, skinDark = 6;
	int eyeL = -1, eyeR = -1;
	for (int y = 0; y < h / 3; ++y) {
		const byte *row = bitmap.getUnsafeDataAt(y * w, w);
		Common::Array<int> eyes;
		for (int x = 1; x + 1 < w; ++x)
			if (row[x] == black && row[x - 1] != clearKey && row[x - 1] != black && row[x + 1] != clearKey && row[x + 1] != black &&
				(row[x - 1] == skinLight || row[x - 1] == skinDark || row[x + 1] == skinLight || row[x + 1] == skinDark))
				eyes.push_back(x);
		int xl = -1, xr = -1;
		for (uint i = 0; i + 1 < eyes.size() && xl < 0; ++i)
			if (eyes[i + 1] - eyes[i] >= 2 && eyes[i + 1] - eyes[i] <= 4 && (eyeL < 0 || (eyes[i] == eyeL && eyes[i + 1] == eyeR))) {
				xl = eyes[i];
				xr = eyes[i + 1];
			}
		if (xl < 0) {
			if (eyeL >= 0)
				break;	// the rows below the eyes
			continue;
		}
		eyeL = xl;
		eyeR = xr;
		{
			const int pl = (xl - startX) >> 1, pr = (xr - startX) >> 1;
			if (pr != pl + 1)
				continue;
			byte *f = fat + y * fatW + pad;
			if (f[pl] != black || f[pr] != black)
				continue;
			// pl - 2 and pr + 2 stay inside the padded row (pad is at least 3)
			const bool canL = f[pl - 1] != clearKey && f[pl - 1] != black && f[pl - 2] != clearKey;
			const bool canR = f[pr + 1] != clearKey && f[pr + 1] != black && f[pr + 2] != clearKey;
			if (!canL && !canR)
				continue;
			bool left = canL;
			if (canL && canR) {
				// gap cell after a left move is pl, after a right move pr: pick the one whose
				// source columns are nearer the eyes' midpoint
				const int mid2 = xl + xr;	// twice the midpoint
				const int dL = ABS(2 * (startX + 2 * pl) + 1 - mid2);
				const int dR = ABS(2 * (startX + 2 * pr) + 1 - mid2);
				left = dL <= dR;
			}
			const int target = left ? pl - 1 : pr + 1;
			const int gap = left ? pl : pr;
			const byte face = f[target];
			f[target] = black;
			f[gap] = face;
		}
	}
}

// PQ2 diving suits: Sonny's head is copied from his standard walking view (view 0) onto the
// suit views, so towards gets his normal eyes and away his normal rounded head. The rows are
// view 0's own collapsed head (fat pixels), towards rows 0-8 and away rows 1-9, and land on rows
// 0-8 of the suit cel. View 0's head is an odd width, the suit bodies are centred between two fat
// pixels, so the head sits half a fat pixel left of the body's centre. hood: hair colour (yellow)
// becomes black, for the hooded diver (views 21 and 22, towards only: his own away head is kept).
struct AgiDemakeHeadCopy {
	SciGameId game;
	int16 view, loop;
	int16 left;			// first fat pixel (pair) of the pattern in the suit cel
	bool hood;
	const char *const *rows;	// 9 rows of fat pixels, view 0 colours ('.' transparent)
};
static const char *const kAgiDemakeSonnyHeadTowards[9] = {
	".....", ".eee.", ".eeee", "e44ee", "e44ee", "e4c4e", ".040.", ".c4c.", ".ccc."
};
static const char *const kAgiDemakeSonnyHeadAway[9] = {
	".....", ".eee.", "eeeee", "eeeee", "eeeee", "eeeee", "eeeee", ".eee.", ".eee."
};
static const AgiDemakeHeadCopy kAgiDemakeHeadCopies[] = {
	{ GID_PQ2, 17, 2, 2, false, kAgiDemakeSonnyHeadTowards },	// Sonny, wetsuit
	{ GID_PQ2, 17, 3, 2, false, kAgiDemakeSonnyHeadAway },
	{ GID_PQ2, 22, 2, 2, true, kAgiDemakeSonnyHeadTowards },	// hooded diver suit
	{ GID_PQ2, 21, 2, 1, true, kAgiDemakeSonnyHeadTowards },	// the same hooded diver out of his scuba gear
};

static void agiDemakeCopyHead(GuiResourceId view, int16 loop, int h, byte clearKey, byte *fat, int fatW, int pad, int numPairs) {
	for (uint i = 0; i < ARRAYSIZE(kAgiDemakeHeadCopies); ++i) {
		const AgiDemakeHeadCopy &hc = kAgiDemakeHeadCopies[i];
		if (hc.game != g_sci->getGameId() || hc.view != view || hc.loop != loop)
			continue;
		for (int r = 0; r < 9 && r < h; ++r) {
			byte *f = fat + r * fatW + pad;
			for (int p = 0; p < numPairs; ++p)
				f[p] = clearKey;
			const char *row = hc.rows[r];
			for (int x = 0; row[x]; ++x) {
				if (row[x] == '.' || hc.left + x < 0 || hc.left + x >= numPairs)
					continue;
				byte c = (byte)((row[x] <= '9') ? row[x] - '0' : row[x] - 'a' + 10);
				if (hc.hood && c == 14)
					c = 0;
				f[hc.left + x] = c;
			}
		}
		return;
	}
}

// Single fat pixel touch-ups after the collapse, for heads too narrow for the general rules.
// Each changes one fat pixel (pair) of a loop's cels from one colour to another, and only when
// it still has the expected colour, so frames that differ are left alone.
struct AgiDemakeFatPatch {
	SciGameId game;
	int16 view, loop, cel;		// cel -1: every cel of the loop
	int16 row, pair;
	byte from, to;
};
static const AgiDemakeFatPatch kAgiDemakeFatPatches[] = {
	// QfG1 Elsa in front of her desk (view 97 loop 4 cel 9 as the brigand, loop 5 as the princess): her
	// eyes apart on Sierra's eye row, and the princess's white mouth pixel. The transformation close-up
	// (view 473): the right eye a mirror of the left
	{ GID_QFG1, 97, 4, 9, 4, 7, 0, 4 }, { GID_QFG1, 97, 4, 9, 5, 4, 0, 12 }, { GID_QFG1, 97, 4, 9, 5, 6, 0, 12 }, { GID_QFG1, 97, 4, 9, 5, 7, 4, 0 }, { GID_QFG1, 97, 4, 9, 6, 7, 0, 12 },
	{ GID_QFG1, 97, 5, 0, 4, 5, 0, 12 }, { GID_QFG1, 97, 5, 0, 7, 5, 12, 15 },
	{ GID_QFG1, 97, 5, 1, 5, 5, 0, 12 },
	{ GID_QFG1, 97, 5, 2, 5, 5, 0, 12 },
	{ GID_QFG1, 97, 5, 3, 5, 5, 0, 12 },
	{ GID_QFG1, 97, 5, 4, 5, 5, 0, 12 },
	{ GID_QFG1, 97, 5, 5, 5, 5, 0, 12 },
	{ GID_QFG1, 473, 0, 0, 20, 23, 0, 4 }, { GID_QFG1, 473, 0, 0, 20, 25, 0, 4 }, { GID_QFG1, 473, 0, 0, 21, 24, 7, 1 }, { GID_QFG1, 473, 0, 0, 21, 25, 0, 15 },
	{ GID_QFG1, 473, 0, 1, 15, 21, 0, 12 }, { GID_QFG1, 473, 0, 1, 15, 24, 0, 12 }, { GID_QFG1, 473, 0, 1, 16, 21, 7, 15 }, { GID_QFG1, 473, 0, 1, 16, 22, 7, 0 }, { GID_QFG1, 473, 0, 1, 16, 23, 4, 15 }, { GID_QFG1, 473, 0, 1, 16, 24, 0, 12 }, { GID_QFG1, 473, 0, 1, 17, 21, 0, 12 }, { GID_QFG1, 473, 0, 1, 17, 24, 0, 12 },
	// QfG1 Baba Yaga (view 20): her green eyes, which the collapse loses in her face, back in at Sierra's
	// places, with light red skin between and no green band across
	{ GID_QFG1, 20, 1, 0, 5, 2, 2, 12 }, { GID_QFG1, 20, 1, 0, 5, 4, 2, 12 },
	{ GID_QFG1, 20, 1, 1, 5, 2, 2, 12 }, { GID_QFG1, 20, 1, 1, 5, 4, 2, 12 },
	{ GID_QFG1, 20, 3, 1, 5, 2, 12, 10 },
	{ GID_QFG1, 20, 3, 2, 5, 2, 12, 10 },
	{ GID_QFG1, 20, 3, 3, 5, 2, 12, 10 },
	{ GID_QFG1, 20, 3, 4, 5, 2, 12, 10 },
	{ GID_QFG1, 20, 3, 5, 5, 2, 12, 10 },
	{ GID_QFG1, 20, 4, 0, 5, 1, 12, 10 }, { GID_QFG1, 20, 4, 0, 5, 3, 12, 10 },
	{ GID_QFG1, 20, 4, 1, 5, 1, 12, 10 }, { GID_QFG1, 20, 4, 1, 5, 3, 12, 10 },
	{ GID_QFG1, 20, 4, 2, 5, 1, 12, 10 }, { GID_QFG1, 20, 4, 2, 5, 3, 12, 10 },
	{ GID_QFG1, 20, 4, 3, 5, 1, 12, 10 }, { GID_QFG1, 20, 4, 3, 5, 3, 12, 10 },
	{ GID_QFG1, 20, 4, 4, 5, 1, 12, 10 }, { GID_QFG1, 20, 4, 4, 5, 3, 12, 10 },
	{ GID_QFG1, 20, 5, 0, 5, 3, 12, 10 }, { GID_QFG1, 20, 5, 0, 5, 5, 12, 10 },
	{ GID_QFG1, 20, 5, 1, 5, 3, 12, 10 }, { GID_QFG1, 20, 5, 1, 5, 5, 12, 10 },
	{ GID_QFG1, 20, 5, 2, 5, 3, 12, 10 }, { GID_QFG1, 20, 5, 2, 5, 5, 12, 10 },
	{ GID_QFG1, 20, 5, 3, 5, 3, 12, 10 }, { GID_QFG1, 20, 5, 3, 5, 5, 12, 10 },
	{ GID_QFG1, 20, 5, 4, 5, 3, 12, 10 }, { GID_QFG1, 20, 5, 4, 5, 5, 12, 10 },
	{ GID_QFG1, 20, 6, 0, 5, 4, 12, 10 }, { GID_QFG1, 20, 6, 0, 5, 6, 12, 10 },
	{ GID_QFG1, 20, 6, 1, 5, 5, 12, 10 }, { GID_QFG1, 20, 6, 1, 5, 7, 12, 10 },
	{ GID_QFG1, 20, 6, 2, 5, 6, 12, 10 }, { GID_QFG1, 20, 6, 2, 5, 8, 12, 10 },
	{ GID_QFG1, 20, 6, 4, 5, 5, 12, 10 }, { GID_QFG1, 20, 6, 4, 5, 7, 12, 10 },
	{ GID_QFG1, 20, 7, 0, 5, 5, 12, 10 }, { GID_QFG1, 20, 7, 0, 5, 7, 12, 10 },
	{ GID_QFG1, 20, 7, 1, 5, 6, 12, 10 }, { GID_QFG1, 20, 7, 1, 5, 8, 12, 10 },
	{ GID_QFG1, 20, 7, 2, 5, 5, 12, 10 }, { GID_QFG1, 20, 7, 2, 5, 7, 12, 10 },
	{ GID_QFG1, 20, 7, 4, 6, 7, 12, 10 }, { GID_QFG1, 20, 7, 4, 6, 9, 12, 10 },
	{ GID_QFG1, 20, 7, 5, 5, 8, 12, 10 }, { GID_QFG1, 20, 7, 5, 5, 10, 12, 10 },
	{ GID_QFG1, 20, 9, 0, 6, 9, 12, 10 },
	{ GID_QFG1, 20, 9, 1, 7, 9, 12, 10 },
	{ GID_QFG1, 20, 9, 2, 5, 8, 12, 10 },
	{ GID_QFG1, 20, 9, 3, 5, 9, 12, 10 },
	// QfG1 Otto in town (view 606): his green eyes with light red skin between (spread left as on view
	// 602), grey brows only above each eye, the symmetrical top of the head from loop 3 cel 2, the right
	// side of his head mirroring the left, and his white tooth back in at Sierra's row
	{ GID_QFG1, 606, 1, 0, 2, 6, 0, 12 }, { GID_QFG1, 606, 1, 0, 2, 7, 1, 0 }, { GID_QFG1, 606, 1, 0, 3, 4, 8, 12 }, { GID_QFG1, 606, 1, 0, 3, 6, 0, 12 }, { GID_QFG1, 606, 1, 0, 3, 7, 1, 0 }, { GID_QFG1, 606, 1, 0, 4, 3, 4, 10 }, { GID_QFG1, 606, 1, 0, 4, 5, 4, 10 }, { GID_QFG1, 606, 1, 0, 7, 7, 0, 12 }, { GID_QFG1, 606, 1, 0, 7, 8, 1, 0 }, { GID_QFG1, 606, 1, 0, 8, 7, 0, 12 }, { GID_QFG1, 606, 1, 0, 8, 8, 1, 0 }, { GID_QFG1, 606, 1, 0, 9, 3, 12, 15 }, { GID_QFG1, 606, 1, 0, 9, 7, 0, 4 }, { GID_QFG1, 606, 1, 0, 9, 8, 1, 0 }, { GID_QFG1, 606, 1, 0, 10, 7, 0, 4 }, { GID_QFG1, 606, 1, 0, 10, 8, 1, 0 }, { GID_QFG1, 606, 1, 0, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 1, 1, 2, 6, 0, 12 }, { GID_QFG1, 606, 1, 1, 2, 7, 1, 0 }, { GID_QFG1, 606, 1, 1, 3, 4, 8, 12 }, { GID_QFG1, 606, 1, 1, 3, 6, 0, 12 }, { GID_QFG1, 606, 1, 1, 3, 7, 1, 0 }, { GID_QFG1, 606, 1, 1, 4, 3, 4, 10 }, { GID_QFG1, 606, 1, 1, 4, 5, 4, 10 }, { GID_QFG1, 606, 1, 1, 7, 7, 0, 12 }, { GID_QFG1, 606, 1, 1, 7, 8, 1, 0 }, { GID_QFG1, 606, 1, 1, 8, 7, 0, 12 }, { GID_QFG1, 606, 1, 1, 8, 8, 1, 0 }, { GID_QFG1, 606, 1, 1, 9, 7, 0, 4 }, { GID_QFG1, 606, 1, 1, 9, 8, 1, 0 }, { GID_QFG1, 606, 1, 1, 10, 7, 0, 4 }, { GID_QFG1, 606, 1, 1, 10, 8, 1, 0 }, { GID_QFG1, 606, 1, 1, 11, 3, 12, 15 }, { GID_QFG1, 606, 1, 1, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 1, 2, 2, 6, 0, 12 }, { GID_QFG1, 606, 1, 2, 2, 7, 1, 0 }, { GID_QFG1, 606, 1, 2, 3, 4, 8, 12 }, { GID_QFG1, 606, 1, 2, 3, 6, 0, 12 }, { GID_QFG1, 606, 1, 2, 3, 7, 1, 0 }, { GID_QFG1, 606, 1, 2, 4, 3, 4, 10 }, { GID_QFG1, 606, 1, 2, 4, 5, 4, 10 }, { GID_QFG1, 606, 1, 2, 7, 7, 0, 12 }, { GID_QFG1, 606, 1, 2, 7, 8, 1, 0 }, { GID_QFG1, 606, 1, 2, 8, 7, 0, 12 }, { GID_QFG1, 606, 1, 2, 8, 8, 1, 0 }, { GID_QFG1, 606, 1, 2, 9, 7, 0, 4 }, { GID_QFG1, 606, 1, 2, 9, 8, 1, 0 }, { GID_QFG1, 606, 1, 2, 10, 7, 0, 4 }, { GID_QFG1, 606, 1, 2, 10, 8, 1, 0 }, { GID_QFG1, 606, 1, 2, 11, 3, 12, 15 }, { GID_QFG1, 606, 1, 2, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 1, 3, 2, 6, 0, 12 }, { GID_QFG1, 606, 1, 3, 2, 7, 1, 0 }, { GID_QFG1, 606, 1, 3, 3, 4, 8, 12 }, { GID_QFG1, 606, 1, 3, 3, 6, 0, 12 }, { GID_QFG1, 606, 1, 3, 3, 7, 1, 0 }, { GID_QFG1, 606, 1, 3, 4, 3, 4, 10 }, { GID_QFG1, 606, 1, 3, 4, 5, 4, 10 }, { GID_QFG1, 606, 1, 3, 7, 7, 0, 12 }, { GID_QFG1, 606, 1, 3, 7, 8, 1, 0 }, { GID_QFG1, 606, 1, 3, 8, 7, 0, 12 }, { GID_QFG1, 606, 1, 3, 8, 8, 1, 0 }, { GID_QFG1, 606, 1, 3, 9, 7, 0, 4 }, { GID_QFG1, 606, 1, 3, 9, 8, 1, 0 }, { GID_QFG1, 606, 1, 3, 10, 7, 0, 4 }, { GID_QFG1, 606, 1, 3, 10, 8, 1, 0 }, { GID_QFG1, 606, 1, 3, 11, 3, 12, 15 }, { GID_QFG1, 606, 1, 3, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 1, 4, 2, 6, 0, 12 }, { GID_QFG1, 606, 1, 4, 2, 7, 1, 0 }, { GID_QFG1, 606, 1, 4, 3, 4, 8, 12 }, { GID_QFG1, 606, 1, 4, 3, 6, 0, 12 }, { GID_QFG1, 606, 1, 4, 3, 7, 1, 0 }, { GID_QFG1, 606, 1, 4, 5, 3, 4, 10 }, { GID_QFG1, 606, 1, 4, 5, 5, 4, 10 }, { GID_QFG1, 606, 1, 4, 7, 7, 0, 12 }, { GID_QFG1, 606, 1, 4, 7, 8, 1, 0 }, { GID_QFG1, 606, 1, 4, 8, 7, 0, 12 }, { GID_QFG1, 606, 1, 4, 8, 8, 1, 0 }, { GID_QFG1, 606, 1, 4, 9, 7, 0, 4 }, { GID_QFG1, 606, 1, 4, 9, 8, 1, 0 }, { GID_QFG1, 606, 1, 4, 10, 7, 0, 4 }, { GID_QFG1, 606, 1, 4, 10, 8, 1, 0 }, { GID_QFG1, 606, 1, 4, 11, 3, 12, 15 }, { GID_QFG1, 606, 1, 4, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 1, 5, 2, 6, 0, 12 }, { GID_QFG1, 606, 1, 5, 2, 7, 1, 0 }, { GID_QFG1, 606, 1, 5, 3, 4, 8, 12 }, { GID_QFG1, 606, 1, 5, 3, 6, 0, 12 }, { GID_QFG1, 606, 1, 5, 3, 7, 1, 0 }, { GID_QFG1, 606, 1, 5, 4, 3, 4, 10 }, { GID_QFG1, 606, 1, 5, 4, 5, 4, 10 }, { GID_QFG1, 606, 1, 5, 7, 7, 0, 12 }, { GID_QFG1, 606, 1, 5, 7, 8, 1, 0 }, { GID_QFG1, 606, 1, 5, 8, 7, 0, 12 }, { GID_QFG1, 606, 1, 5, 8, 8, 1, 0 }, { GID_QFG1, 606, 1, 5, 9, 7, 0, 4 }, { GID_QFG1, 606, 1, 5, 9, 8, 1, 0 }, { GID_QFG1, 606, 1, 5, 10, 7, 0, 4 }, { GID_QFG1, 606, 1, 5, 10, 8, 1, 0 }, { GID_QFG1, 606, 1, 5, 11, 3, 12, 15 }, { GID_QFG1, 606, 1, 5, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 1, 6, 2, 6, 0, 12 }, { GID_QFG1, 606, 1, 6, 2, 7, 1, 0 }, { GID_QFG1, 606, 1, 6, 3, 4, 8, 12 }, { GID_QFG1, 606, 1, 6, 3, 6, 0, 12 }, { GID_QFG1, 606, 1, 6, 3, 7, 1, 0 }, { GID_QFG1, 606, 1, 6, 4, 3, 4, 10 }, { GID_QFG1, 606, 1, 6, 4, 5, 4, 10 }, { GID_QFG1, 606, 1, 6, 7, 7, 0, 12 }, { GID_QFG1, 606, 1, 6, 7, 8, 1, 0 }, { GID_QFG1, 606, 1, 6, 8, 7, 0, 12 }, { GID_QFG1, 606, 1, 6, 8, 8, 1, 0 }, { GID_QFG1, 606, 1, 6, 9, 3, 12, 15 }, { GID_QFG1, 606, 1, 6, 9, 7, 0, 4 }, { GID_QFG1, 606, 1, 6, 9, 8, 1, 0 }, { GID_QFG1, 606, 1, 6, 10, 7, 0, 4 }, { GID_QFG1, 606, 1, 6, 10, 8, 1, 0 }, { GID_QFG1, 606, 1, 6, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 1, 7, 2, 6, 0, 12 }, { GID_QFG1, 606, 1, 7, 2, 7, 1, 0 }, { GID_QFG1, 606, 1, 7, 3, 4, 8, 12 }, { GID_QFG1, 606, 1, 7, 3, 6, 0, 12 }, { GID_QFG1, 606, 1, 7, 3, 7, 1, 0 }, { GID_QFG1, 606, 1, 7, 4, 3, 4, 10 }, { GID_QFG1, 606, 1, 7, 4, 5, 4, 10 }, { GID_QFG1, 606, 1, 7, 7, 7, 0, 12 }, { GID_QFG1, 606, 1, 7, 7, 8, 1, 0 }, { GID_QFG1, 606, 1, 7, 8, 7, 0, 12 }, { GID_QFG1, 606, 1, 7, 8, 8, 1, 0 }, { GID_QFG1, 606, 1, 7, 9, 3, 12, 15 }, { GID_QFG1, 606, 1, 7, 9, 7, 0, 4 }, { GID_QFG1, 606, 1, 7, 9, 8, 1, 0 }, { GID_QFG1, 606, 1, 7, 10, 7, 0, 4 }, { GID_QFG1, 606, 1, 7, 10, 8, 1, 0 }, { GID_QFG1, 606, 1, 7, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 2, 0, 2, 6, 0, 12 }, { GID_QFG1, 606, 2, 0, 2, 7, 1, 0 }, { GID_QFG1, 606, 2, 0, 3, 4, 8, 12 }, { GID_QFG1, 606, 2, 0, 3, 6, 0, 12 }, { GID_QFG1, 606, 2, 0, 3, 7, 1, 0 }, { GID_QFG1, 606, 2, 0, 4, 3, 4, 10 }, { GID_QFG1, 606, 2, 0, 4, 5, 4, 10 }, { GID_QFG1, 606, 2, 0, 7, 7, 0, 12 }, { GID_QFG1, 606, 2, 0, 7, 8, 1, 0 }, { GID_QFG1, 606, 2, 0, 8, 7, 0, 12 }, { GID_QFG1, 606, 2, 0, 8, 8, 1, 0 }, { GID_QFG1, 606, 2, 0, 9, 3, 12, 15 }, { GID_QFG1, 606, 2, 0, 9, 7, 0, 4 }, { GID_QFG1, 606, 2, 0, 9, 8, 1, 0 }, { GID_QFG1, 606, 2, 0, 10, 7, 0, 4 }, { GID_QFG1, 606, 2, 0, 10, 8, 1, 0 }, { GID_QFG1, 606, 2, 0, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 2, 1, 2, 6, 0, 12 }, { GID_QFG1, 606, 2, 1, 2, 7, 1, 0 }, { GID_QFG1, 606, 2, 1, 3, 4, 8, 12 }, { GID_QFG1, 606, 2, 1, 3, 6, 0, 12 }, { GID_QFG1, 606, 2, 1, 3, 7, 1, 0 }, { GID_QFG1, 606, 2, 1, 5, 3, 4, 10 }, { GID_QFG1, 606, 2, 1, 5, 5, 4, 10 }, { GID_QFG1, 606, 2, 1, 7, 7, 0, 12 }, { GID_QFG1, 606, 2, 1, 7, 8, 1, 0 }, { GID_QFG1, 606, 2, 1, 8, 7, 0, 12 }, { GID_QFG1, 606, 2, 1, 8, 8, 1, 0 }, { GID_QFG1, 606, 2, 1, 9, 7, 0, 4 }, { GID_QFG1, 606, 2, 1, 9, 8, 1, 0 }, { GID_QFG1, 606, 2, 1, 10, 7, 0, 4 }, { GID_QFG1, 606, 2, 1, 10, 8, 1, 0 }, { GID_QFG1, 606, 2, 1, 11, 3, 12, 15 }, { GID_QFG1, 606, 2, 1, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 2, 2, 2, 6, 0, 12 }, { GID_QFG1, 606, 2, 2, 2, 7, 1, 0 }, { GID_QFG1, 606, 2, 2, 3, 4, 8, 12 }, { GID_QFG1, 606, 2, 2, 3, 6, 0, 12 }, { GID_QFG1, 606, 2, 2, 3, 7, 1, 0 }, { GID_QFG1, 606, 2, 2, 5, 3, 4, 10 }, { GID_QFG1, 606, 2, 2, 5, 5, 4, 10 }, { GID_QFG1, 606, 2, 2, 7, 7, 0, 12 }, { GID_QFG1, 606, 2, 2, 7, 8, 1, 0 }, { GID_QFG1, 606, 2, 2, 8, 7, 0, 12 }, { GID_QFG1, 606, 2, 2, 8, 8, 1, 0 }, { GID_QFG1, 606, 2, 2, 9, 7, 0, 4 }, { GID_QFG1, 606, 2, 2, 9, 8, 1, 0 }, { GID_QFG1, 606, 2, 2, 10, 7, 0, 4 }, { GID_QFG1, 606, 2, 2, 10, 8, 1, 0 }, { GID_QFG1, 606, 2, 2, 11, 3, 12, 15 }, { GID_QFG1, 606, 2, 2, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 2, 3, 2, 6, 0, 12 }, { GID_QFG1, 606, 2, 3, 2, 7, 1, 0 }, { GID_QFG1, 606, 2, 3, 3, 4, 8, 12 }, { GID_QFG1, 606, 2, 3, 3, 6, 0, 12 }, { GID_QFG1, 606, 2, 3, 3, 7, 1, 0 }, { GID_QFG1, 606, 2, 3, 5, 3, 4, 10 }, { GID_QFG1, 606, 2, 3, 5, 5, 4, 10 }, { GID_QFG1, 606, 2, 3, 7, 7, 0, 12 }, { GID_QFG1, 606, 2, 3, 7, 8, 1, 0 }, { GID_QFG1, 606, 2, 3, 8, 7, 0, 12 }, { GID_QFG1, 606, 2, 3, 8, 8, 1, 0 }, { GID_QFG1, 606, 2, 3, 9, 7, 0, 4 }, { GID_QFG1, 606, 2, 3, 9, 8, 1, 0 }, { GID_QFG1, 606, 2, 3, 10, 7, 0, 4 }, { GID_QFG1, 606, 2, 3, 10, 8, 1, 0 }, { GID_QFG1, 606, 2, 3, 11, 3, 12, 15 }, { GID_QFG1, 606, 2, 3, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 2, 4, 2, 6, 0, 12 }, { GID_QFG1, 606, 2, 4, 2, 7, 1, 0 }, { GID_QFG1, 606, 2, 4, 3, 4, 8, 12 }, { GID_QFG1, 606, 2, 4, 3, 6, 0, 12 }, { GID_QFG1, 606, 2, 4, 3, 7, 1, 0 }, { GID_QFG1, 606, 2, 4, 4, 3, 4, 10 }, { GID_QFG1, 606, 2, 4, 4, 5, 4, 10 }, { GID_QFG1, 606, 2, 4, 7, 7, 0, 12 }, { GID_QFG1, 606, 2, 4, 7, 8, 1, 0 }, { GID_QFG1, 606, 2, 4, 8, 7, 0, 12 }, { GID_QFG1, 606, 2, 4, 8, 8, 1, 0 }, { GID_QFG1, 606, 2, 4, 9, 7, 0, 4 }, { GID_QFG1, 606, 2, 4, 9, 8, 1, 0 }, { GID_QFG1, 606, 2, 4, 10, 7, 0, 4 }, { GID_QFG1, 606, 2, 4, 10, 8, 1, 0 }, { GID_QFG1, 606, 2, 4, 11, 3, 12, 15 }, { GID_QFG1, 606, 2, 4, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 2, 5, 2, 6, 0, 12 }, { GID_QFG1, 606, 2, 5, 2, 7, 1, 0 }, { GID_QFG1, 606, 2, 5, 3, 4, 8, 12 }, { GID_QFG1, 606, 2, 5, 3, 6, 0, 12 }, { GID_QFG1, 606, 2, 5, 3, 7, 1, 0 }, { GID_QFG1, 606, 2, 5, 5, 3, 4, 10 }, { GID_QFG1, 606, 2, 5, 5, 5, 4, 10 }, { GID_QFG1, 606, 2, 5, 7, 7, 0, 12 }, { GID_QFG1, 606, 2, 5, 7, 8, 1, 0 }, { GID_QFG1, 606, 2, 5, 8, 7, 0, 12 }, { GID_QFG1, 606, 2, 5, 8, 8, 1, 0 }, { GID_QFG1, 606, 2, 5, 9, 7, 0, 4 }, { GID_QFG1, 606, 2, 5, 9, 8, 1, 0 }, { GID_QFG1, 606, 2, 5, 10, 7, 0, 4 }, { GID_QFG1, 606, 2, 5, 10, 8, 1, 0 }, { GID_QFG1, 606, 2, 5, 11, 3, 12, 15 }, { GID_QFG1, 606, 2, 5, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 2, 6, 2, 6, 0, 12 }, { GID_QFG1, 606, 2, 6, 2, 7, 1, 0 }, { GID_QFG1, 606, 2, 6, 3, 4, 8, 12 }, { GID_QFG1, 606, 2, 6, 3, 6, 0, 12 }, { GID_QFG1, 606, 2, 6, 3, 7, 1, 0 }, { GID_QFG1, 606, 2, 6, 5, 3, 4, 10 }, { GID_QFG1, 606, 2, 6, 5, 5, 4, 10 }, { GID_QFG1, 606, 2, 6, 7, 7, 0, 12 }, { GID_QFG1, 606, 2, 6, 7, 8, 1, 0 }, { GID_QFG1, 606, 2, 6, 8, 7, 0, 12 }, { GID_QFG1, 606, 2, 6, 8, 8, 1, 0 }, { GID_QFG1, 606, 2, 6, 9, 7, 0, 4 }, { GID_QFG1, 606, 2, 6, 9, 8, 1, 0 }, { GID_QFG1, 606, 2, 6, 10, 7, 0, 4 }, { GID_QFG1, 606, 2, 6, 10, 8, 1, 0 }, { GID_QFG1, 606, 2, 6, 11, 3, 12, 15 }, { GID_QFG1, 606, 2, 6, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 2, 7, 2, 6, 0, 12 }, { GID_QFG1, 606, 2, 7, 2, 7, 1, 0 }, { GID_QFG1, 606, 2, 7, 3, 4, 8, 12 }, { GID_QFG1, 606, 2, 7, 3, 6, 0, 12 }, { GID_QFG1, 606, 2, 7, 3, 7, 1, 0 }, { GID_QFG1, 606, 2, 7, 7, 7, 0, 12 }, { GID_QFG1, 606, 2, 7, 7, 8, 1, 0 }, { GID_QFG1, 606, 2, 7, 8, 7, 0, 12 }, { GID_QFG1, 606, 2, 7, 8, 8, 1, 0 }, { GID_QFG1, 606, 2, 7, 9, 7, 0, 4 }, { GID_QFG1, 606, 2, 7, 9, 8, 1, 0 }, { GID_QFG1, 606, 2, 7, 10, 7, 0, 4 }, { GID_QFG1, 606, 2, 7, 10, 8, 1, 0 }, { GID_QFG1, 606, 2, 7, 11, 3, 12, 15 }, { GID_QFG1, 606, 2, 7, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 2, 8, 2, 6, 0, 12 }, { GID_QFG1, 606, 2, 8, 2, 7, 1, 0 }, { GID_QFG1, 606, 2, 8, 3, 4, 8, 12 }, { GID_QFG1, 606, 2, 8, 3, 6, 0, 12 }, { GID_QFG1, 606, 2, 8, 3, 7, 1, 0 }, { GID_QFG1, 606, 2, 8, 5, 3, 4, 10 }, { GID_QFG1, 606, 2, 8, 5, 5, 4, 10 }, { GID_QFG1, 606, 2, 8, 7, 7, 0, 12 }, { GID_QFG1, 606, 2, 8, 7, 8, 1, 0 }, { GID_QFG1, 606, 2, 8, 8, 7, 0, 12 }, { GID_QFG1, 606, 2, 8, 8, 8, 1, 0 }, { GID_QFG1, 606, 2, 8, 9, 7, 0, 4 }, { GID_QFG1, 606, 2, 8, 9, 8, 1, 0 }, { GID_QFG1, 606, 2, 8, 10, 7, 0, 4 }, { GID_QFG1, 606, 2, 8, 10, 8, 1, 0 }, { GID_QFG1, 606, 2, 8, 11, 3, 12, 15 }, { GID_QFG1, 606, 2, 8, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 2, 9, 2, 6, 0, 12 }, { GID_QFG1, 606, 2, 9, 2, 7, 1, 0 }, { GID_QFG1, 606, 2, 9, 3, 4, 8, 12 }, { GID_QFG1, 606, 2, 9, 3, 6, 0, 12 }, { GID_QFG1, 606, 2, 9, 3, 7, 1, 0 }, { GID_QFG1, 606, 2, 9, 5, 3, 4, 10 }, { GID_QFG1, 606, 2, 9, 5, 5, 4, 10 }, { GID_QFG1, 606, 2, 9, 7, 7, 0, 12 }, { GID_QFG1, 606, 2, 9, 7, 8, 1, 0 }, { GID_QFG1, 606, 2, 9, 8, 7, 0, 12 }, { GID_QFG1, 606, 2, 9, 8, 8, 1, 0 }, { GID_QFG1, 606, 2, 9, 9, 7, 0, 4 }, { GID_QFG1, 606, 2, 9, 9, 8, 1, 0 }, { GID_QFG1, 606, 2, 9, 10, 7, 0, 4 }, { GID_QFG1, 606, 2, 9, 10, 8, 1, 0 }, { GID_QFG1, 606, 2, 9, 11, 3, 12, 15 }, { GID_QFG1, 606, 2, 9, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 2, 10, 2, 6, 0, 12 }, { GID_QFG1, 606, 2, 10, 2, 7, 1, 0 }, { GID_QFG1, 606, 2, 10, 3, 4, 8, 12 }, { GID_QFG1, 606, 2, 10, 3, 6, 0, 12 }, { GID_QFG1, 606, 2, 10, 3, 7, 1, 0 }, { GID_QFG1, 606, 2, 10, 5, 3, 4, 10 }, { GID_QFG1, 606, 2, 10, 5, 5, 4, 10 }, { GID_QFG1, 606, 2, 10, 7, 7, 0, 12 }, { GID_QFG1, 606, 2, 10, 7, 8, 1, 0 }, { GID_QFG1, 606, 2, 10, 8, 7, 0, 12 }, { GID_QFG1, 606, 2, 10, 8, 8, 1, 0 }, { GID_QFG1, 606, 2, 10, 9, 7, 0, 4 }, { GID_QFG1, 606, 2, 10, 9, 8, 1, 0 }, { GID_QFG1, 606, 2, 10, 10, 7, 0, 4 }, { GID_QFG1, 606, 2, 10, 10, 8, 1, 0 }, { GID_QFG1, 606, 2, 10, 11, 3, 12, 15 }, { GID_QFG1, 606, 2, 10, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 2, 11, 2, 6, 0, 12 }, { GID_QFG1, 606, 2, 11, 2, 7, 1, 0 }, { GID_QFG1, 606, 2, 11, 3, 4, 8, 12 }, { GID_QFG1, 606, 2, 11, 3, 6, 0, 12 }, { GID_QFG1, 606, 2, 11, 3, 7, 1, 0 }, { GID_QFG1, 606, 2, 11, 4, 3, 4, 10 }, { GID_QFG1, 606, 2, 11, 4, 5, 4, 10 }, { GID_QFG1, 606, 2, 11, 7, 7, 0, 12 }, { GID_QFG1, 606, 2, 11, 7, 8, 1, 0 }, { GID_QFG1, 606, 2, 11, 8, 7, 0, 12 }, { GID_QFG1, 606, 2, 11, 8, 8, 1, 0 }, { GID_QFG1, 606, 2, 11, 9, 3, 12, 15 }, { GID_QFG1, 606, 2, 11, 9, 7, 0, 4 }, { GID_QFG1, 606, 2, 11, 9, 8, 1, 0 }, { GID_QFG1, 606, 2, 11, 10, 7, 0, 4 }, { GID_QFG1, 606, 2, 11, 10, 8, 1, 0 }, { GID_QFG1, 606, 2, 11, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 3, 0, 2, 6, 0, 12 }, { GID_QFG1, 606, 3, 0, 2, 7, 1, 0 }, { GID_QFG1, 606, 3, 0, 3, 4, 8, 12 }, { GID_QFG1, 606, 3, 0, 3, 6, 0, 12 }, { GID_QFG1, 606, 3, 0, 3, 7, 1, 0 }, { GID_QFG1, 606, 3, 0, 4, 3, 4, 10 }, { GID_QFG1, 606, 3, 0, 4, 5, 4, 10 }, { GID_QFG1, 606, 3, 0, 7, 7, 0, 12 }, { GID_QFG1, 606, 3, 0, 7, 8, 1, 0 }, { GID_QFG1, 606, 3, 0, 8, 7, 0, 12 }, { GID_QFG1, 606, 3, 0, 8, 8, 1, 0 }, { GID_QFG1, 606, 3, 0, 9, 3, 12, 15 }, { GID_QFG1, 606, 3, 0, 9, 7, 0, 4 }, { GID_QFG1, 606, 3, 0, 9, 8, 1, 0 }, { GID_QFG1, 606, 3, 0, 10, 7, 0, 4 }, { GID_QFG1, 606, 3, 0, 10, 8, 1, 0 }, { GID_QFG1, 606, 3, 0, 11, 8, 1, 0 },
	{ GID_QFG1, 606, 3, 1, 2, 6, 0, 12 }, { GID_QFG1, 606, 3, 1, 2, 7, 1, 0 }, { GID_QFG1, 606, 3, 1, 3, 4, 8, 12 }, { GID_QFG1, 606, 3, 1, 3, 6, 0, 12 }, { GID_QFG1, 606, 3, 1, 3, 7, 1, 0 }, { GID_QFG1, 606, 3, 1, 5, 3, 4, 10 }, { GID_QFG1, 606, 3, 1, 5, 5, 4, 10 }, { GID_QFG1, 606, 3, 1, 7, 7, 0, 12 }, { GID_QFG1, 606, 3, 1, 7, 8, 1, 0 }, { GID_QFG1, 606, 3, 1, 8, 7, 0, 12 }, { GID_QFG1, 606, 3, 1, 8, 8, 1, 0 }, { GID_QFG1, 606, 3, 1, 9, 3, 12, 15 }, { GID_QFG1, 606, 3, 1, 9, 7, 0, 4 }, { GID_QFG1, 606, 3, 1, 9, 8, 1, 0 }, { GID_QFG1, 606, 3, 1, 10, 7, 0, 4 }, { GID_QFG1, 606, 3, 1, 10, 8, 1, 0 },
	{ GID_QFG1, 606, 3, 2, 3, 4, 8, 12 }, { GID_QFG1, 606, 3, 2, 3, 6, 8, 12 }, { GID_QFG1, 606, 3, 2, 5, 3, 4, 10 }, { GID_QFG1, 606, 3, 2, 5, 5, 12, 10 }, { GID_QFG1, 606, 3, 2, 9, 3, 12, 15 },
	{ GID_QFG1, 606, 3, 3, 3, 4, 8, 12 }, { GID_QFG1, 606, 3, 3, 3, 6, 8, 12 }, { GID_QFG1, 606, 3, 3, 5, 3, 4, 10 }, { GID_QFG1, 606, 3, 3, 5, 5, 12, 10 }, { GID_QFG1, 606, 3, 3, 9, 3, 12, 15 },
	{ GID_QFG1, 606, 3, 4, 3, 4, 8, 12 }, { GID_QFG1, 606, 3, 4, 3, 6, 8, 12 }, { GID_QFG1, 606, 3, 4, 5, 3, 4, 10 }, { GID_QFG1, 606, 3, 4, 5, 5, 12, 10 }, { GID_QFG1, 606, 3, 4, 9, 3, 12, 15 },
	{ GID_QFG1, 606, 3, 5, 2, 6, 0, 12 }, { GID_QFG1, 606, 3, 5, 2, 7, 1, 0 }, { GID_QFG1, 606, 3, 5, 3, 4, 8, 12 }, { GID_QFG1, 606, 3, 5, 3, 6, 0, 12 }, { GID_QFG1, 606, 3, 5, 3, 7, 1, 0 }, { GID_QFG1, 606, 3, 5, 5, 3, 4, 10 }, { GID_QFG1, 606, 3, 5, 5, 5, 4, 10 }, { GID_QFG1, 606, 3, 5, 7, 7, 0, 12 }, { GID_QFG1, 606, 3, 5, 7, 8, 1, 0 }, { GID_QFG1, 606, 3, 5, 8, 7, 0, 12 }, { GID_QFG1, 606, 3, 5, 8, 8, 1, 0 }, { GID_QFG1, 606, 3, 5, 9, 3, 12, 15 }, { GID_QFG1, 606, 3, 5, 9, 7, 0, 4 }, { GID_QFG1, 606, 3, 5, 9, 8, 1, 0 }, { GID_QFG1, 606, 3, 5, 10, 7, 0, 4 }, { GID_QFG1, 606, 3, 5, 10, 8, 1, 0 },
	{ GID_QFG1, 606, 3, 6, 2, 6, 0, 12 }, { GID_QFG1, 606, 3, 6, 2, 7, 1, 0 }, { GID_QFG1, 606, 3, 6, 3, 4, 8, 12 }, { GID_QFG1, 606, 3, 6, 3, 6, 0, 12 }, { GID_QFG1, 606, 3, 6, 3, 7, 1, 0 }, { GID_QFG1, 606, 3, 6, 5, 3, 4, 10 }, { GID_QFG1, 606, 3, 6, 5, 5, 4, 10 }, { GID_QFG1, 606, 3, 6, 7, 7, 0, 12 }, { GID_QFG1, 606, 3, 6, 7, 8, 1, 0 }, { GID_QFG1, 606, 3, 6, 8, 7, 0, 12 }, { GID_QFG1, 606, 3, 6, 8, 8, 1, 0 }, { GID_QFG1, 606, 3, 6, 9, 7, 0, 4 }, { GID_QFG1, 606, 3, 6, 9, 8, 1, 0 }, { GID_QFG1, 606, 3, 6, 10, 7, 0, 4 }, { GID_QFG1, 606, 3, 6, 10, 8, 1, 0 }, { GID_QFG1, 606, 3, 6, 11, 3, 12, 15 },
	{ GID_QFG1, 606, 3, 7, 2, 6, 0, 12 }, { GID_QFG1, 606, 3, 7, 2, 7, 1, 0 }, { GID_QFG1, 606, 3, 7, 3, 4, 8, 12 }, { GID_QFG1, 606, 3, 7, 3, 6, 0, 12 }, { GID_QFG1, 606, 3, 7, 3, 7, 1, 0 }, { GID_QFG1, 606, 3, 7, 5, 3, 4, 10 }, { GID_QFG1, 606, 3, 7, 5, 5, 4, 10 }, { GID_QFG1, 606, 3, 7, 7, 7, 0, 12 }, { GID_QFG1, 606, 3, 7, 7, 8, 1, 0 }, { GID_QFG1, 606, 3, 7, 8, 7, 0, 12 }, { GID_QFG1, 606, 3, 7, 8, 8, 1, 0 }, { GID_QFG1, 606, 3, 7, 9, 7, 0, 4 }, { GID_QFG1, 606, 3, 7, 9, 8, 1, 0 }, { GID_QFG1, 606, 3, 7, 10, 7, 0, 4 }, { GID_QFG1, 606, 3, 7, 10, 8, 1, 0 }, { GID_QFG1, 606, 3, 7, 11, 3, 12, 15 },
	{ GID_QFG1, 606, 3, 8, 2, 3, 8, 12 }, { GID_QFG1, 606, 3, 8, 2, 4, 8, 12 }, { GID_QFG1, 606, 3, 8, 2, 5, 8, 12 }, { GID_QFG1, 606, 3, 8, 2, 6, 0, 12 }, { GID_QFG1, 606, 3, 8, 2, 7, 1, 0 }, { GID_QFG1, 606, 3, 8, 3, 3, 12, 8 }, { GID_QFG1, 606, 3, 8, 3, 5, 12, 8 }, { GID_QFG1, 606, 3, 8, 3, 6, 0, 12 }, { GID_QFG1, 606, 3, 8, 3, 7, 1, 0 }, { GID_QFG1, 606, 3, 8, 4, 3, 4, 10 }, { GID_QFG1, 606, 3, 8, 4, 5, 4, 10 }, { GID_QFG1, 606, 3, 8, 7, 7, 0, 12 }, { GID_QFG1, 606, 3, 8, 7, 8, 1, 0 }, { GID_QFG1, 606, 3, 8, 8, 7, 0, 12 }, { GID_QFG1, 606, 3, 8, 8, 8, 1, 0 }, { GID_QFG1, 606, 3, 8, 9, 7, 0, 4 }, { GID_QFG1, 606, 3, 8, 9, 8, 1, 0 }, { GID_QFG1, 606, 3, 8, 10, 7, 0, 4 }, { GID_QFG1, 606, 3, 8, 10, 8, 1, 0 }, { GID_QFG1, 606, 3, 8, 11, 3, 12, 15 },
	{ GID_QFG1, 606, 3, 9, 2, 6, 0, 12 }, { GID_QFG1, 606, 3, 9, 2, 7, 1, 0 }, { GID_QFG1, 606, 3, 9, 3, 4, 8, 12 }, { GID_QFG1, 606, 3, 9, 3, 6, 0, 12 }, { GID_QFG1, 606, 3, 9, 3, 7, 1, 0 }, { GID_QFG1, 606, 3, 9, 4, 3, 4, 10 }, { GID_QFG1, 606, 3, 9, 4, 5, 4, 10 }, { GID_QFG1, 606, 3, 9, 7, 7, 0, 12 }, { GID_QFG1, 606, 3, 9, 7, 8, 1, 0 }, { GID_QFG1, 606, 3, 9, 8, 7, 0, 12 }, { GID_QFG1, 606, 3, 9, 8, 8, 1, 0 }, { GID_QFG1, 606, 3, 9, 9, 7, 0, 4 }, { GID_QFG1, 606, 3, 9, 9, 8, 1, 0 }, { GID_QFG1, 606, 3, 9, 10, 7, 0, 4 }, { GID_QFG1, 606, 3, 9, 10, 8, 1, 0 }, { GID_QFG1, 606, 3, 9, 11, 3, 12, 15 },
	{ GID_QFG1, 606, 3, 10, 2, 3, 8, 12 }, { GID_QFG1, 606, 3, 10, 2, 4, 8, 12 }, { GID_QFG1, 606, 3, 10, 2, 5, 8, 12 }, { GID_QFG1, 606, 3, 10, 2, 6, 0, 12 }, { GID_QFG1, 606, 3, 10, 2, 7, 1, 0 }, { GID_QFG1, 606, 3, 10, 3, 3, 12, 8 }, { GID_QFG1, 606, 3, 10, 3, 5, 12, 8 }, { GID_QFG1, 606, 3, 10, 3, 6, 0, 12 }, { GID_QFG1, 606, 3, 10, 3, 7, 1, 0 }, { GID_QFG1, 606, 3, 10, 4, 3, 4, 10 }, { GID_QFG1, 606, 3, 10, 4, 5, 4, 10 }, { GID_QFG1, 606, 3, 10, 7, 7, 0, 12 }, { GID_QFG1, 606, 3, 10, 7, 8, 1, 0 }, { GID_QFG1, 606, 3, 10, 8, 7, 0, 12 }, { GID_QFG1, 606, 3, 10, 8, 8, 1, 0 }, { GID_QFG1, 606, 3, 10, 9, 7, 0, 4 }, { GID_QFG1, 606, 3, 10, 9, 8, 1, 0 }, { GID_QFG1, 606, 3, 10, 10, 7, 0, 4 }, { GID_QFG1, 606, 3, 10, 10, 8, 1, 0 }, { GID_QFG1, 606, 3, 10, 11, 3, 12, 15 },
	{ GID_QFG1, 606, 3, 11, 2, 6, 0, 12 }, { GID_QFG1, 606, 3, 11, 2, 7, 1, 0 }, { GID_QFG1, 606, 3, 11, 3, 4, 8, 12 }, { GID_QFG1, 606, 3, 11, 3, 6, 0, 12 }, { GID_QFG1, 606, 3, 11, 3, 7, 1, 0 }, { GID_QFG1, 606, 3, 11, 4, 3, 4, 10 }, { GID_QFG1, 606, 3, 11, 4, 5, 4, 10 }, { GID_QFG1, 606, 3, 11, 7, 7, 0, 12 }, { GID_QFG1, 606, 3, 11, 7, 8, 1, 0 }, { GID_QFG1, 606, 3, 11, 8, 7, 0, 12 }, { GID_QFG1, 606, 3, 11, 8, 8, 1, 0 }, { GID_QFG1, 606, 3, 11, 9, 7, 0, 4 }, { GID_QFG1, 606, 3, 11, 9, 8, 1, 0 }, { GID_QFG1, 606, 3, 11, 10, 7, 0, 4 }, { GID_QFG1, 606, 3, 11, 10, 8, 1, 0 }, { GID_QFG1, 606, 3, 11, 11, 3, 12, 15 },
	{ GID_QFG1, 606, 3, 12, 2, 3, 8, 12 }, { GID_QFG1, 606, 3, 12, 2, 4, 8, 12 }, { GID_QFG1, 606, 3, 12, 2, 5, 8, 12 }, { GID_QFG1, 606, 3, 12, 2, 6, 0, 12 }, { GID_QFG1, 606, 3, 12, 2, 7, 1, 0 }, { GID_QFG1, 606, 3, 12, 3, 3, 12, 8 }, { GID_QFG1, 606, 3, 12, 3, 5, 12, 8 }, { GID_QFG1, 606, 3, 12, 3, 6, 0, 12 }, { GID_QFG1, 606, 3, 12, 3, 7, 1, 0 }, { GID_QFG1, 606, 3, 12, 4, 3, 4, 10 }, { GID_QFG1, 606, 3, 12, 4, 5, 4, 10 }, { GID_QFG1, 606, 3, 12, 7, 7, 0, 12 }, { GID_QFG1, 606, 3, 12, 7, 8, 1, 0 }, { GID_QFG1, 606, 3, 12, 8, 7, 0, 12 }, { GID_QFG1, 606, 3, 12, 8, 8, 1, 0 }, { GID_QFG1, 606, 3, 12, 9, 7, 0, 4 }, { GID_QFG1, 606, 3, 12, 9, 8, 1, 0 }, { GID_QFG1, 606, 3, 12, 10, 7, 0, 4 }, { GID_QFG1, 606, 3, 12, 10, 8, 1, 0 }, { GID_QFG1, 606, 3, 12, 11, 3, 12, 15 },
	{ GID_QFG1, 606, 3, 13, 2, 6, 0, 12 }, { GID_QFG1, 606, 3, 13, 2, 7, 1, 0 }, { GID_QFG1, 606, 3, 13, 3, 4, 8, 12 }, { GID_QFG1, 606, 3, 13, 3, 6, 0, 12 }, { GID_QFG1, 606, 3, 13, 3, 7, 1, 0 }, { GID_QFG1, 606, 3, 13, 4, 3, 4, 10 }, { GID_QFG1, 606, 3, 13, 4, 5, 4, 10 }, { GID_QFG1, 606, 3, 13, 7, 7, 0, 12 }, { GID_QFG1, 606, 3, 13, 7, 8, 1, 0 }, { GID_QFG1, 606, 3, 13, 8, 7, 0, 12 }, { GID_QFG1, 606, 3, 13, 8, 8, 1, 0 }, { GID_QFG1, 606, 3, 13, 9, 3, 12, 15 }, { GID_QFG1, 606, 3, 13, 9, 7, 0, 4 }, { GID_QFG1, 606, 3, 13, 9, 8, 1, 0 }, { GID_QFG1, 606, 3, 13, 10, 7, 0, 4 }, { GID_QFG1, 606, 3, 13, 10, 8, 1, 0 },
	{ GID_QFG1, 606, 3, 14, 3, 4, 8, 12 }, { GID_QFG1, 606, 3, 14, 3, 6, 8, 12 }, { GID_QFG1, 606, 3, 14, 5, 3, 4, 10 }, { GID_QFG1, 606, 3, 14, 5, 5, 12, 10 }, { GID_QFG1, 606, 3, 14, 9, 3, 12, 15 },
	{ GID_QFG1, 606, 3, 15, 2, 6, 0, 12 }, { GID_QFG1, 606, 3, 15, 2, 7, 1, 0 }, { GID_QFG1, 606, 3, 15, 3, 4, 8, 12 }, { GID_QFG1, 606, 3, 15, 3, 6, 0, 12 }, { GID_QFG1, 606, 3, 15, 3, 7, 1, 0 }, { GID_QFG1, 606, 3, 15, 4, 3, 4, 10 }, { GID_QFG1, 606, 3, 15, 4, 5, 4, 10 }, { GID_QFG1, 606, 3, 15, 7, 7, 0, 12 }, { GID_QFG1, 606, 3, 15, 7, 8, 1, 0 }, { GID_QFG1, 606, 3, 15, 8, 7, 0, 12 }, { GID_QFG1, 606, 3, 15, 8, 8, 1, 0 }, { GID_QFG1, 606, 3, 15, 9, 3, 12, 15 }, { GID_QFG1, 606, 3, 15, 9, 7, 0, 4 }, { GID_QFG1, 606, 3, 15, 9, 8, 1, 0 }, { GID_QFG1, 606, 3, 15, 10, 7, 0, 4 }, { GID_QFG1, 606, 3, 15, 10, 8, 1, 0 }, { GID_QFG1, 606, 3, 15, 11, 8, 1, 0 },
	// QfG1 Otto (view 602): his green eyes, which the collapse loses in his dark red face, with light red
	// skin between them, grey brows only above each eye, and his white tooth back in at Sierra's row
	{ GID_QFG1, 602, 0, 0, 4, 5, 8, 12 }, { GID_QFG1, 602, 0, 0, 5, 4, 4, 10 }, { GID_QFG1, 602, 0, 0, 5, 6, 4, 10 }, { GID_QFG1, 602, 0, 0, 12, 4, 12, 15 },
	{ GID_QFG1, 602, 0, 1, 2, 5, 8, 12 }, { GID_QFG1, 602, 0, 1, 4, 4, 4, 10 }, { GID_QFG1, 602, 0, 1, 4, 6, 4, 10 }, { GID_QFG1, 602, 0, 1, 11, 4, 4, 15 },
	{ GID_QFG1, 602, 0, 2, 4, 5, 8, 12 }, { GID_QFG1, 602, 0, 2, 5, 4, 4, 10 }, { GID_QFG1, 602, 0, 2, 5, 6, 4, 10 }, { GID_QFG1, 602, 0, 2, 12, 4, 12, 15 },
	{ GID_QFG1, 602, 0, 3, 3, 6, 8, 12 }, { GID_QFG1, 602, 0, 3, 5, 5, 4, 10 }, { GID_QFG1, 602, 0, 3, 5, 7, 4, 10 }, { GID_QFG1, 602, 0, 3, 12, 5, 4, 15 },
	{ GID_QFG1, 602, 0, 4, 3, 5, 8, 12 }, { GID_QFG1, 602, 0, 4, 4, 4, 4, 10 }, { GID_QFG1, 602, 0, 4, 4, 6, 4, 10 }, { GID_QFG1, 602, 0, 4, 11, 4, 12, 15 },
	{ GID_QFG1, 602, 0, 5, 3, 5, 8, 12 }, { GID_QFG1, 602, 0, 5, 5, 4, 4, 10 }, { GID_QFG1, 602, 0, 5, 5, 6, 4, 10 }, { GID_QFG1, 602, 0, 5, 10, 4, 12, 15 },
	{ GID_QFG1, 602, 1, 0, 3, 5, 8, 12 }, { GID_QFG1, 602, 1, 0, 5, 5, 8, 12 }, { GID_QFG1, 602, 1, 0, 6, 4, 4, 2 }, { GID_QFG1, 602, 1, 0, 6, 5, 2, 12 }, { GID_QFG1, 602, 1, 0, 6, 6, 4, 2 }, { GID_QFG1, 602, 1, 0, 10, 4, 12, 15 },
	{ GID_QFG1, 602, 1, 1, 2, 4, 8, 12 }, { GID_QFG1, 602, 1, 1, 4, 4, 8, 12 }, { GID_QFG1, 602, 1, 1, 5, 3, 4, 2 }, { GID_QFG1, 602, 1, 1, 5, 4, 2, 12 }, { GID_QFG1, 602, 1, 1, 5, 5, 4, 2 }, { GID_QFG1, 602, 1, 1, 9, 3, 12, 15 },
	{ GID_QFG1, 602, 1, 2, 3, 5, 8, 12 }, { GID_QFG1, 602, 1, 2, 5, 5, 8, 12 }, { GID_QFG1, 602, 1, 2, 6, 4, 4, 2 }, { GID_QFG1, 602, 1, 2, 6, 5, 2, 12 }, { GID_QFG1, 602, 1, 2, 6, 6, 4, 2 }, { GID_QFG1, 602, 1, 2, 10, 4, 12, 15 },
	{ GID_QFG1, 602, 1, 3, 3, 3, 1, 12 }, { GID_QFG1, 602, 1, 3, 3, 5, 8, 12 }, { GID_QFG1, 602, 1, 3, 3, 7, 12, 1 }, { GID_QFG1, 602, 1, 3, 4, 3, 1, 12 }, { GID_QFG1, 602, 1, 3, 4, 7, 12, 1 }, { GID_QFG1, 602, 1, 3, 5, 5, 8, 12 }, { GID_QFG1, 602, 1, 3, 6, 5, 4, 12 }, { GID_QFG1, 602, 1, 3, 10, 4, 12, 15 },
	{ GID_QFG1, 602, 1, 4, 2, 5, 8, 12 }, { GID_QFG1, 602, 1, 4, 4, 5, 8, 12 }, { GID_QFG1, 602, 1, 4, 5, 4, 4, 2 }, { GID_QFG1, 602, 1, 4, 5, 5, 2, 12 }, { GID_QFG1, 602, 1, 4, 5, 6, 4, 2 }, { GID_QFG1, 602, 1, 4, 9, 4, 12, 15 },
	{ GID_QFG1, 602, 1, 5, 3, 5, 8, 12 }, { GID_QFG1, 602, 1, 5, 5, 5, 8, 12 }, { GID_QFG1, 602, 1, 5, 6, 4, 4, 2 }, { GID_QFG1, 602, 1, 5, 6, 5, 2, 12 }, { GID_QFG1, 602, 1, 5, 6, 6, 4, 2 }, { GID_QFG1, 602, 1, 5, 10, 4, 12, 15 },
	{ GID_QFG1, 602, 4, 0, 5, 10, 4, 10 }, { GID_QFG1, 602, 4, 0, 5, 12, 4, 10 }, { GID_QFG1, 602, 4, 0, 3, 11, 8, 12 },
	{ GID_QFG1, 602, 5, 0, 3, 5, 8, 12 }, { GID_QFG1, 602, 5, 0, 6, 4, 4, 2 }, { GID_QFG1, 602, 5, 0, 6, 5, 2, 12 }, { GID_QFG1, 602, 5, 0, 6, 6, 4, 2 },
	{ GID_QFG1, 602, 5, 1, 3, 5, 8, 12 }, { GID_QFG1, 602, 5, 1, 6, 4, 4, 2 }, { GID_QFG1, 602, 5, 1, 6, 5, 2, 12 }, { GID_QFG1, 602, 5, 1, 6, 6, 4, 2 },
	// QfG1 painting in the Sheriff's house (view 321 loop 4 cel 0): its brown frame down both sides
	{ GID_QFG1, 321, 4, 0, 2, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 3, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 4, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 5, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 6, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 7, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 8, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 9, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 9, 12, 4, 6 }, { GID_QFG1, 321, 4, 0, 10, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 10, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 11, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 11, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 12, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 12, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 13, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 13, 12, 1, 6 }, { GID_QFG1, 321, 4, 0, 14, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 14, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 15, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 15, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 16, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 16, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 17, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 17, 12, 1, 6 }, { GID_QFG1, 321, 4, 0, 18, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 18, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 19, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 19, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 20, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 20, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 21, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 21, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 22, 0, 8, 6 }, { GID_QFG1, 321, 4, 0, 22, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 23, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 24, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 25, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 26, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 27, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 28, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 29, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 30, 12, 9, 6 }, { GID_QFG1, 321, 4, 0, 31, 12, 8, 6 },
	// QfG1 man in the turban at the inn (view 302 loops 0 and 1): face a fat pixel wider each side at
	// his brows and eyes (grey brows, cyan eyes apart with skin round them), Sierra's width below, and
	// on loop 0 cel 0 his mouth shown as on loop 1
	{ GID_QFG1, 302, 0, 0, 5, 7, 8, 12 }, { GID_QFG1, 302, 0, 0, 5, 8, 8, 12 }, { GID_QFG1, 302, 0, 0, 6, 5, 9, 12 }, { GID_QFG1, 302, 0, 0, 6, 6, 0, 3 }, { GID_QFG1, 302, 0, 0, 6, 7, 0, 12 }, { GID_QFG1, 302, 0, 0, 6, 8, 0, 12 }, { GID_QFG1, 302, 0, 0, 6, 9, 0, 3 }, { GID_QFG1, 302, 0, 0, 6, 10, 9, 12 }, { GID_QFG1, 302, 0, 0, 7, 6, 0, 12 }, { GID_QFG1, 302, 0, 0, 7, 9, 0, 12 }, { GID_QFG1, 302, 0, 0, 9, 7, 0, 4 }, { GID_QFG1, 302, 0, 0, 9, 8, 0, 4 },
	{ GID_QFG1, 302, 0, 1, 5, 8, 8, 12 }, { GID_QFG1, 302, 0, 1, 5, 9, 8, 12 }, { GID_QFG1, 302, 0, 1, 6, 6, 9, 12 }, { GID_QFG1, 302, 0, 1, 6, 7, 0, 3 }, { GID_QFG1, 302, 0, 1, 6, 8, 0, 12 }, { GID_QFG1, 302, 0, 1, 6, 9, 0, 12 }, { GID_QFG1, 302, 0, 1, 6, 10, 0, 3 }, { GID_QFG1, 302, 0, 1, 6, 11, 9, 12 }, { GID_QFG1, 302, 0, 1, 7, 7, 0, 12 }, { GID_QFG1, 302, 0, 1, 7, 10, 0, 12 },
	{ GID_QFG1, 302, 0, 3, 5, 9, 8, 12 }, { GID_QFG1, 302, 0, 3, 6, 6, 9, 12 }, { GID_QFG1, 302, 0, 3, 6, 7, 0, 3 }, { GID_QFG1, 302, 0, 3, 6, 8, 0, 12 }, { GID_QFG1, 302, 0, 3, 6, 9, 0, 12 }, { GID_QFG1, 302, 0, 3, 6, 10, 0, 3 }, { GID_QFG1, 302, 0, 3, 6, 11, 9, 12 }, { GID_QFG1, 302, 0, 3, 7, 7, 0, 12 }, { GID_QFG1, 302, 0, 3, 7, 10, 0, 12 },
	{ GID_QFG1, 302, 1, 2, 5, 4, 8, 12 }, { GID_QFG1, 302, 1, 2, 5, 5, 8, 12 }, { GID_QFG1, 302, 1, 2, 6, 2, 9, 12 }, { GID_QFG1, 302, 1, 2, 6, 3, 0, 3 }, { GID_QFG1, 302, 1, 2, 6, 4, 0, 12 }, { GID_QFG1, 302, 1, 2, 6, 5, 0, 12 }, { GID_QFG1, 302, 1, 2, 6, 6, 0, 3 }, { GID_QFG1, 302, 1, 2, 6, 7, 9, 12 }, { GID_QFG1, 302, 1, 2, 7, 3, 0, 12 }, { GID_QFG1, 302, 1, 2, 7, 6, 0, 12 },
	{ GID_QFG1, 302, 1, 3, 5, 4, 8, 12 }, { GID_QFG1, 302, 1, 3, 5, 5, 8, 12 }, { GID_QFG1, 302, 1, 3, 6, 2, 9, 12 }, { GID_QFG1, 302, 1, 3, 6, 3, 0, 3 }, { GID_QFG1, 302, 1, 3, 6, 4, 0, 12 }, { GID_QFG1, 302, 1, 3, 6, 5, 0, 12 }, { GID_QFG1, 302, 1, 3, 6, 6, 0, 3 }, { GID_QFG1, 302, 1, 3, 6, 7, 9, 12 }, { GID_QFG1, 302, 1, 3, 7, 3, 0, 12 }, { GID_QFG1, 302, 1, 3, 7, 6, 0, 12 },
	{ GID_QFG1, 302, 1, 4, 5, 4, 8, 12 }, { GID_QFG1, 302, 1, 4, 5, 5, 8, 12 }, { GID_QFG1, 302, 1, 4, 6, 2, 9, 12 }, { GID_QFG1, 302, 1, 4, 6, 3, 0, 3 }, { GID_QFG1, 302, 1, 4, 6, 4, 0, 12 }, { GID_QFG1, 302, 1, 4, 6, 5, 0, 12 }, { GID_QFG1, 302, 1, 4, 6, 6, 0, 3 }, { GID_QFG1, 302, 1, 4, 6, 7, 9, 12 }, { GID_QFG1, 302, 1, 4, 7, 3, 0, 12 }, { GID_QFG1, 302, 1, 4, 7, 6, 0, 12 },
	// QfG1 sign appearing in room 28 (view 28 loop 0): white rim down both sides, where the finished
	// sign has it
	{ GID_QFG1, 28, 0, 1, 1, 2, 12, 15 }, { GID_QFG1, 28, 0, 1, 3, 1, 12, 15 }, { GID_QFG1, 28, 0, 1, 4, 0, 12, 15 },
	{ GID_QFG1, 28, 0, 2, 1, 2, 12, 15 }, { GID_QFG1, 28, 0, 2, 3, 1, 12, 15 }, { GID_QFG1, 28, 0, 2, 4, 0, 12, 15 }, { GID_QFG1, 28, 0, 2, 5, 0, 12, 15 }, { GID_QFG1, 28, 0, 2, 6, 0, 12, 15 }, { GID_QFG1, 28, 0, 2, 7, 0, 12, 15 }, { GID_QFG1, 28, 0, 2, 8, 0, 12, 15 },
	{ GID_QFG1, 28, 0, 3, 1, 2, 12, 15 }, { GID_QFG1, 28, 0, 3, 3, 1, 12, 15 }, { GID_QFG1, 28, 0, 3, 4, 0, 12, 15 }, { GID_QFG1, 28, 0, 3, 5, 0, 12, 15 }, { GID_QFG1, 28, 0, 3, 6, 0, 12, 15 }, { GID_QFG1, 28, 0, 3, 7, 0, 12, 15 }, { GID_QFG1, 28, 0, 3, 8, 0, 12, 15 }, { GID_QFG1, 28, 0, 3, 9, 0, 4, 15 }, { GID_QFG1, 28, 0, 3, 10, 0, 12, 15 }, { GID_QFG1, 28, 0, 3, 11, 0, 12, 15 }, { GID_QFG1, 28, 0, 3, 12, 0, 12, 15 }, { GID_QFG1, 28, 0, 3, 13, 0, 12, 15 }, { GID_QFG1, 28, 0, 3, 14, 0, 12, 15 }, { GID_QFG1, 28, 0, 3, 15, 0, 12, 15 }, { GID_QFG1, 28, 0, 3, 16, 0, 12, 15 },
	{ GID_QFG1, 28, 0, 4, 1, 2, 12, 15 }, { GID_QFG1, 28, 0, 4, 3, 1, 12, 15 }, { GID_QFG1, 28, 0, 4, 4, 0, 12, 15 }, { GID_QFG1, 28, 0, 4, 5, 0, 12, 15 }, { GID_QFG1, 28, 0, 4, 6, 0, 12, 15 }, { GID_QFG1, 28, 0, 4, 7, 0, 12, 15 }, { GID_QFG1, 28, 0, 4, 8, 0, 12, 15 }, { GID_QFG1, 28, 0, 4, 9, 0, 4, 15 }, { GID_QFG1, 28, 0, 4, 10, 0, 12, 15 }, { GID_QFG1, 28, 0, 4, 11, 0, 12, 15 }, { GID_QFG1, 28, 0, 4, 12, 0, 12, 15 }, { GID_QFG1, 28, 0, 4, 13, 0, 12, 15 }, { GID_QFG1, 28, 0, 4, 14, 0, 12, 15 }, { GID_QFG1, 28, 0, 4, 15, 0, 12, 15 }, { GID_QFG1, 28, 0, 4, 16, 0, 12, 15 }, { GID_QFG1, 28, 0, 4, 17, 0, 12, 15 }, { GID_QFG1, 28, 0, 4, 19, 1, 12, 15 },
	// QfG1 hero with the pitchfork (view 140 loop 3 cels 4 and 5): the middle of his joined eyes is skin
	{ GID_QFG1, 140, 3, 4, 5, 6, 0, 12 }, { GID_QFG1, 140, 3, 5, 5, 6, 0, 12 },
	// QfG1 castle door guards (view 41): the middle of their joined eyes is made skin (and on loop 1
	// cel 0, his right eye a fat pixel nearer the left)
	{ GID_QFG1, 41, 1, 0, 5, 4, 0, 12 }, { GID_QFG1, 41, 1, 0, 5, 6, 0, 12 },
	{ GID_QFG1, 41, 1, 1, 4, 4, 0, 12 }, { GID_QFG1, 41, 1, 1, 5, 5, 0, 12 },
	{ GID_QFG1, 41, 0, 1, 4, 10, 0, 12 }, { GID_QFG1, 41, 0, 1, 5, 9, 0, 12 },
	// QfG1 gate guard (view 37 loop 4): his eyes apart with skin between, his mustache centred and his
	// helmet symmetrical (left side copied from the right). Both frames get the same head
	{ GID_QFG1, 37, 4, 0, 0, 4, 2, 15 }, { GID_QFG1, 37, 4, 0, 1, 4, 8, 7 }, { GID_QFG1, 37, 4, 0, 2, 3, 8, 15 }, { GID_QFG1, 37, 4, 0, 2, 4, 8, 7 }, { GID_QFG1, 37, 4, 0, 3, 3, 8, 15 }, { GID_QFG1, 37, 4, 0, 4, 3, 8, 7 }, { GID_QFG1, 37, 4, 0, 5, 5, 4, 12 }, { GID_QFG1, 37, 4, 0, 7, 4, 4, 14 }, { GID_QFG1, 37, 4, 0, 7, 6, 4, 14 }, { GID_QFG1, 37, 4, 0, 7, 7, 14, 4 },
	{ GID_QFG1, 37, 4, 1, 0, 6, 2, 15 }, { GID_QFG1, 37, 4, 1, 1, 3, 8, 2 }, { GID_QFG1, 37, 4, 1, 1, 6, 15, 7 }, { GID_QFG1, 37, 4, 1, 2, 3, 8, 15 }, { GID_QFG1, 37, 4, 1, 2, 7, 2, 15 }, { GID_QFG1, 37, 4, 1, 3, 3, 8, 15 }, { GID_QFG1, 37, 4, 1, 3, 7, 2, 15 }, { GID_QFG1, 37, 4, 1, 4, 3, 8, 7 }, { GID_QFG1, 37, 4, 1, 4, 7, 2, 7 }, { GID_QFG1, 37, 4, 1, 5, 3, 0, 7 }, { GID_QFG1, 37, 4, 1, 5, 5, 0, 12 }, { GID_QFG1, 37, 4, 1, 5, 7, 2, 7 }, { GID_QFG1, 37, 4, 1, 6, 7, 2, 14 }, { GID_QFG1, 37, 4, 1, 7, 5, 4, 14 }, { GID_QFG1, 37, 4, 1, 7, 6, 4, 14 }, { GID_QFG1, 37, 4, 1, 7, 7, 2, 4 }, { GID_QFG1, 37, 4, 1, 8, 3, 4, 14 },
	// QfG1 Baron on his throne (view 141 loop 2, every cel): his eyes collapse into one block, so the
	// right eye moves a fat pixel right and skin goes between them
	{ GID_QFG1, 141, 2, -1, 6, 6, 0, 4 }, { GID_QFG1, 141, 2, -1, 6, 7, 4, 0 },
	// QfG1 "DRY GOODS" above the store (view 320 loop 2 cel 0): Sierra's letters, with the second O
	// made a copy of the first (it read as a B). The first O is a pixel wider, so the copy covers the
	// left edge of the D
	{ GID_QFG1, 320, 2, 0, 0, 29, 0xFF, 9 }, { GID_QFG1, 320, 2, 0, 0, 33, 0xFF, 9 }, { GID_QFG1, 320, 2, 0, 1, 30, 0xFF, 4 }, { GID_QFG1, 320, 2, 0, 1, 32, 0xFF, 12 }, { GID_QFG1, 320, 2, 0, 2, 30, 0xFF, 12 }, { GID_QFG1, 320, 2, 0, 2, 33, 0xFF, 12 }, { GID_QFG1, 320, 2, 0, 3, 30, 0xFF, 12 }, { GID_QFG1, 320, 2, 0, 3, 33, 0xFF, 12 }, { GID_QFG1, 320, 2, 0, 4, 30, 0xFF, 12 }, { GID_QFG1, 320, 2, 0, 4, 31, 0xFF, 4 }, { GID_QFG1, 320, 2, 0, 4, 33, 0xFF, 12 }, { GID_QFG1, 320, 2, 0, 5, 30, 0xFF, 12 }, { GID_QFG1, 320, 2, 0, 5, 33, 0xFF, 12 }, { GID_QFG1, 320, 2, 0, 6, 30, 0xFF, 12 }, { GID_QFG1, 320, 2, 0, 6, 33, 0xFF, 12 }, { GID_QFG1, 320, 2, 0, 7, 30, 0xFF, 12 }, { GID_QFG1, 320, 2, 0, 7, 33, 0xFF, 12 }, { GID_QFG1, 320, 2, 0, 8, 29, 0xFF, 9 }, { GID_QFG1, 320, 2, 0, 8, 30, 0xFF, 4 }, { GID_QFG1, 320, 2, 0, 8, 33, 0xFF, 9 },
	// QfG1 "MART" sign: red in the gap of its top and bottom border
	{ GID_QFG1, 320, 3, 4, 1, 14, 0xFF, 4 }, { GID_QFG1, 320, 3, 4, 9, 14, 0xFF, 4 },
	// QfG1 "INN" sign: blue instead of red left of the I, and red in the cyan edge opposite the pink one
	{ GID_QFG1, 300, 2, 0, 12, 1, 0xFF, 1 }, { GID_QFG1, 300, 2, 0, 13, 1, 0xFF, 1 }, { GID_QFG1, 300, 2, 0, 14, 1, 0xFF, 1 }, { GID_QFG1, 300, 2, 0, 15, 1, 0xFF, 1 }, { GID_QFG1, 300, 2, 0, 16, 1, 0xFF, 1 }, { GID_QFG1, 300, 2, 0, 17, 1, 0xFF, 1 }, { GID_QFG1, 300, 2, 0, 18, 1, 0xFF, 1 }, { GID_QFG1, 300, 2, 0, 14, 0, 0xFF, 4 }, { GID_QFG1, 300, 2, 0, 15, 0, 0xFF, 4 }, { GID_QFG1, 300, 2, 0, 16, 0, 0xFF, 4 },
	// QfG1 Sheriff's door board: its rosettes and green trim go, to make room for "Sheriff"
	{ GID_QFG1, 300, 2, 2, 5, 3, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 5, 21, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 6, 2, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 6, 3, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 6, 21, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 6, 22, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 7, 2, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 7, 3, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 7, 21, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 7, 22, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 8, 2, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 8, 3, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 8, 21, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 8, 22, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 9, 3, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 9, 10, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 9, 11, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 9, 12, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 9, 13, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 9, 21, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 10, 4, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 10, 5, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 10, 6, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 10, 7, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 10, 8, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 10, 9, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 10, 15, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 10, 16, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 10, 17, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 10, 18, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 10, 19, 0xFF, 4 }, { GID_QFG1, 300, 2, 2, 10, 20, 0xFF, 4 },
	// QfG1 hero sitting down (view 301 loop 6 cel 4): the two dark grey pixels under his chin, one
	// cleared and one skin
	{ GID_QFG1, 301, 6, 4, 14, 4, 8, 2 }, { GID_QFG1, 301, 6, 4, 14, 5, 8, 12 },
	// QfG1 "Spielburg" sign (view 165): letter pixels the collapse leaves pink or brown, made black.
	// P: middle bar, and the right side of its bowl
	{ GID_QFG1, 165, 0, 0, 19, 9, 12, 0 },
	{ GID_QFG1, 165, 0, 0, 16, 10, 6, 0 },
	{ GID_QFG1, 165, 0, 0, 17, 10, 6, 0 },
	// I, E, U: top pixel
	{ GID_QFG1, 165, 0, 0, 13, 12, 12, 0 },
	{ GID_QFG1, 165, 0, 0, 12, 15, 12, 0 },
	{ GID_QFG1, 165, 0, 0, 6, 27, 12, 0 },
	// R: top right, and the bottom of its bowl (right side, then the pixel left of it that closes it)
	{ GID_QFG1, 165, 0, 0, 5, 31, 12, 0 },
	{ GID_QFG1, 165, 0, 0, 8, 31, 6, 0 },
	{ GID_QFG1, 165, 0, 0, 9, 30, 6, 0 },
	// G: the bar turning in from its right side
	{ GID_QFG1, 165, 0, 0, 8, 34, 6, 0 },
	// PQ2 Keith facing you (view 20): his face is 4 fat pixels wide, so the eyes merge. The
	// lower of the two black pixels on the left of his head becomes hair brown, and the eyes
	// move apart with skin between: left eye on the face's left edge
	{ GID_PQ2, 20, 2, -1, 4, 2, 0, 6 },
	{ GID_PQ2, 20, 2, -1, 5, 2, 4, 0 },
	{ GID_PQ2, 20, 2, -1, 5, 3, 0, 12 },
	// PQ2 Sonny in the office (view 3 loop 3, three-quarter face): the two eye pixels merge into a
	// black bar. Only the right eye is kept
	{ GID_PQ2, 3, 3, -1, 5, 4, 0, 12 },
	// PQ2 man in the yellow top (view 61 loop 6): eyes merged on a 4 fat pixel face. Left eye
	// moves onto the grey hair edge, skin between
	{ GID_PQ2, 61, 6, -1, 3, 2, 8, 0 },
	{ GID_PQ2, 61, 6, -1, 3, 3, 0, 12 },
	// PQ2 man at the desk (view 68 cel 2): eyes merged on a 4 fat pixel face. Right eye moves onto
	// the hair edge, face between, and face colour under both eyes as in SCI
	{ GID_PQ2, 68, 0, 2, 5, 5, 0, 12 },
	{ GID_PQ2, 68, 0, 2, 5, 6, 6, 0 },
	{ GID_PQ2, 68, 0, 2, 6, 4, 6, 12 },
	{ GID_PQ2, 68, 0, 2, 6, 6, 6, 12 },
	// PQ2 Narcotics files, front photo eyes made symmetric after the general eye rebuild (rows, pairs
	// at the files' pairing). Dickey (205 loop 1 cel 2): his right eye (our right) mirrored onto the left
	{ GID_PQ2, 205, 1, 2, 22, 5, 0xFF, 15 },
	{ GID_PQ2, 205, 1, 2, 22, 6, 0xFF, 1 },
	{ GID_PQ2, 205, 1, 2, 22, 7, 0xFF, 15 },
	{ GID_PQ2, 205, 1, 2, 22, 8, 0xFF, 12 },
	{ GID_PQ2, 205, 1, 2, 22, 9, 0xFF, 12 },
	{ GID_PQ2, 205, 1, 2, 22, 10, 0xFF, 15 },
	{ GID_PQ2, 205, 1, 2, 22, 11, 0xFF, 1 },
	{ GID_PQ2, 205, 1, 2, 22, 12, 0xFF, 15 },
	{ GID_PQ2, 205, 1, 2, 23, 5, 0xFF, 15 },
	{ GID_PQ2, 205, 1, 2, 23, 6, 0xFF, 1 },
	{ GID_PQ2, 205, 1, 2, 23, 7, 0xFF, 15 },
	{ GID_PQ2, 205, 1, 2, 23, 8, 0xFF, 12 },
	{ GID_PQ2, 205, 1, 2, 23, 9, 0xFF, 12 },
	{ GID_PQ2, 205, 1, 2, 23, 10, 0xFF, 15 },
	{ GID_PQ2, 205, 1, 2, 23, 11, 0xFF, 1 },
	{ GID_PQ2, 205, 1, 2, 23, 12, 0xFF, 15 },
	// Jones (205 loop 0 cel 2): his right eye (our right, as in v0.164) mirrored onto the left, with the
	// black pupil one fat pixel further out on each eye so he doesn't look cross-eyed
	{ GID_PQ2, 205, 0, 2, 21, 5, 0xFF, 15 },
	{ GID_PQ2, 205, 0, 2, 21, 6, 0xFF, 0 },
	{ GID_PQ2, 205, 0, 2, 21, 7, 0xFF, 15 },
	{ GID_PQ2, 205, 0, 2, 21, 8, 0xFF, 15 },
	{ GID_PQ2, 205, 0, 2, 21, 9, 0xFF, 6 },
	{ GID_PQ2, 205, 0, 2, 21, 10, 0xFF, 15 },
	{ GID_PQ2, 205, 0, 2, 21, 11, 0xFF, 15 },
	{ GID_PQ2, 205, 0, 2, 21, 12, 0xFF, 0 },
	{ GID_PQ2, 205, 0, 2, 21, 13, 0xFF, 15 },
	{ GID_PQ2, 205, 0, 2, 22, 5, 0xFF, 6 },
	{ GID_PQ2, 205, 0, 2, 22, 6, 0xFF, 0 },
	{ GID_PQ2, 205, 0, 2, 22, 7, 0xFF, 15 },
	{ GID_PQ2, 205, 0, 2, 22, 8, 0xFF, 4 },
	{ GID_PQ2, 205, 0, 2, 22, 9, 0xFF, 6 },
	{ GID_PQ2, 205, 0, 2, 22, 10, 0xFF, 4 },
	{ GID_PQ2, 205, 0, 2, 22, 11, 0xFF, 15 },
	{ GID_PQ2, 205, 0, 2, 22, 12, 0xFF, 0 },
	{ GID_PQ2, 205, 0, 2, 22, 13, 0xFF, 6 },
	// Jones: pupils 2 fat pixels wide on the top eye row (the extra one on the nose side)
	{ GID_PQ2, 205, 0, 2, 21, 7, 0xFF, 0 },
	{ GID_PQ2, 205, 0, 2, 21, 11, 0xFF, 0 },
	// Jones: the moustache lost its left end in the collapse. Two black fat pixels to the left of his
	// mouth make it match the right
	{ GID_PQ2, 205, 0, 2, 32, 7, 6, 0 },
	{ GID_PQ2, 205, 0, 2, 33, 7, 6, 0 },
	// PQ2 shooting range target touch-ups (view 70 loop 2, at the pairings in kAgiDemakeTargetStartX):
	// rings closed, "10"s kept off the rings, c4's ring widened and its right frame kept
	// cel 0: 10 fat pixels
	{ GID_PQ2, 70, 2, 0, 5, 15, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 0, 5, 16, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 0, 12, 17, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 0, 13, 17, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 0, 21, 15, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 0, 21, 16, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 0, 32, 13, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 0, 32, 18, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 0, 77, 13, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 0, 77, 18, 0xFF, 15 },
	// cel 1: 6 fat pixels
	{ GID_PQ2, 70, 2, 1, 5, 12, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 1, 5, 13, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 1, 10, 14, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 1, 11, 14, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 1, 17, 12, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 1, 17, 13, 0xFF, 15 },
	// cel 1: frame kept black on both sides where the thin white rule turned it white
	{ GID_PQ2, 70, 2, 1, 47, 0, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 1, 47, 25, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 1, 48, 0, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 1, 48, 25, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 1, 49, 0, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 1, 49, 25, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 1, 50, 0, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 1, 50, 25, 0xFF, 0 },
	// cel 2: 5 fat pixels
	{ GID_PQ2, 70, 2, 2, 29, 11, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 2, 30, 11, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 2, 31, 11, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 2, 32, 11, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 2, 33, 11, 0xFF, 0 },
	// cel 3: 19 fat pixels
	{ GID_PQ2, 70, 2, 3, 36, 4, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 3, 36, 5, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 3, 36, 6, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 3, 36, 10, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 3, 36, 11, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 3, 36, 12, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 3, 37, 4, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 3, 37, 5, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 3, 37, 6, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 3, 37, 10, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 3, 37, 11, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 3, 37, 12, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 3, 38, 4, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 3, 38, 6, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 3, 38, 7, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 3, 38, 8, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 3, 38, 9, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 3, 38, 10, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 3, 38, 12, 0xFF, 0 },
	// cel 4: 29 fat pixels
	{ GID_PQ2, 70, 2, 4, 15, 8, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 4, 15, 9, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 4, 16, 8, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 4, 16, 9, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 4, 17, 8, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 4, 17, 9, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 4, 18, 8, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 4, 18, 9, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 4, 19, 8, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 4, 19, 9, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 4, 20, 8, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 4, 20, 9, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 4, 21, 8, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 4, 22, 8, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 4, 22, 9, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 4, 22, 12, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 4, 23, 8, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 4, 23, 9, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 4, 23, 12, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 4, 24, 8, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 4, 24, 12, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 4, 25, 8, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 4, 25, 9, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 4, 26, 8, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 4, 26, 9, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 4, 27, 8, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 4, 27, 9, 0xFF, 15 },
	{ GID_PQ2, 70, 2, 4, 28, 8, 0xFF, 0 },
	{ GID_PQ2, 70, 2, 4, 28, 9, 0xFF, 15 },
	// PQ2 stewardess facing you (view 26 loop 2): eyes merged on a 4 fat pixel face. Left eye moves onto
	// the face's left edge, skin between
	{ GID_PQ2, 26, 2, -1, 6, 2, 12, 0 },
	{ GID_PQ2, 26, 2, -1, 6, 3, 0, 12 },
	// PQ2 the Chief in the band scene (view 282): his eye kept only its grey white. Black pupil in
	// front, grey white behind, as in SCI. Body cels (loop 1) and talking head cels (loop 2)
	{ GID_PQ2, 282, 1, 0, 17, 10, 7, 0 },
	{ GID_PQ2, 282, 1, 0, 17, 11, 6, 7 },
	{ GID_PQ2, 282, 1, 1, 17, 10, 7, 0 },
	{ GID_PQ2, 282, 1, 1, 17, 11, 6, 7 },
	{ GID_PQ2, 282, 1, 4, 17, 10, 7, 0 },
	{ GID_PQ2, 282, 1, 4, 17, 11, 6, 7 },
	{ GID_PQ2, 282, 1, 5, 17, 10, 7, 0 },
	{ GID_PQ2, 282, 1, 5, 17, 11, 6, 7 },
	{ GID_PQ2, 282, 2, 0, 17, 5, 7, 0 },
	{ GID_PQ2, 282, 2, 0, 17, 6, 6, 7 },
	{ GID_PQ2, 282, 2, 1, 17, 5, 7, 0 },
	{ GID_PQ2, 282, 2, 1, 17, 6, 6, 7 },
	{ GID_PQ2, 282, 2, 2, 17, 5, 7, 0 },
	{ GID_PQ2, 282, 2, 2, 17, 6, 6, 7 },
	{ GID_PQ2, 282, 2, 4, 17, 5, 7, 0 },
	{ GID_PQ2, 282, 2, 4, 17, 6, 6, 7 },
	// talking head: pupil 2 rows high, as on the body cels
	{ GID_PQ2, 282, 2, 0, 16, 5, 6, 0 },
	{ GID_PQ2, 282, 2, 1, 16, 5, 6, 0 },
	{ GID_PQ2, 282, 2, 2, 16, 5, 6, 0 },
	{ GID_PQ2, 282, 2, 4, 16, 5, 6, 0 },
	// Keith in the band scene (view 282 loop 0 cel 2): the Chief's eye in blue. Blue pupil 2 rows high in
	// front, grey white behind on the lower row
	{ GID_PQ2, 282, 0, 2, 14, 4, 0, 12 },
	{ GID_PQ2, 282, 0, 2, 15, 3, 1, 1 },
	{ GID_PQ2, 282, 0, 2, 15, 4, 0, 7 },
	// PQ2 sign on a pole (view 253 cel 0): the black marks run into the sign's white edge after the
	// collapse. One white fat pixel is kept at each side (the red corner can still reach the edge)
	{ GID_PQ2, 253, 0, 0, 2, 0, 0, 15 },
	{ GID_PQ2, 253, 0, 0, 3, 0, 0, 15 },
	{ GID_PQ2, 253, 0, 0, 5, 0, 0, 15 },
	{ GID_PQ2, 253, 0, 0, 6, 0, 0, 15 },
	{ GID_PQ2, 253, 0, 0, 7, 0, 0, 15 },
	{ GID_PQ2, 253, 0, 0, 10, 0, 0, 15 },
	{ GID_PQ2, 253, 0, 0, 12, 0, 0, 15 },
	{ GID_PQ2, 253, 0, 0, 14, 0, 0, 15 },
	{ GID_PQ2, 253, 0, 0, 5, 4, 0, 15 },
	{ GID_PQ2, 253, 0, 0, 6, 4, 0, 15 },
	{ GID_PQ2, 253, 0, 0, 7, 4, 0, 15 },
	{ GID_PQ2, 253, 0, 0, 10, 4, 0, 15 },
	{ GID_PQ2, 253, 0, 0, 12, 4, 0, 15 },
};

static void agiDemakeApplyFatPatches(GuiResourceId view, int16 loop, int16 cel, int h, byte *fat, int fatW, int pad, int numPairs) {
	for (uint i = 0; i < ARRAYSIZE(kAgiDemakeFatPatches); ++i) {
		const AgiDemakeFatPatch &fp = kAgiDemakeFatPatches[i];
		if (fp.game != g_sci->getGameId() || fp.view != view || fp.loop != loop || (fp.cel >= 0 && fp.cel != cel))
			continue;
		if (fp.row < 0 || fp.row >= h || fp.pair < 0 || fp.pair >= numPairs)
			continue;
		byte &px = fat[fp.row * fatW + pad + fp.pair];
		if (fp.from == 0xFF || px == fp.from)	// 0xFF: whatever is there
			px = fp.to;
	}
}

// Hand-drawn blocks of fat pixels, for lettering that has to stay handwriting (no font) but
// does not survive the collapse. A block replaces the cel's fat pixels at the given row and pair,
// one string per row ('.' keeps the pixel). Only pixels that are already the block's two colours,
// or see-through, change, so the block never paints over the card's frame if the pairing shifts.
struct AgiDemakeFatBlock {
	SciGameId game;
	int16 view, loop, cel;
	int16 top, pair;
	byte paper, ink;
	const char *rows[24];
};
static const AgiDemakeFatBlock kAgiDemakeFatBlocks[] = {
	// QfG1 "FARMERS MART" on the fruit stall (view 320 loop 3 cel 4) cannot fit, so it says "MART"
	{ GID_QFG1, 320, 3, 4, 3, 3, 9, 14,
		{ "ff1fff1ff11ff111ff111fff",
		  "ff11f11f1ff1f1ff1ff1ffff",
		  "ff1f1f1f1111f111fff1ffff",
		  "ff1fff1f1ff1f1f1fff1ffff",
		  "ff1fff1f1ff1f1ff1ff1ffff" } },
	// QfG1 "HERO'S TALE INN" sign (view 300 loop 2 cel 0) cannot be read at this size, so it says "INN"
	{ GID_QFG1, 300, 2, 0, 7, 1, 1, 14,
		{ ".fffffffffff.",
		  ".ffffffffffff",
		  ".ffffffffffff",
		  ".ffffffffffff",
		  ".ffffffffffff",
		  ".1f1ff1f1ff1f",
		  ".1f11f1f11f1f",
		  ".1f11f1f11f1f",
		  ".1f1f11f1f11f",
		  ".1f1f11f1f11f",
		  ".1f1ff1f1ff1f",
		  ".1f1ff1f1ff1f",
		  ".ffffffffffff",
		  ".ffffffffffff",
		  ".ffffffffffff",
		  ".ffffffffffff",
		  ".ffffffffffff",
		  "..ffffffffff.",
		  "..fffffffff.." } },
	// QfG1 Sheriff's door board (view 300 loop 2 cel 2) says "Sheriff" in small letters, across where
	// its two rosettes and green trim were (cleared by the patches above)
	{ GID_QFG1, 300, 2, 2, 3, 2, 4, 14,
		{ "..ffffffffffffffffff..",
		  "ffffffffffffffffffffff",
		  "f11f1ffffffffff1ff1ff1",
		  "1fff1ffff1ff11fff1ff1f",
		  "f1ff11ff1f1f1ff1f11f11",
		  "ff1f1f1f11ff1ff1f1ff1f",
		  "11ff1f1ff11f1ff1f1ff1f" } },
	// QfG1 title sign "Wanted: Hero" (view 100 cel 0): letters re-spaced and drawn one by one like "Village of Spielburg",
	// W and H hand-drawn narrower
	{ GID_QFG1, 100, 0, 0, 0, 0, 2, 0,
		{ "fff1fff1fffffffffffffffffffffffff1ffffffffffffffff",
		  "ff11ff11ffffffffffffffffffffffff11ff1fffffffffffff",
		  "fff1fff1ffffffffff1ffffffff1fffff1f11fffffffffffff",
		  "fff1fff1ffffffffff1ffffffff1fffff1ff1fffffffffffff",
		  "fff1fff1ffffffffff11fffffff1fffff1ff1fffffffffffff",
		  "fff1fff1fffffffff111fffffff1fffff1ff1ffffffffff1ff",
		  "fff1fff1ff11fff1f11fff11fff1fffff1ff1ff11f11ff11ff",
		  "fff1fff1f1f1f111ff1ff111ff11f1fff1ff1f111f11f111ff",
		  "fff1f1f1f1f1f1f1ff1ff1f1f111f1fff1111f1f1f11f1f1ff",
		  "fff1f1f1f111f1f1ff1ff1f1f1f1fffff1ff1f1f1f1ff1f1ff",
		  "fff1f1f1f1f1f1f1ff1ff111f1f1fffff1ff1f111f1ff1f1ff",
		  "fff1f1f1f1f1f1f1ff1ff11ff1f1f1fff1ff1f11ff1ff1f1ff",
		  "fff1f1f1f1f1f1f1ff1ff1fff111f1fff1ff1f1fff1ff111ff",
		  "fff1f1f1f1f1f1f1ff11f1f1f111fffff1ff1f1f1f1ff111ff",
		  "ffff1f1ff111f1f1ff1ff111f111fffff1ff1f111f1ff11fff",
		  "fffffffff1fff1f1ff1ff1fff1f1fffff1ff1f1fff1ff1ffff",
		  "ffffffffffffffffffffffffffffffffffffffffffffffffff" } },
	// QfG1 title sign "for the" (view 100 cel 1): letters re-spaced and drawn one by one like "Village of Spielburg"
	{ GID_QFG1, 100, 0, 1, 0, 0, 2, 0,
		{ "f11fffffffff1ff1ffffff",
		  "f11fffffffff1ff1ffffff",
		  "f1ffff1f11ff1ff1fffff1",
		  "f1ffff1f11ff11f1f1f111",
		  "f11f111f1fff11f111f1f1",
		  "f1ff1f1f1fff1ff1f1f1f1",
		  "f1ff1f1f1fff1ff1f1f1f1",
		  "f1ff1f1f1fff1ff1f1f11f",
		  "f1ff1f1f1fff1ff1f1f1f1",
		  "f1ff1f1f1fff1ff1f1f111",
		  "f1ff11ff1fff1ff1f1f1ff" } },
	// QfG1 title sign "Village of Spielburg" (view 100 cel 2): letters re-spaced and drawn one by one so each keeps its hole, V and S
	// hand-drawn narrower, less space round "of". Starts one fat pixel in, so the V is on the parchment
	{ GID_QFG1, 100, 0, 2, 0, 0, 2, 0,
		{ "f1ff1fff1f1fffffffffffffffffffffffff111fffffffffff1f1fffffffffffff",
		  "f11f1fff1f1fffffffffffffffffff11fff11f1fffffffffff1f1fffffffffffff",
		  "f1ff1f1f1f1fffffffffffffffffff11fff1ffffffff1fffff1f1fffffffffffff",
		  "f1ff1fff1f1fff1fff1fff1fffff1f1ffff11fff1f1fffff1f1f1fffff1f11fff1",
		  "f1ff1f1f1f1ff11fff1f111fffff1f1ffff111ff111f1f111f1f1f1f1f1f11fff1",
		  "f1ff1f1f1f1f1f1f111f1f1fff111f11ffff11ff1f1f1f1f1f1f111f1f1f1ff111",
		  "ff1f1f1f1f1fff1f1f1f1f1fff1f1f1fffff111f1f1f1f1f1f1f1f1f1f1f1ff1f1",
		  "ff1f1f1f1f1f111f1f1f1f1fff1f1f1ffffff11f1f1f1f1f1f1f1f1f1f1f1ff1f1",
		  "ff1f1f1f1f1f1f1f1f1f11ffff1f1f1fffffff1f1f1f1f11ff1f1f1f1f1f1ff1f1",
		  "ff11ff1f1f1f1f1f1f1f1f1fff1f1f1ffff1ff1f1f1f1f1f1f1f1f1f1f1f1ff1f1",
		  "ff11ff1f1f1f111f111f111fff1f1f1ffff1111f1f1f1f111f1f111f111f1ff111",
		  "fff1ff1f1f1f1fff1f1f1fffff11ff1ffff111ff111f1f1fff1f1fff1fff1ff1f1",
		  "ffffffffffffffffff1fffffffffffffffffffff1ffffffffffffffffffffffff1",
		  "fffffffffffffff1ff1fffffffffffffffffffff1fffffffffffffffffffff1ff1",
		  "ffffffffffffffff111fffffffffffffffffffff1ffffffffffffffffffffff111",
		  "ffffffffffffffffff1ffffffffffffffffffffffffffffffffffffffffffffff1" } },
	// PQ2 note pinned to the door (view 136 cel 1): "Sonny" handwritten, redrawn in fat pixels
	{ GID_PQ2, 136, 0, 1, 21, 7, 15, 0,
		{ "ff111fffffffffffffffff",
		  "f1fff1ffffffffffffffff",
		  "1fffffffffffffffffffff",
		  "1fffffffffffffffffffff",
		  "1fffffffffffffffffffff",
		  "f1ffffffffffffffffffff",
		  "ff11ffff1ff11ff11ff1f1",
		  "ffff1ff1f1f1f1f1f1f1f1",
		  "fffff1f1f1f1f1f1f1f1f1",
		  "fffff1f1f1f1f1f1f1f1f1",
		  "fffff1f1f1f1f1f1f1f1f1",
		  "fffff1f1f1f1f1f1f1f1f1",
		  "1ffff1f1f1f1f1f1f1ff11",
		  "f1ff1ff1f1f1f1f1f1fff1",
		  "ff11ffff1ff1f1f1f1fff1",
		  "fffffffffffffffffffff1",
		  "fffffffffffffffffffff1",
		  "fffffffffffffffffffff1",
		  "fffffffffffffffffffff1",
		  "fffffffffffffffffffff1",
		  "fffffffffffffffffffff1",
		  "fffffffffffffffffffff1",
		  "ffffffffffffffffffffff" } },
	// PQ2 Burt Park sign (view 190 loop 1): "BURT PARK" hand-drawn, black border all round, moved out
	// one fat pixel each side so there is a brown gap between the lettering and the border
	{ GID_PQ2, 190, 1, 0, 0, 0, 6, 0,
		{ "111111111111111111111111111111111111",
		  "1ffffffffffffffffffffffffffffffffff1",
		  "1f11ff1f1f11ff111ff11fff1ff11ff1f1f1",
		  "1f1f1f1f1f1f1ff1fff1f1f1f1f1f1f1f1f1",
		  "1f1f1f1f1f1f1ff1fff1f1f1f1f1f1f11ff1",
		  "1f11ff1f1f11fff1fff11ff111f11ff1fff1",
		  "1f1f1f1f1f1f1ff1fff1fff1f1f1f1f11ff1",
		  "1f1f1f1f1f1f1ff1fff1fff1f1f1f1f1f1f1",
		  "1f11ff111f1f1ff1fff1fff1f1f1f1f1f1f1",
		  "1ffffffffffffffffffffffffffffffffff1",
		  "111111111111111111111111111111111111" } },
	// PQ2 back of Sonny's business card: "36-4-12" handwritten, redrawn legibly in fat pixels
	{ GID_PQ2, 137, 0, 1, 15, 5, 15, 1,
		{ "ff11fff11ffffffffffffffff",
		  "ffff1f1fffff1f1ffffffffff",
		  "ffff1f1fffff1f1ffff1f11ff",
		  "fff1ff11ffff1f1fff11fff1f",
		  "ffff1f1f1f1f111f1ff1fff1f",
		  "ffff1f1f1fffff1ffff1ff1ff",
		  "ff11fff1ffffff1ffff1f1fff",
		  "ffffffffffffff1ffff1f1fff",
		  "fffffffffffffffffff1f111f",
		  "fffffffffffffffffffffffff" } },
};

// Draws the lettering into the cel's fat pixels, from the first fat pixel wholly inside the cel
static void agiDemakeQfg1DrawArt(GuiResourceId view, int16 loop, int16 cel, int h, byte clearKey, byte *fat, int fatW, int first, int celLeft) {
	const AgiDemakeQfg1Art *art = agiDemakeQfg1FindArt(view, loop, cel);
	if (!art)
		return;
	for (int i = 0; i < fatW * h; ++i)
		fat[i] = clearKey;
	if (art->onlyHere && celLeft != art->left)
		return;
	for (int y = 0; y < h && y < 17 && art->rows[y]; ++y)
		for (int x = 0; art->rows[y][x]; ++x) {
			const char ch = art->rows[y][x];
			if (ch == '.' || first + x >= fatW)
				continue;
			fat[y * fatW + first + x] = (ch >= 'a') ? ch - 'a' + 10 : ch - '0';
		}
}

// The fat columns the cel's lettering covers now, so the new lettering can be centred on the same spot
static bool agiDemakeFatExtent(const byte *fat, int fatW, int h, byte clearKey, int &lo, int &hi) {
	lo = fatW;
	hi = -1;
	for (int y = 0; y < h; ++y)
		for (int x = 0; x < fatW; ++x)
			if (fat[y * fatW + x] != clearKey) {
				lo = MIN(lo, x);
				hi = MAX(hi, x);
			}
	return hi >= lo;
}

// Lays out columns of lettering on the old lettering: centred on its middle, or lined up with its
// right or left edge
static void agiDemakePlaceColumns(const Common::Array<Common::Array<byte> > &cols, int h, byte clearKey, byte *fat, int fatW, char anchor = 'C') {
	int lo, hi;
	if (!agiDemakeFatExtent(fat, fatW, h, clearKey, lo, hi))
		return;
	const int width = cols.size();
	int start = (anchor == 'R') ? hi + 1 - width : (anchor == 'L') ? lo : lo + ((hi - lo + 1) - width) / 2;
	start = CLIP(start, 0, MAX(0, fatW - width));
	for (int i = 0; i < fatW * h; ++i)
		fat[i] = clearKey;
	for (int x = 0; x < width && start + x < fatW; ++x)
		for (int y = 0; y < h && y < (int)cols[x].size(); ++y)
			if (cols[x][y] != clearKey)
				fat[y * fatW + start + x] = cols[x][y];
}

// QfG1 "PRESENTS" (view 908 loop 1): each letter (a run of lit columns in Sierra's cel) is collapsed on its own, so
// letters no longer merge, with one fat pixel between letters and two between words. Dark blue
// speckle is left out. Centred where Sierra's lettering was
static void agiDemakeQfg1Respace(GuiResourceId view, int16 loop, int16 cel, const SciSpan<const byte> &bitmap, int w, int h, byte clearKey, byte *fat, int fatW) {
	if (g_sci->getGameId() != GID_QFG1 || view != 908 || loop != 1 || cel > 7)
		return;
	auto px = [&](int x, int y) -> byte {
		const byte v = bitmap[y * w + x];
		return (v == 1) ? clearKey : v;
	};
	Common::Array<bool> lit(w, false);
	for (int x = 0; x < w; ++x)
		for (int y = 0; y < h; ++y)
			if (px(x, y) != clearKey)
				lit[x] = true;
	Common::Array<Common::Array<byte> > cols;
	int prevEnd = -1;
	for (int x = 0; x < w;) {
		if (!lit[x]) {
			++x;
			continue;
		}
		const int s = x;
		while (x < w && lit[x])
			++x;
		const int e = x;
		if (prevEnd >= 0) {
			cols.push_back(Common::Array<byte>(h, clearKey));
			if (s - prevEnd >= 3)
				cols.push_back(Common::Array<byte>(h, clearKey));
		}
		for (int c = s; c < e; c += 2) {
			Common::Array<byte> col(h, clearKey);
			for (int y = 0; y < h; ++y) {
				const byte a = px(c, y);
				const byte b = (c + 1 < e) ? px(c + 1, y) : clearKey;
				col[y] = (a != clearKey) ? a : b;
			}
			cols.push_back(col);
		}
		prevEnd = e;
	}
	// The resting PRESENTS (cel 0): each S's bottom half was a solid block. Its middle column is
	// cleared on rows 6 and 7, so the S has its shape (both S's, fat columns 13 and 29), and the N
	// is tidied
	if (cel == 0 && cols.size() > 29 && h > 7) {
		for (int x : { 13, 29 })
			for (int y : { 6, 7 })
				cols[x][y] = clearKey;
		// The N (fat column 20): the light cyan pixel in its left side moves up one, dark green under it
		if (cols[20][5] == 11) {
			cols[20][4] = 11;
			cols[20][5] = 2;
		}
	}
	if (!cols.empty())
		agiDemakePlaceColumns(cols, h, clearKey, fat, fatW);
}

static void agiDemakeApplyFatBlocks(GuiResourceId view, int16 loop, int16 cel, int h, byte clearKey, byte *fat, int fatW, int pad, int numPairs) {
	for (uint i = 0; i < ARRAYSIZE(kAgiDemakeFatBlocks); ++i) {
		const AgiDemakeFatBlock &b = kAgiDemakeFatBlocks[i];
		if (b.game != g_sci->getGameId() || b.view != view || b.loop != loop || b.cel != cel)
			continue;
		for (int r = 0; r < 24 && b.rows[r]; ++r) {
			const int y = b.top + r;
			if (y < 0 || y >= h)
				continue;
			for (int x = 0; b.rows[r][x]; ++x) {
				const int p = b.pair + x;
				const char ch = b.rows[r][x];
				if (ch == '.' || p < -pad || p >= numPairs + pad)
					continue;
				byte &px = fat[y * fatW + pad + p];
				if (px == b.paper || px == b.ink || px == clearKey)
					px = (ch == '1') ? b.ink : b.paper;
			}
		}
	}
}

// PQ2 inventory items (views 100-199): a frame of 3 pixel rings each side, e.g. red, brown and
// green, or black, white and white. On screen the fat pixels sit on even columns, so depending on
// the item's width and screen position the first or last fat pixel of a row hangs half outside
// the item and is clipped, and the outer ring vanishes on that side. On a cel framed like that
// (left 3 columns mirror the right 3 on every row, the same 3 colours down both sides) every row
// is rebuilt to fit inside the item: fat pixels that would be clipped are dropped, the outermost
// whole fat pixel each side gets the outer ring colour and the next one in the inner ring colour.
static bool agiDemakeIsInventoryFrame(const SciSpan<const byte> &bitmap, int w, int h, byte clearKey) {
	if (w < 20 || h < 10)
		return false;
	for (int y = 0; y < h; ++y) {
		const byte *row = bitmap.getUnsafeDataAt(y * w, w);
		for (int i = 0; i < 3; ++i)
			if (row[i] != row[w - 1 - i] || row[i] == clearKey)
				return false;
	}
	const byte *mid = bitmap.getUnsafeDataAt((h / 2) * w, w);
	if (mid[0] == mid[1] && mid[1] == mid[2])
		return false;
	for (int y = 3; y < h - 3; ++y) {
		const byte *row = bitmap.getUnsafeDataAt(y * w, w);
		if (row[0] != mid[0] || row[1] != mid[1] || row[2] != mid[2])
			return false;
	}
	return true;
}

static void agiDemakeInventoryFrame(const SciSpan<const byte> &bitmap, int w, int h, byte clearKey, byte *fat, int fatW, int pad, int numPairs, int pairStart, int celLeft) {
	if (numPairs < 6 || !agiDemakeIsInventoryFrame(bitmap, w, h, clearKey))
		return;
	const byte *mid = bitmap.getUnsafeDataAt((h / 2) * w, w);
	const byte outer = mid[0], inner = mid[2];
	// Most items also have a 2 pixel band of one colour (black) just inside the rings, which the
	// collapse can merge into the picture on one side. When it runs down both sides it is kept as
	// the third fat pixel in from each edge
	const byte bandColor = mid[3];
	bool band = mid[4] == bandColor;
	for (int y = 3; y < h - 3 && band; ++y) {
		const byte *row = bitmap.getUnsafeDataAt(y * w, w);
		if (row[3] != bandColor || row[4] != bandColor || row[w - 4] != bandColor || row[w - 5] != bandColor)
			band = false;
	}
	// first and last fat pixels that land wholly inside the item on screen
	int p0 = 0, p1 = numPairs - 1;
	while (p0 < numPairs && pairStart + 2 * p0 < celLeft)
		++p0;
	while (p1 >= 0 && pairStart + 2 * p1 + 1 > celLeft + w - 1)
		--p1;
	if (p1 - p0 < 6)
		return;
	for (int y = 0; y < h; ++y) {
		byte *f = fat + y * fatW + pad;
		for (int p = -pad; p < p0; ++p)
			f[p] = clearKey;
		for (int p = p1 + 1; p < numPairs + pad; ++p)
			f[p] = clearKey;
		if (y == 0 || y == h - 1)
			continue;
		f[p0] = f[p1] = outer;
		if (y >= 2 && y < h - 2)
			f[p0 + 1] = f[p1 - 1] = inner;
		if (band && y >= 3 && y < h - 3)
			f[p0 + 2] = f[p1 - 2] = bandColor;
	}
}

// PQ2 Jesse Bains' front photo on the two mugshot inventory items (views 112 and 123) and the
// copy protection screen on loadup (view 701 cel 4): the collapse keeps his photo-left eye but
// not the photo-right one. The left eye's 3 fat pixels are
// mirrored onto the right, after one fat pixel of face as the gap, which takes a little of the right
// side of his face. Pairs are counted at the pairing these photos are drawn with (startX -1), and
// nothing changes at any other pairing.
struct AgiDemakeEyeMirror {
	SciGameId game;
	int16 view, loop, cel;		// loop or cel -1: any
	int16 startX;
	int16 firstRow, rows;
	int16 leftPair;		// first of the left eye's 3 fat pixels; the gap is leftPair + 3
};
static const AgiDemakeEyeMirror kAgiDemakeEyeMirrors[] = {
	{ GID_PQ2, 112, -1, -1, -1, 22, 2, 6 },	// any loop/cel: this view only holds his photos
	{ GID_PQ2, 123, -1, -1, -1, 21, 1, 6 },
	{ GID_PQ2, 701, 0, 4, -1, 22, 2, 6 },	// copy protection photo on loadup (same drawing as 112)
};

static void agiDemakeMirrorEyes(GuiResourceId view, int16 loop, int16 cel, int startX, int h, byte *fat, int fatW, int pad, int numPairs) {
	for (uint i = 0; i < ARRAYSIZE(kAgiDemakeEyeMirrors); ++i) {
		const AgiDemakeEyeMirror &m = kAgiDemakeEyeMirrors[i];
		if (m.game != g_sci->getGameId() || m.view != view || (m.loop >= 0 && m.loop != loop) || (m.cel >= 0 && m.cel != cel) || m.startX != startX)
			continue;
		if (m.leftPair + 6 >= numPairs || m.firstRow + m.rows > h)
			continue;
		for (int y = m.firstRow; y < m.firstRow + m.rows; ++y) {
			byte *f = fat + y * fatW + pad;
			for (int k = 0; k < 3; ++k)
				f[m.leftPair + 4 + k] = f[m.leftPair + 2 - k];
		}
	}
}

// Fat pixels that should take one colour from the source pair whenever it is there, where the
// usual darker-wins pick loses it. Pairs are counted at the given pairing (startX) only.
struct AgiDemakePairPrefer {
	SciGameId game;
	int16 view, loop, cel;
	int16 startX, pair;
	int16 firstRow, lastRow;
	byte color;
};
static const AgiDemakePairPrefer kAgiDemakePairPrefers[] = {
	// PQ2 sign on a pole (view 253 cel 0): the pole is a white column with a black shadow column,
	// which collapses to black. It stays white, as in SCI
	{ GID_PQ2, 253, 0, 0, 0, 2, 16, 61, 15 },
	// PQ2 bomb timer digits (view 250 loop 8, any digit): each digit's 1 pixel left stroke shares its pair
	// with black and was lost. The first fat pixel keeps the red
	{ GID_PQ2, 250, 8, -1, 0, 0, 0, 8, 12 },
};

static void agiDemakeApplyPairPrefers(const SciSpan<const byte> &bitmap, GuiResourceId view, int16 loop, int16 cel, int startX, int w, int h, byte *fat, int fatW, int pad, int numPairs) {
	for (uint i = 0; i < ARRAYSIZE(kAgiDemakePairPrefers); ++i) {
		const AgiDemakePairPrefer &pp = kAgiDemakePairPrefers[i];
		if (pp.game != g_sci->getGameId() || pp.view != view || pp.loop != loop || (pp.cel >= 0 && pp.cel != cel) || pp.startX != startX)
			continue;
		if (pp.pair < 0 || pp.pair >= numPairs)
			continue;
		const int c0 = startX + 2 * pp.pair;
		for (int y = MAX<int>(pp.firstRow, 0); y <= pp.lastRow && y < h; ++y)
			for (int k = 0; k < 2; ++k)
				if (c0 + k >= 0 && c0 + k < w && bitmap[y * w + c0 + k] == pp.color)
					fat[y * fatW + pad + pp.pair] = pp.color;
	}
}

// A painted bar (several paint rows with the same ends, e.g. the mugshot placard bars) whose end
// falls in the middle of a fat pixel gets that fat pixel from whatever is beside the bar on each
// row, so the end can step where the picture beside it changes (Snider's file). Those end fat
// pixels are made the same on every row of the bar as on its first row.
static void agiDemakeTidyPaintEdges(const AgiDemakeTextOverride *t, int startX, int h, byte *fat, int fatW, int pad, int numPairs) {
	if (!t || t->paint[0].x0 > t->paint[0].x1)
		return;
	const AgiDemakePaint &first = t->paint[0];
	if (first.y < 0 || first.y >= h)
		return;
	int ends[2], n = 0;
	if ((first.x0 - startX) & 1)
		ends[n++] = (first.x0 - startX) >> 1;
	if (!((first.x1 - startX) & 1))
		ends[n++] = (first.x1 - startX) >> 1;
	for (int i = 1; i < 8; ++i) {
		const AgiDemakePaint &pt = t->paint[i];
		if (pt.x0 > pt.x1)
			break;
		if (pt.x0 != first.x0 || pt.x1 != first.x1 || pt.y < 0 || pt.y >= h)
			continue;
		for (int e = 0; e < n; ++e)
			if (ends[e] >= 0 && ends[e] < numPairs)
				fat[pt.y * fatW + pad + ends[e]] = fat[first.y * fatW + pad + ends[e]];
	}
}

// PQ2 Narcotics file mugshots (view 205): each eye is white, a pupil and iris pair, white. Where the
// pair lands across two fat pixels the collapse keeps one or the other, so the two eyes (and the
// profile's eye) came out in different colours or with no iris at all. Every eye is rebuilt as
// white, iris, white: black for dark red (brown) eyes, the iris colour for blue or green ones. The
// front photo is the left one (columns 0-35); its irises sit towards the nose when the pair has to
// pick a side, the profile's sits next to its white.
static void agiDemakeMugshotEyes(const SciSpan<const byte> &bitmap, int w, int h, int startX, byte *fat, int fatW, int pad, int numPairs) {
	const byte white = 15, black = 0, darkRed = 4, skin = 12, brown = 6;
	const int frontRight = 36, frontMid = 19;
	auto isIris = [](byte c) { return c == 1 || c == 2 || c == 3 || c == 4 || c == 5 || c == 9; };
	for (int y = 15; y < 33 && y < h; ++y) {
		const byte *row = bitmap.getUnsafeDataAt(y * w, w);
		byte *f = fat + y * fatW + pad;
		for (int x = 1; x + 2 < w; ++x) {
			const byte a = row[x], b = row[x + 1];
			if (!((a == black && isIris(b)) || (b == black && isIris(a))))
				continue;
			if (row[x - 1] != white || (row[x + 2] != white && row[x + 2] != skin && row[x + 2] != brown))
				continue;
			const byte iris = (a == black) ? b : a;
			const bool side = x >= frontRight;
			const int p0 = (x - startX) >> 1, p1 = (x + 1 - startX) >> 1;
			int p = p0;
			if (p0 != p1)
				p = side ? p0 : (x < frontMid ? p1 : p0);
			if (p < 1 || p + 1 >= numPairs)
				continue;
			f[p] = (iris == darkRed) ? black : iris;
			const int lc = startX + 2 * (p - 1), rc = startX + 2 * (p + 1);
			if ((lc >= 0 && row[lc] == white) || (lc + 1 >= 0 && lc + 1 < w && row[lc + 1] == white))
				f[p - 1] = white;
			if (rc + 1 < w && (row[rc] == white || row[rc + 1] == white))
				f[p + 1] = white;
			else if (side && rc + 1 < w)
				f[p + 1] = row[x + 2];
			++x;
		}
	}
}

// PQ2 shooting range target (view 70 loop 2, cels 0-4 by distance): its rings and "10"s are white
// lines 1 pixel wide on black, which darker-wins loses. A white pixel with black on both sides wins
// its pair; everything else is darker-wins. Each cel uses the pairing that keeps the most of it, and
// the remaining gaps are touched up in the fat patch table.
static const int8 kAgiDemakeTargetStartX[5] = { -1, 0, -1, -1, -1 };

static byte agiDemakeThinWhitePick(const byte *row, int w, int cx, byte clearKey) {
	const byte white = 15, black = 0;
	for (int k = 0; k < 2; ++k) {
		const int x = cx + k;
		if (x > 0 && x + 1 < w && row[x] == white && row[x - 1] == black && row[x + 1] == black)
			return white;
	}
	const byte a = (cx >= 0) ? row[cx] : clearKey;
	const byte b = (cx + 1 < w) ? row[cx + 1] : clearKey;
	return agiDemakePick(a, b, clearKey);
}

// Diagnostics for adding games: only when "agi_demake_debug=true" is set in the game's section of
// scummvm.ini. Writes agi_demake_dump.txt (each distinct cel drawn, source and result) next to the exe.
static void agiDemakeDump(int viewId, int loopNo, int celNo, const CelInfo *ci, bool mirrored, int startX,
			const SciSpan<const byte> &bitmap, const byte *fat, int fatW, int celLeft, int celTop, int pairStart,
			const Common::Rect &clip, int priority) {
	if (!ConfMan.hasKey("agi_demake_debug") || !ConfMan.getBool("agi_demake_debug"))
		return;
	static Common::DumpFile *file = nullptr;
	static Common::HashMap<Common::String, bool> seen;
	// one entry per cel, pairing and screen parity, so a cel drawn in two places shows up twice
	const Common::String key = Common::String::format("%d/%d/%d/%d/%d", viewId, loopNo, celNo, startX, celLeft & 1);
	if (seen.contains(key))
		return;
	seen[key] = true;
	if (!file) {
		file = new Common::DumpFile();
		if (!file->open(Common::Path("agi_demake_dump.txt"))) {
			delete file;
			file = nullptr;
			return;
		}
	}
	const char *hex = "0123456789abcdef";
	file->writeString(Common::String::format("view %d loop %d cel %d w %d h %d dx %d clear %d mirrored %d startX %d at %d,%d pairStart %d clip %d,%d-%d,%d priority %d\n",
		viewId, loopNo, celNo, ci->width, ci->height, ci->displaceX, ci->clearKey, mirrored ? 1 : 0, startX,
		celLeft, celTop, pairStart, clip.left, clip.top, clip.right, clip.bottom, priority));
	for (int y = 0; y < ci->height; ++y) {
		Common::String line;
		for (int x = 0; x < ci->width; ++x) {
			const byte c = bitmap[y * ci->width + x];
			line += (c == ci->clearKey) ? '.' : (c < 16 ? hex[c] : '?');
		}
		line += "  |  ";
		for (int f = 0; f < fatW; ++f) {
			const byte c = fat[y * fatW + f];
			line += (c == ci->clearKey) ? '.' : (c < 16 ? hex[c] : '?');
		}
		file->writeString(line + "\n");
	}
	file->writeString("\n");
	file->flush();
}

// AGI demake: collapse cel pixel pairs in cel space and snap to even screen columns,
// so the same detail survives whatever x position the sprite is drawn at
// QfG1 look-around (room 300): fat pixels of his standing-away frame's hair that poke out beside
// the turned head, given in screen pixels from the head cel's top-left (each is 2 screen pixels wide)
static const struct { int8 cel, dx, dy; } kAgiDemakeQfgHeadHide[] = {
	// looking left: the column right of the head
	{ 0, 12, 3 }, { 0, 12, 4 }, { 0, 12, 5 }, { 0, 12, 6 }, { 0, 12, 7 }, { 0, 12, 8 },
	// looking right: the same column, the hair above and in front of his face, and the two brown
	// pixels under his nose
	{ 1, 11, 3 }, { 1, 11, 4 }, { 1, 11, 5 }, { 1, 11, 6 }, { 1, 11, 7 }, { 1, 11, 8 },
	{ 1, 9, 1 }, { 1, 9, 5 }, { 1, 9, 7 }, { 1, 9, 8 }
};
// where the turned head was last drawn
static struct { bool valid; uint32 picGeneration; int cel, left, top, priority; } g_agiDemakeQfgHead = { false, 0, 0, 0, 0, 0 };

static bool agiDemakeUnderWindow(const Common::Array<Common::Rect> &rects, int x, int y) {
	for (uint i = 0; i < rects.size(); ++i)
		if (rects[i].contains(x, y))
			return true;
	return false;
}

void GfxView::drawAgiDemake(const Common::Rect &rect, const Common::Rect &clipRect, const Common::Rect &clipRectTranslated,
			int16 loopNo, int16 celNo, byte priority, uint16 scaleSignal, const Palette *palette) {
	// QfG1: a sprite in the room (drawn in the picture's port) doesn't draw over a window that is open
	// above it, as when Erasmus' head kept moving under a message box. The AGI frame makes those
	// boxes bigger than Sierra's, so they now reach over sprites that used to be clear of them
	Common::Array<Common::Rect> windowRects;
	if (g_sci->getGameId() == GID_QFG1 && g_sci->_gfxPorts && g_sci->_gfxPorts->getPort() == g_sci->_gfxPorts->_picWind)
		g_sci->_gfxPorts->agiDemakeOpenWindowRects(windowRects);

	// Some calls (inventory item windows) pass loop and cel numbers past the end. The cel drawn is the
	// clamped one, so every per-cel fix below is matched against the clamped numbers too
	loopNo = CLIP<int16>(loopNo, 0, _loop.size() - 1);
	celNo = CLIP<int16>(celNo, 0, _loop[loopNo].cel.size() - 1);
	const CelInfo *celInfo = getCelInfo(loopNo, celNo);
	const SciSpan<const byte> &origBitmap = getBitmap(loopNo, celNo);
	const int16 celHeight = celInfo->height;
	const int16 celWidth = celInfo->width;
	const byte clearKey = celInfo->clearKey;
	const byte drawMask = priority > 15 ? GFX_SCREEN_MASK_VISUAL : GFX_SCREEN_MASK_VISUAL|GFX_SCREEN_MASK_PRIORITY;
	const int offsetX = clipRectTranslated.left - clipRect.left;
	const int offsetY = clipRectTranslated.top - clipRect.top;

	// Painted lettering that is replaced by AGI font text: remove it from a copy of the cel
	const AgiDemakeTextOverride *textOverride = agiDemakeFindTextOverride(_resourceId, loopNo, celNo);
	if (!textOverride && g_sci->getGameId() == GID_PQ2 && (_resourceId == 204 || _resourceId == 205))	// Homicide, Narcotics files
		textOverride = agiDemakeMugshotPlacard(origBitmap, celWidth, celHeight);
	else if (!textOverride && g_sci->getGameId() == GID_PQ2 && (_resourceId == 112 || _resourceId == 123))
		textOverride = agiDemakeMugshotPlacard(origBitmap, celWidth, celHeight, 0);
	Common::Array<byte> strippedData;
	if (textOverride) {
		strippedData.resize(celWidth * celHeight);
		for (int i = 0; i < celWidth * celHeight; ++i)
			strippedData[i] = origBitmap[i];
		for (int a = 0; a < 4; ++a) {
			const AgiDemakeTextArea &ar = textOverride->areas[a];
			for (int y = MAX<int>(ar.top, 0); y <= MIN<int>(ar.bottom, celHeight - 1); ++y)
				for (int x = MAX<int>(ar.left, 0); x <= MIN<int>(ar.right, celWidth - 1); ++x)
					if (strippedData[y * celWidth + x] == textOverride->letterColor)
						strippedData[y * celWidth + x] = textOverride->background;
		}
		for (int i = 0; i < 8; ++i) {
			const AgiDemakePaint &pt = textOverride->paint[i];
			if (pt.x0 > pt.x1)
				break;
			for (int x = pt.x0; x <= pt.x1; ++x)
				if (pt.y >= 0 && pt.y < celHeight && x >= 0 && x < celWidth)
					strippedData[pt.y * celWidth + x] = pt.color;
		}
	}
	// QfG1 hero: his dark grey outline is dropped before the collapse
	// (view 503 loop 4 cels 4 and 5 are left as Sierra drew them, at Tom's call)
	const bool qfgHeroKeep = g_sci->getGameId() == GID_QFG1 && _resourceId == 503 && loopNo == 4 && (celNo == 4 || celNo == 5);
	const bool qfgHero = agiDemakeIsQfgHero(_resourceId, loopNo) && !qfgHeroKeep;
	const bool qfgSign = agiDemakeIsQfgSign(_resourceId, loopNo);
	if ((qfgHero || qfgSign) && !textOverride) {
		Common::Array<byte> orig(celWidth * celHeight);
		for (int i = 0; i < celWidth * celHeight; ++i)
			orig[i] = origBitmap[i];
		strippedData.resize(celWidth * celHeight);
		if (qfgHero)
			agiDemakeHeroSource(_resourceId, loopNo, orig.data(), celWidth, celHeight, clearKey, strippedData.data());
		else
			agiDemakeSignLetters(orig.data(), celWidth, celHeight, strippedData.data());
	}
	const SciSpan<const byte> strippedSpan(strippedData.data(), strippedData.size());
	const SciSpan<const byte> &bitmap = (textOverride || qfgHero || qfgSign) ? strippedSpan : origBitmap;
	// SCI direction loops (kDirLoop): 0 right, 1 left, 2 towards, 3 away, for views with 4+ loops.
	// Towards and away use the v4 pairing (from the cel's left edge, odd mirrored cels from the
	// right). Side views use the per-loop phase and nose repair.
	const bool fourLoops = _loop.size() >= 4;
	// PQ2 view 99 (sewer: water streams, Sonny falling) has 4+ loops but they aren't facing
	// directions, and the away-head rebuild turned the top of a water stream into a green block
	const bool notWalker = (g_sci->getGameId() == GID_PQ2 && _resourceId == 99) ||
		(g_sci->getGameId() == GID_QFG1 && _resourceId == 32);	// QfG1 Mage's Maze buttons and ladders
	const bool towards = fourLoops && loopNo == 2 && !notWalker;
	const bool away = fourLoops && loopNo == 3 && !notWalker;
	const bool mirrored = _loop[CLIP<int16>(loopNo, 0, _loop.size() - 1)].mirrorFlag;
	const bool still = _screen->agiDemakeStill();
	// Wide side-view sprites (cars and similar): lighter pixel wins, which keeps wheels round
	// QfG1 brigands in the dining hall (view 295 loop 1): wide, but characters, so darker wins as for
	// other characters and their eyes stay
	const bool brigands = g_sci->getGameId() == GID_QFG1 && _resourceId == 295 && loopNo == 1;
	const bool wide = !still && !towards && !away && celWidth >= 40 && !brigands;
	int startX;
	int pairStart;	// screen column of the first pair, always even
	if (still) {
		const byte white = _screen->getColorWhite();
		startX = (agiDemakeEyeScore(bitmap, celWidth, celHeight, clearKey, 0, white) >
			agiDemakeEyeScore(bitmap, celWidth, celHeight, clearKey, -1, white)) ? 0 : -1;
		const int pairScreenX = rect.left + offsetX + startX;
		pairStart = pairScreenX + (pairScreenX & 1);
	} else if (towards || away) {
		startX = (mirrored && (celWidth & 1)) ? -1 : 0;
		pairStart = (rect.left + offsetX + 1) & ~1;
	} else {
		const int anchor = (celWidth >> 1) - celInfo->displaceX;
		startX = ((anchor + agiDemakePhase(loopNo)) & 1) ? -1 : 0;
		// at an odd column the pairs start one column to the left, not the right: shifting right pushed
		// the last fat pixel past the cel's right edge, where it was clipped away (the mugshots' right
		// frame). The half fat pixel this leaves at the left edge is kept whole by the display driver
		const int pairScreenX = rect.left + offsetX + startX;
		pairStart = pairScreenX - (pairScreenX & 1);
	}

	// QfG1 Mage's Maze (view 32: buttons, ladders): paired from their own left edge, so the edge columns
	// (a ladder's left rail, TRIGGER's right frame) aren't in fat pixels half outside the sprite and cut off
	// QfG1 Elsa's room torches (view 97 loop 2): Sierra's flame frames start on odd and even pixels in
	// turn. Each is paired from the screen's own even columns, so the holder stays put and only the
	// flame moves
	if (g_sci->getGameId() == GID_QFG1 && _resourceId == 97 && loopNo == 2 && !still) {
		startX = ((rect.left + offsetX) & 1) ? -1 : 0;
		pairStart = rect.left + offsetX + startX;
	}
	if (g_sci->getGameId() == GID_QFG1 && (_resourceId == 32 || (_resourceId == 341 && loopNo == 5) ||
			(_resourceId == 96 && (loopNo == 4 || (loopNo == 7 && (celNo == 2 || celNo == 3))))) && !still) {
		startX = ((rect.left + offsetX) & 1) ? -1 : 0;
		pairStart = rect.left + offsetX + startX;
	}

	const bool rangeTarget = g_sci->getGameId() == GID_PQ2 && _resourceId == 70 && loopNo == 2 && celNo >= 0 && celNo <= 4;
	if (rangeTarget) {
		startX = kAgiDemakeTargetStartX[celNo];
		const int pairScreenX = rect.left + offsetX + startX;
		pairStart = pairScreenX - (pairScreenX & 1);
	}

	// Collapse the whole cel into fat pixels, with spare fat columns each side for repairs
	const int pad = celWidth / 4 + 3;
	const int numPairs = (celWidth - startX + 1) / 2;
	const int fatW = numPairs + 2 * pad;
	Common::Array<byte> fat(fatW * celHeight, clearKey);
	if (still || wide) {
		// dialog photos and wide sprites (cars, file mugshots): eye whites and pupils are kept
		Common::Array<byte> stillRow;
		for (int cy = 0; cy < celHeight; ++cy) {
			agiDemakeStillRow(bitmap, celWidth, clearKey, _screen->getColorWhite(), cy, startX, stillRow, wide);
			for (int p = 0; p < numPairs && p < (int)stillRow.size(); ++p)
				fat[cy * fatW + p + pad] = stillRow[p];
		}
		agiDemakeFixEyeChecker(bitmap, celWidth, celHeight, clearKey, _screen->getColorWhite(), startX, fat.data(), fatW, pad, numPairs);
	}
	for (int cy = 0; cy < celHeight && !still && !wide; ++cy) {
		const byte *row = bitmap.getUnsafeDataAt(cy * celWidth, celWidth);
		for (int p = 0; p < numPairs; ++p) {
			const int cx = startX + 2 * p;
			byte a = (cx >= 0) ? row[cx] : clearKey;
			byte b = (cx + 1 < celWidth) ? row[cx + 1] : clearKey;
			byte v;
			if (wide) {
				// Wide sprites: lighter wins, except that a pixel on the sprite's outline (its outer
				// neighbour is transparent or off the cel) beats an inner one, so frames and edges stay
				const bool aEdge = (a != clearKey) && (cx == 0 || (cx > 0 && row[cx - 1] == clearKey));
				const bool bEdge = (b != clearKey) && (cx + 1 == celWidth - 1 || (cx + 2 < celWidth && row[cx + 2] == clearKey));
				if (a != clearKey && b != clearKey && bEdge && !aEdge)
					v = b;
				else if (a != clearKey && b != clearKey && aEdge && !bEdge)
					v = a;
				else
					v = agiDemakePickLighter(a, b, clearKey);
			} else {
				// QfG1 saurus head trophy in the guild hall (view 311 loop 2 cel 3): its black mouth lines
				// swallowed the head, so the lighter colour wins on it
				if (g_sci->getGameId() == GID_QFG1 && _resourceId == 311 && loopNo == 2 && celNo == 3)
					v = agiDemakePickLighter(a, b, clearKey);
				// QfG1 green meeps (views 60 and 61): light green beats green in their fur, so they
				// don't vanish into the grass
				else if (g_sci->getGameId() == GID_QFG1 && (_resourceId == 60 || _resourceId == 61) &&
						((a == 2 && b == 10) || (a == 10 && b == 2)))
					v = 10;
				else
					v = agiDemakePick(a, b, clearKey);
			}
			fat[cy * fatW + p + pad] = v;
		}
	}
	if (rangeTarget) {
		for (int cy = 0; cy < celHeight; ++cy) {
			const byte *row = bitmap.getUnsafeDataAt(cy * celWidth, celWidth);
			for (int p = 0; p < numPairs; ++p)
				fat[cy * fatW + p + pad] = agiDemakeThinWhitePick(row, celWidth, startX + 2 * p, clearKey);
		}
	}
	// Away: v4 body, only the head is rebuilt
	// still images: plain collapse, no outline repairs
	if (still) {
	} else if (away)
		agiDemakeAwayHead(bitmap, celWidth, celHeight, clearKey, fat.data(), fatW, startX, pad);
	else if (!towards && !wide && !brigands)
		agiDemakeRepairBumps(bitmap, celWidth, celHeight, clearKey, fat.data(), fatW, startX);
	if (towards && !rangeTarget)
		agiDemakeSpreadEyes(bitmap, celWidth, celHeight, clearKey, fat.data(), fatW, startX, pad);
	if (towards && !rangeTarget)
		agiDemakeHeroEyes(_resourceId, bitmap, celWidth, celHeight, clearKey, fat.data(), fatW, startX, pad, numPairs);
	else if (g_sci->getGameId() == GID_QFG1 && (_resourceId == 503 || _resourceId == 519 || _resourceId == 550 ||
			(_resourceId == 139 && loopNo == 4)) && !qfgHeroKeep)
		agiDemakeHeroEyes(_resourceId, bitmap, celWidth, celHeight, clearKey, fat.data(), fatW, startX, pad, numPairs, loopNo);	// facing you in other poses
	// QfG1 award ceremony, the hero smiling (view 139 loop 4 cel 4): Sierra drew his eyes shut, which
	// as fat pixels leaves him with none. He gets the same eyes as his open-eyed frames (cels 1 to 3):
	// row 5, black at the 2nd and 4th of four face-coloured fat pixels
	if (g_sci->getGameId() == GID_QFG1 && _resourceId == 139 && loopNo == 4 && celNo == 4 && celHeight > 5 && numPairs > 6) {
		byte *f = fat.data() + 5 * fatW + pad;
		for (int x = 3; x <= 6; ++x)
			f[x] = 12;
		f[4] = 0;
		f[6] = 0;
	}
	if (towards || away)
		agiDemakeCopyHead(_resourceId, loopNo, celHeight, clearKey, fat.data(), fatW, pad, numPairs);
	if (g_sci->getGameId() == GID_PQ2 && _resourceId == 205 && celWidth == 73)
		agiDemakeMugshotEyes(origBitmap, celWidth, celHeight, startX, fat.data(), fatW, pad, numPairs);
	agiDemakeKattaEyes(_resourceId, loopNo, bitmap, celWidth, celHeight, clearKey, fat.data(), fatW, startX, pad, numPairs);
	agiDemakeIceManEyes(_resourceId, bitmap, celWidth, celHeight, clearKey, fat.data(), fatW, startX, pad, numPairs);
	agiDemakeHeroSittingHead(_resourceId, loopNo, celNo, celHeight, clearKey, fat.data(), fatW, pad, numPairs);
	agiDemakeHeroSittingRightHead(_resourceId, loopNo, celHeight, clearKey, fat.data(), fatW, pad, numPairs);
	agiDemakeGatePlanks(_resourceId, loopNo, bitmap, celWidth, celHeight, clearKey, fat.data(), fatW, startX, pad, numPairs);
	agiDemakeFightLabel(_resourceId, loopNo, bitmap, celWidth, celHeight, fat.data(), fatW, pad, numPairs);
	agiDemakeYorickHair(_resourceId, loopNo, celHeight, clearKey, fat.data(), fatW, pad, numPairs);
	agiDemakeYorick(_resourceId, loopNo, celNo, celHeight, clearKey, fat.data(), fatW, pad, numPairs);
	agiDemakeBrigandSign(_resourceId, loopNo, celNo, celHeight, fat.data(), fatW, pad, numPairs);
	agiDemakeKnifeLabels(_resourceId, loopNo, celNo, celHeight, clearKey, fat.data(), fatW, pad, numPairs);
	agiDemakeMazeButton(_resourceId, loopNo, celNo, celHeight, fat.data(), fatW, pad, numPairs);
	agiDemakeSkullGate(_resourceId, loopNo, celNo, bitmap, celWidth, celHeight, clearKey, fat.data(), fatW, startX, pad, numPairs);
	agiDemakeMeepFur(_resourceId, loopNo, celHeight, clearKey, fat.data(), fatW, pad, numPairs);
	agiDemakeApplyFatPatches(_resourceId, loopNo, celNo, celHeight, fat.data(), fatW, pad, numPairs);
	// QfG1 guild hall: the "QUESTS" plate (view 311 loop 2 cel 0) is painted wider on the wall
	// instead (see GfxPaint16::drawPicture), so the sprite itself is left out
	if (g_sci->getGameId() == GID_QFG1 && _resourceId == 311 && loopNo == 2 && celNo == 0)
		for (uint i = 0; i < fat.size(); ++i)
			fat[i] = clearKey;
	agiDemakeTidyPaintEdges(textOverride, startX, celHeight, fat.data(), fatW, pad, numPairs);
	agiDemakeQfg1DrawArt(_resourceId, loopNo, celNo, celHeight, clearKey, fat.data(), fatW,
		pad + ((rect.left + offsetX) - pairStart + 1) / 2, rect.left + offsetX);
	agiDemakeQfg1Respace(_resourceId, loopNo, celNo, bitmap, celWidth, celHeight, clearKey, fat.data(), fatW);
	agiDemakeApplyFatBlocks(_resourceId, loopNo, celNo, celHeight, clearKey, fat.data(), fatW, pad, numPairs);
	agiDemakeApplyPairPrefers(bitmap, _resourceId, loopNo, celNo, startX, celWidth, celHeight, fat.data(), fatW, pad, numPairs);
	agiDemakeMirrorEyes(_resourceId, loopNo, celNo, startX, celHeight, fat.data(), fatW, pad, numPairs);
	// PQ2 inventory items: at an odd screen column the item is drawn one column to the left, so it
	// looks the same as at an even column (whole fat pixels from its left edge, the spare column on
	// the right). The frame is then rebuilt to fit inside the item
	int frameLeft = rect.left + offsetX;
	int clipLeft = clipRectTranslated.left;
	if (g_sci->getGameId() == GID_PQ2 && _resourceId >= 100 && _resourceId <= 199 &&
			agiDemakeIsInventoryFrame(bitmap, celWidth, celHeight, clearKey)) {
		if (frameLeft & 1) {
			pairStart = frameLeft - 1;	// even, since frameLeft is odd
			if (clipLeft == frameLeft)
				clipLeft = frameLeft - 1;
			frameLeft -= 1;
		}
		agiDemakeInventoryFrame(bitmap, celWidth, celHeight, clearKey, fat.data(), fatW, pad, numPairs, pairStart, frameLeft);
	}
	agiDemakeDump(_resourceId, loopNo, celNo, celInfo, mirrored, startX, origBitmap, fat.data(), fatW,
		rect.left + offsetX, rect.top + offsetY, pairStart, clipRectTranslated, priority);

	// QfG1 look-around: the standing-away frame leaves out the hair that would poke out beside the
	// turned head, when that head is on screen right now (drawn earlier this cycle: its priority is
	// still on the priority screen at a pixel of its face)
	if (g_sci->getGameId() == GID_QFG1 && _resourceId == 4 && loopNo == 3 && g_agiDemakeQfgHead.valid &&
			g_agiDemakeQfgHead.picGeneration == _screen->_agiPicGeneration &&
			_screen->getPriority(g_agiDemakeQfgHead.left + 4, g_agiDemakeQfgHead.top + 4) == g_agiDemakeQfgHead.priority) {
		const int top = rect.top + offsetY;
		for (uint i = 0; i < ARRAYSIZE(kAgiDemakeQfgHeadHide); ++i) {
			if (kAgiDemakeQfgHeadHide[i].cel != g_agiDemakeQfgHead.cel)
				continue;
			const int x2 = g_agiDemakeQfgHead.left + kAgiDemakeQfgHeadHide[i].dx;
			const int y2 = g_agiDemakeQfgHead.top + kAgiDemakeQfgHeadHide[i].dy;
			const int cy = y2 - top, f = ((x2 - pairStart) >> 1) + pad;
			if (x2 >= pairStart && cy >= 0 && cy < celHeight && f < fatW) {
				byte &v = fat[cy * fatW + f];
				if (v == 6 || v == 14)
					v = clearKey;
			}
		}
	}

	// Sprite fat pixels are marked (map value 3) so the display driver keeps a fat pixel whole when
	// the item's edge cuts it in half on screen, instead of letting the background take the pair
	const byte oldSpriteMapValue = _screen->getCurPaletteMapValue();
	_screen->setCurPaletteMapValue(3);
	// PQ2 car park (room 1): the chain-link fence in front of the parked cars is 1 pixel wires with
	// gaps between. A fat pixel with a wire over one half and a gap under the other went wholly to
	// the car (see the sprite edge marking above), so the fence vanished. Here a sprite fat pixel
	// where something in front covers either half shows the thing in front in both halves
	const bool hideHalfCovered = g_sci->getGameId() == GID_PQ2 && g_sci->getEngineState() &&
		g_sci->getEngineState()->currentRoomNumber() == 1;
	for (int cy = clipRect.top - rect.top; cy < MIN<int>(celHeight, clipRect.bottom - rect.top); ++cy) {
		const int y2 = rect.top + cy + offsetY;
		for (int f = 0; f < fatW; ++f) {
			const byte color = fat[cy * fatW + f];
			if (color == clearKey)
				continue;
			if (hideHalfCovered) {
				int coveredX = -1;
				for (int i = 0; i < 2; ++i) {
					const int x2 = pairStart + 2 * (f - pad) + i;
					if (x2 >= clipLeft && x2 < clipRectTranslated.right && priority < _screen->getPriority(x2, y2))
						coveredX = x2;
				}
				if (coveredX >= 0) {
					// The other half shows the fence too: left as it was, it showed whatever is behind
					// the car there (the yellow parking line)
					const byte front = _screen->getVisual(coveredX, y2);
					for (int i = 0; i < 2; ++i) {
						const int x2 = pairStart + 2 * (f - pad) + i;
						if (x2 != coveredX && x2 >= clipLeft && x2 < clipRectTranslated.right &&
								priority >= _screen->getPriority(x2, y2) && !agiDemakeUnderWindow(windowRects, x2, y2))
							_screen->putPixel(x2, y2, GFX_SCREEN_MASK_VISUAL, front, 0, 0);
					}
					continue;
				}
			}
			for (int i = 0; i < 2; ++i) {
				const int x2 = pairStart + 2 * (f - pad) + i;
				if (x2 < clipLeft || x2 >= clipRectTranslated.right)
					continue;
				if (priority >= _screen->getPriority(x2, y2) && !agiDemakeUnderWindow(windowRects, x2, y2))
					_screen->putPixel(x2, y2, drawMask, getMappedColor(color, scaleSignal, palette, x2, y2), priority, 0);
			}
		}
	}
	// QfG1 meep's fur clump (view 61 loop 8): a black shadow one row under each of its lowest pixels
	if (g_sci->getGameId() == GID_QFG1 && _resourceId == 61 && loopNo == 8) {
		for (int f = 0; f < fatW; ++f)
			for (int cy = 0; cy < celHeight; ++cy) {
				if (fat[cy * fatW + f] == clearKey || (cy + 1 < celHeight && fat[(cy + 1) * fatW + f] != clearKey))
					continue;
				const int y2 = rect.top + cy + 1 + offsetY;
				for (int i = 0; i < 2; ++i) {
					const int x2 = pairStart + 2 * (f - pad) + i;
					if (x2 < 0 || x2 >= _screen->getWidth() || y2 >= _screen->getHeight())
						continue;
					if (priority >= _screen->getPriority(x2, y2))
						_screen->putPixel(x2, y2, drawMask, getMappedColor(0, scaleSignal, palette, x2, y2), priority, 0);
				}
			}
	}
	// QfG1 fight screen labels: the box's left frame line, one fat pixel left of the sprite
	if (agiDemakeIsFightLabel(_resourceId, loopNo, celWidth, celHeight)) {
		for (int cy = clipRect.top - rect.top; cy < MIN<int>(celHeight, clipRect.bottom - rect.top); ++cy) {
			const int y2 = rect.top + cy + offsetY;
			for (int i = 0; i < 2; ++i) {
				const int x2 = pairStart - 2 + i;
				if (x2 < 0 || x2 >= _screen->getWidth())
					continue;
				if (priority >= _screen->getPriority(x2, y2))
					_screen->putPixel(x2, y2, drawMask, getMappedColor(9, scaleSignal, palette, x2, y2), priority, 0);
			}
		}
	}
	// QfG1 Yorick's room: the SECRET DOOR letters at full resolution (the AGI text resolution),
	// marked so the display keeps them as single pixels
	if (g_sci->getGameId() == GID_QFG1 && _resourceId == 96 && loopNo == 7 && celNo == 2) {
		const bool box = false;
		const int8 (*ink)[2] = kAgiYorickSecretInk;
		const int count = ARRAYSIZE(kAgiYorickSecretInk);
		const byte inkColor = box ? 0 : 14;
		_screen->setCurPaletteMapValue(2);
		for (int i = 0; i < count; ++i) {
			const int x2 = rect.left + offsetX + ink[i][1], y2 = rect.top + offsetY + ink[i][0];
			if (x2 < clipRectTranslated.left - 1 || x2 >= clipRectTranslated.right || y2 < clipRectTranslated.top - 1 || y2 >= clipRectTranslated.bottom)
				continue;
			if (x2 < 0 || y2 < 0 || x2 >= _screen->getWidth() || y2 >= _screen->getHeight())
				continue;
			if (priority >= _screen->getPriority(x2, y2) && !agiDemakeUnderWindow(windowRects, x2, y2))
				_screen->putPixel(x2, y2, drawMask, getMappedColor(inkColor, scaleSignal, palette, x2, y2), priority, 0);
		}
	}
	_screen->setCurPaletteMapValue(oldSpriteMapValue);

	// QfG1 look-around (room 300): the turned head (view 300 loop 8) is drawn over his standing-away
	// frame (view 4 loop 3), and as fat pixels that frame's own hair pokes out beside the turned
	// head. Those fat pixels (see kAgiDemakeQfgHeadHide) are hidden whichever of the two is drawn
	// second: drawn after the body, the head puts the room picture back there (only over his
	// yellow or brown hair). Drawn before it, the head is remembered and the body leaves them out
	if (g_sci->getGameId() == GID_QFG1 && _resourceId == 300 && loopNo == 8 && celNo <= 1) {
		const int left = rect.left + offsetX, top = rect.top + offsetY;
		g_agiDemakeQfgHead.valid = true;
		g_agiDemakeQfgHead.picGeneration = _screen->_agiPicGeneration;
		g_agiDemakeQfgHead.cel = celNo;
		g_agiDemakeQfgHead.left = left;
		g_agiDemakeQfgHead.top = top;
		g_agiDemakeQfgHead.priority = priority;
		for (uint i = 0; i < ARRAYSIZE(kAgiDemakeQfgHeadHide); ++i) {
			if (kAgiDemakeQfgHeadHide[i].cel != celNo)
				continue;
			for (int d = 0; d < 2; ++d) {
				const int x2 = left + kAgiDemakeQfgHeadHide[i].dx + d, y2 = top + kAgiDemakeQfgHeadHide[i].dy;
				const byte c = _screen->getVisual(x2, y2);
				if (c == 6 || c == 14)
					_screen->agiDemakeRestorePicturePixel(x2, y2);
			}
		}
	}

	// Replacement words: backing box, then letters, at full resolution and flagged as text
	if (textOverride) {
		const byte oldMapValue = _screen->getCurPaletteMapValue();
		_screen->setCurPaletteMapValue(1);
		for (int pass = 0; pass < 2; ++pass) {
			for (int it = 0; it < 6 && textOverride->items[it].word; ++it) {
				const AgiDemakeTextItem &item = textOverride->items[it];
				const int wordW = agiDemakeWordWidth(item.word);
				const int x0 = item.alignRight ? item.x - wordW : item.x;
				const int y0 = item.y;
				Common::Array<bool> ink((wordW + 2) * 9, false);
				int gxPos = 0;
				for (const char *ch = item.word; *ch; ++ch) {
					const uint8 *g = Graphics::DosFont::fontData_PCBIOS + (byte)*ch * 8;
					int a, b;
					agiDemakeGlyphCols((byte)*ch, a, b);
					for (int gy = 0; gy < 7; ++gy)
						for (int gx = a; gx <= b; ++gx)
							if (g[gy] & (0x80 >> gx))
								ink[(gy + 1) * (wordW + 2) + gxPos + gx - a + 1] = true;
					gxPos += b - a + 2;
				}
				for (int by = 0; by < 9; ++by) {
					for (int bx = 0; bx < wordW + 2; ++bx) {
						const int cx = x0 - 1 + bx, cy = y0 - 1 + by;
						if (cx < 0 || cx >= celWidth || cy < 0 || cy >= celHeight)
							continue;
						// the painted copy, not the original: a painted bar can cover pixels that are
						// see-through in the original (the gap between Snider's two placards)
						if (bitmap[cy * celWidth + cx] == clearKey)
							continue;
						const bool isInk = ink[by * (wordW + 2) + bx];
						if (pass == 1 && !isInk)
							continue;
						if (pass == 0 && textOverride->backing == 0xFF)
							continue;
						const int x2 = rect.left + offsetX + cx, y2 = rect.top + offsetY + cy;
						if (x2 < clipRectTranslated.left || x2 >= clipRectTranslated.right || y2 < clipRectTranslated.top || y2 >= clipRectTranslated.bottom)
							continue;
						if (priority >= _screen->getPriority(x2, y2)) {
							const byte c = (pass == 1) ? textOverride->textColor : textOverride->backing;
							_screen->putPixel(x2, y2, drawMask, getMappedColor(c, scaleSignal, palette, x2, y2), priority, 0);
						}
					}
				}
			}
		}
		_screen->setCurPaletteMapValue(oldMapValue);
	}
}

void GfxView::drawScaled(const Common::Rect &rect, const Common::Rect &clipRect, const Common::Rect &clipRectTranslated,
			int16 loopNo, int16 celNo, byte priority, int16 scaleX, int16 scaleY, uint16 scaleSignal) {
	const Palette *palette = _embeddedPal ? &_viewPalette : &_palette->_sysPalette;
	const CelInfo *celInfo = getCelInfo(loopNo, celNo);
	const SciSpan<const byte> &bitmap = getBitmap(loopNo, celNo);
	const int16 celHeight = celInfo->height;
	const int16 celWidth = celInfo->width;
	const byte clearKey = celInfo->clearKey;
	const byte drawMask = priority > 15 ? GFX_SCREEN_MASK_VISUAL : GFX_SCREEN_MASK_VISUAL|GFX_SCREEN_MASK_PRIORITY;

	if (_embeddedPal)
		// Merge view palette in...
		_palette->set(&_viewPalette, false);

	Common::Array<uint16> scalingX, scalingY;
	const bool mirrorFlag = _loop[CLIP<int16>(loopNo, 0, _loop.size() - 1)].mirrorFlag;
	createScalingTable(scalingX, celWidth, _screen->getWidth(), scaleX, mirrorFlag);
	if (mirrorFlag) {
		// reverse the table when mirroring; we already reversed the bitmap
		uint scaleTableSize = scalingX.size();
		for (uint i = 0; i < scaleTableSize / 2; i++) {
			SWAP(scalingX[i], scalingX[scaleTableSize - i - 1]);
		}
	}
	createScalingTable(scalingY, celHeight, _screen->getHeight(), scaleY, false);

	int16 scaledWidth = MIN(clipRect.width(), (int16)scalingX.size());
	int16 scaledHeight = MIN(clipRect.height(), (int16)scalingY.size());

	const int16 offsetY = clipRect.top - rect.top;
	const int16 offsetX = clipRect.left - rect.left;

	const byte *bitmapData = bitmap.getUnsafeDataAt(0, celWidth * celHeight);
	for (int y = 0; y < scaledHeight; y++) {
		for (int x = 0; x < scaledWidth; x++) {
			const byte color = bitmapData[scalingY[y + offsetY] * celWidth + scalingX[x + offsetX]];
			const int x2 = clipRectTranslated.left + x;
			const int y2 = clipRectTranslated.top + y;
			if (color != clearKey && priority >= _screen->getPriority(x2, y2)) {
				_screen->putPixel(x2, y2, drawMask, getMappedColor(color, scaleSignal, palette, x2, y2), priority, 0);
			}
		}
	}
}

void GfxView::createScalingTable(Common::Array<uint16> &table, int16 celSize, uint16 maxSize, int16 scale, bool mirrorFlag) {
	const int16 scaledSize = (celSize * scale) >> 7;
	const int16 clippedScaledSize = CLIP<int16>(scaledSize, 0, maxSize);
	const int16 stepCount = scaledSize - 1;

	if (clippedScaledSize <= 0) {
		table.clear();
		return;
	}

	// Note that the table produced by this algorithm when mirroring
	// is slightly different than simply reversing the normal table.
	const int16 start = mirrorFlag ? (celSize - 1) : 0;
	const int16 end   = mirrorFlag ? 0 : (celSize - 1);

	int32 acc;
	int32 inc;
	bool negative = false;
	if (stepCount == 0) {
		acc = start << 16;
		inc = 0;
	} else {
		acc = start << 16;
		inc = end << 16;
		inc -= acc;
		inc /= stepCount;
		if (inc < 0) {
			inc = -inc;
			negative = true;
		}
		if ((inc & 0xffff8000) == 0) {
			acc = (acc & 0xffff0000) | 0x8000;
		} else {
			acc = (acc & 0xffff0000) | (inc & 0xffff);
		}
	}
	if (negative) {
		inc = -inc;
	}

	table.resize(clippedScaledSize);
	for (uint16 i = 0; i < clippedScaledSize; ++i) {
		table[i] = acc >> 16;
		acc += inc;
	}
}

void GfxView::adjustToUpscaledCoordinates(int16 &y, int16 &x) {
	_screen->adjustToUpscaledCoordinates(y, x);
}

void GfxView::adjustBackUpscaledCoordinates(int16 &y, int16 &x) {
	_screen->adjustBackUpscaledCoordinates(y, x);
}

} // End of namespace Sci
