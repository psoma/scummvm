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

#include "engines/advancedDetector.h"
#include "base/plugins.h"
#include "common/file.h"
#include "common/hashmap.h"
#include "common/ptr.h"
#include "common/translation.h"

#include "sci/detection.h"
#include "sci/dialogs.h"
#include "sci/sci.h"

#include "sci/detection_options.h"

namespace Sci {

enum {
	kAgiDemakeCmd = 'AGID'
};

OptionsWidget::OptionsWidget(GuiObject *boss, const Common::String &name, const Common::String &domain) :
		OptionsContainerWidget(boss, name, "SciGameOptionsDialog", domain) {
	_guiOptions = ConfMan.get("guioptions", domain);

	for (const ADExtraGuiOptionsMap *entry = optionsList; entry->guioFlag; ++entry)
		// "Force AGI sound" also shows wherever the AGI demake option does, so games added to the
		// launcher before it existed get it too
		if (checkGameGUIOption(entry->guioFlag, _guiOptions) ||
				(!strcmp(entry->option.configOption, "agi_sound") && checkGameGUIOption(GAMEOPTION_AGI_DEMAKE, _guiOptions))) {
			// AGI demake / AGI sound checkboxes send a command so dependent boxes can follow them
			const uint32 cmd = (!strcmp(entry->option.configOption, "agi_demake") || !strcmp(entry->option.configOption, "agi_sound")) ? kAgiDemakeCmd : 0;
			_checkboxes[entry->option.configOption] = new GUI::CheckboxWidget(widgetsBoss(), _dialogLayout + "." + entry->option.configOption, _(entry->option.label), _(entry->option.tooltip), cmd);
		}

	for (const PopUpOptionsMap *entry = popUpOptionsList; entry->guioFlag; ++entry)
		if (checkGameGUIOption(entry->guioFlag, _guiOptions)) {
			GUI::StaticTextWidget *textWidget = new GUI::StaticTextWidget(widgetsBoss(), _dialogLayout + "." + entry->configOption + "_desc", _(entry->label), _(entry->tooltip));
			textWidget->setAlign(Graphics::kTextAlignRight);

			_popUps[entry->configOption] = new GUI::PopUpWidget(widgetsBoss(), _dialogLayout + "." + entry->configOption);

			for (uint i = 0; entry->items[i].label; ++i)
				_popUps[entry->configOption]->appendEntry(_(entry->items[i].label), entry->items[i].configValue);
		}
}

void OptionsWidget::defineLayout(GUI::ThemeEval &layouts, const Common::String &layoutName, const Common::String &overlayedLayout) const {
	layouts.addDialog(layoutName, overlayedLayout);
	layouts.addLayout(GUI::ThemeLayout::kLayoutVertical).addPadding(0, 0, 0, 0);

	for (const ADExtraGuiOptionsMap *entry = optionsList; entry->guioFlag; ++entry)
		layouts.addWidget(entry->option.configOption, "Checkbox");

	for (const PopUpOptionsMap *entry = popUpOptionsList; entry->guioFlag; ++entry) {
		layouts.addLayout(GUI::ThemeLayout::kLayoutHorizontal).addPadding(0, 0, 0, 0);
		layouts.addWidget(Common::String(entry->configOption) + "_desc", "OptionsLabel");
		layouts.addWidget(entry->configOption, "PopUp").closeLayout();
	}

	layouts.closeLayout().closeDialog();
}

void OptionsWidget::load() {
	for (const ADExtraGuiOptionsMap *entry = optionsList; entry->guioFlag; ++entry)
		if (_checkboxes.contains(entry->option.configOption))
			_checkboxes[entry->option.configOption]->setState(ConfMan.getBool(entry->option.configOption, _domain));

	for (const PopUpOptionsMap *entry = popUpOptionsList; entry->guioFlag; ++entry)
		if (checkGameGUIOption(entry->guioFlag, _guiOptions))
			_popUps[entry->configOption]->setSelectedTag(ConfMan.getInt(entry->configOption, _domain));

	updateDemakeDependents();

	// If the deprecated native_fb01 option is set, use it to set midi_mode
	if (ConfMan.hasKey("native_fb01", _domain) && ConfMan.getBool("native_fb01", _domain))
		_popUps["midi_mode"]->setSelectedTag(kMidiModeFB01);
}

bool OptionsWidget::save() {
	for (const ADExtraGuiOptionsMap *entry = optionsList; entry->guioFlag; ++entry)
		if (_checkboxes.contains(entry->option.configOption))
			ConfMan.setBool(entry->option.configOption, _checkboxes[entry->option.configOption]->getState(), _domain);

	for (const PopUpOptionsMap *entry = popUpOptionsList; entry->guioFlag; ++entry)
		if (checkGameGUIOption(entry->guioFlag, _guiOptions))
			ConfMan.setInt(entry->configOption, _popUps[entry->configOption]->getSelectedTag(), _domain);

	// Remove deprecated option
	ConfMan.removeKey("native_fb01", _domain);

	return true;
}

// AGI demake always undithers: while it is ticked, the EGA undither box is ticked and greyed out.
// AGI sound never uses digital samples: while it is ticked, that box is unticked and greyed out.
void OptionsWidget::updateDemakeDependents() {
	if (_checkboxes.contains("agi_demake") && _checkboxes.contains("disable_dithering")) {
		const bool demake = _checkboxes["agi_demake"]->getState();
		if (demake)
			_checkboxes["disable_dithering"]->setState(true);
		_checkboxes["disable_dithering"]->setEnabled(!demake);
	}
	// AGI sound is PCjr only, so digital sound effects are switched off while it is ticked
	if (_checkboxes.contains("agi_sound") && _checkboxes.contains("prefer_digitalsfx")) {
		const bool agiSound = _checkboxes["agi_sound"]->getState();
		if (agiSound)
			_checkboxes["prefer_digitalsfx"]->setState(false);
		_checkboxes["prefer_digitalsfx"]->setEnabled(!agiSound);
	}
}

void OptionsWidget::handleCommand(GUI::CommandSender *sender, uint32 cmd, uint32 data) {
	if (cmd == kAgiDemakeCmd) {
		updateDemakeDependents();
		return;
	}
	GUI::OptionsContainerWidget::handleCommand(sender, cmd, data);
}

} // End of namespace Sci
