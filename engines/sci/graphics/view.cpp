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

#include "sci/graphics/scifx.h"

namespace Sci {

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
		if (px == fp.from)
			px = fp.to;
	}
}

// Hand-drawn blocks of fat pixels, for lettering that has to stay handwriting (no font) but
// does not survive the collapse. A block replaces the cel's fat pixels at the given row and pair,
// one string per row ('.' keeps the pixel). Only pixels that are already the block's two colours
// change, so the block never paints over the card's frame if the pairing ever shifts.
struct AgiDemakeFatBlock {
	SciGameId game;
	int16 view, loop, cel;
	int16 top, pair;
	byte paper, ink;
	const char *rows[24];
};
static const AgiDemakeFatBlock kAgiDemakeFatBlocks[] = {
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

static void agiDemakeApplyFatBlocks(GuiResourceId view, int16 loop, int16 cel, int h, byte *fat, int fatW, int pad, int numPairs) {
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
				if (ch == '.' || p < 0 || p >= numPairs)
					continue;
				byte &px = fat[y * fatW + pad + p];
				if (px == b.paper || px == b.ink)
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
};

static void agiDemakeApplyPairPrefers(const SciSpan<const byte> &bitmap, GuiResourceId view, int16 loop, int16 cel, int startX, int w, int h, byte *fat, int fatW, int pad, int numPairs) {
	for (uint i = 0; i < ARRAYSIZE(kAgiDemakePairPrefers); ++i) {
		const AgiDemakePairPrefer &pp = kAgiDemakePairPrefers[i];
		if (pp.game != g_sci->getGameId() || pp.view != view || pp.loop != loop || pp.cel != cel || pp.startX != startX)
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

// Diagnostics for adding games: TEMP always on in test builds (normally "agi_demake_debug=true" in the game's section of
// scummvm.ini). Writes agi_demake_dump.txt (each distinct cel drawn, source and result) next to the exe.
static void agiDemakeDump(int viewId, int loopNo, int celNo, const CelInfo *ci, bool mirrored, int startX,
			const SciSpan<const byte> &bitmap, const byte *fat, int fatW, int celLeft, int celTop, int pairStart,
			const Common::Rect &clip, int priority) {
	// TEMP: always on for testing, remove before next public release
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
void GfxView::drawAgiDemake(const Common::Rect &rect, const Common::Rect &clipRect, const Common::Rect &clipRectTranslated,
			int16 loopNo, int16 celNo, byte priority, uint16 scaleSignal, const Palette *palette) {
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
	const SciSpan<const byte> strippedSpan(strippedData.data(), strippedData.size());
	const SciSpan<const byte> &bitmap = textOverride ? strippedSpan : origBitmap;
	// SCI direction loops (kDirLoop): 0 right, 1 left, 2 towards, 3 away, for views with 4+ loops.
	// Towards and away use the v4 pairing (from the cel's left edge, odd mirrored cels from the
	// right). Side views use the per-loop phase and nose repair.
	const bool fourLoops = _loop.size() >= 4;
	const bool towards = fourLoops && loopNo == 2;
	const bool away = fourLoops && loopNo == 3;
	const bool mirrored = _loop[CLIP<int16>(loopNo, 0, _loop.size() - 1)].mirrorFlag;
	const bool still = _screen->agiDemakeStill();
	// Wide side-view sprites (cars and similar): lighter pixel wins, which keeps wheels round
	const bool wide = !still && !towards && !away && celWidth >= 40;
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
		const int pairScreenX = rect.left + offsetX + startX;
		pairStart = pairScreenX + (pairScreenX & 1);
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
				v = agiDemakePick(a, b, clearKey);
			}
			fat[cy * fatW + p + pad] = v;
		}
	}
	// Away: v4 body, only the head is rebuilt
	// still images: plain collapse, no outline repairs
	if (still) {
	} else if (away)
		agiDemakeAwayHead(bitmap, celWidth, celHeight, clearKey, fat.data(), fatW, startX, pad);
	else if (!towards && !wide)
		agiDemakeRepairBumps(bitmap, celWidth, celHeight, clearKey, fat.data(), fatW, startX);
	if (towards)
		agiDemakeSpreadEyes(bitmap, celWidth, celHeight, clearKey, fat.data(), fatW, startX, pad);
	if (towards || away)
		agiDemakeCopyHead(_resourceId, loopNo, celHeight, clearKey, fat.data(), fatW, pad, numPairs);
	agiDemakeApplyFatPatches(_resourceId, loopNo, celNo, celHeight, fat.data(), fatW, pad, numPairs);
	agiDemakeApplyFatBlocks(_resourceId, loopNo, celNo, celHeight, fat.data(), fatW, pad, numPairs);
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
	agiDemakeDump(_resourceId, loopNo, celNo, celInfo, mirrored, startX, bitmap, fat.data(), fatW,
		rect.left + offsetX, rect.top + offsetY, pairStart, clipRectTranslated, priority);

	// Sprite fat pixels are marked (map value 3) so the display driver keeps a fat pixel whole when
	// the item's edge cuts it in half on screen, instead of letting the background take the pair
	const byte oldSpriteMapValue = _screen->getCurPaletteMapValue();
	_screen->setCurPaletteMapValue(3);
	for (int cy = clipRect.top - rect.top; cy < MIN<int>(celHeight, clipRect.bottom - rect.top); ++cy) {
		const int y2 = rect.top + cy + offsetY;
		for (int f = 0; f < fatW; ++f) {
			const byte color = fat[cy * fatW + f];
			if (color == clearKey)
				continue;
			for (int i = 0; i < 2; ++i) {
				const int x2 = pairStart + 2 * (f - pad) + i;
				if (x2 < clipLeft || x2 >= clipRectTranslated.right)
					continue;
				if (priority >= _screen->getPriority(x2, y2))
					_screen->putPixel(x2, y2, drawMask, getMappedColor(color, scaleSignal, palette, x2, y2), priority, 0);
			}
		}
	}
	_screen->setCurPaletteMapValue(oldSpriteMapValue);

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
						if (origBitmap[cy * celWidth + cx] == clearKey)
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
