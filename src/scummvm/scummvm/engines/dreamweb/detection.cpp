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

#include "base/plugins.h"

#include "common/algorithm.h"
#include "common/system.h"
#include "common/text-to-speech.h"

#include "engines/advancedDetector.h"

#include "dreamweb/detection.h"
#include "dreamweb/dreamweb.h"

static const PlainGameDescriptor dreamWebGames[] = {
	{ "dreamweb", "DreamWeb" },
	{ nullptr, nullptr }
};

static const DebugChannelDef debugFlagList[] = {
	{DreamWeb::kDebugAnimation, "Animation", "Animation Debug Flag"},
	{DreamWeb::kDebugSaveLoad, "SaveLoad", "Track Save/Load Function"},
	DEBUG_CHANNEL_END
};

#include "dreamweb/detection_tables.h"

/* openfpgaOS launches from a per-game ini and intentionally skips the normal
 * MD5 scan.  DreamWeb still needs a concrete descriptor because the engine
 * uses it for language, CD/floppy mode, and file-name prefixes. */
namespace DreamWeb {
const DreamWebGameDescription *openFPGAFindGameDesc(const Common::String &gameid,
		const Common::String &extra, Common::Platform, Common::Language) {
	if (!gameid.equalsIgnoreCase("dreamweb"))
		return nullptr;

	/* The supplied game files are the English floppy layout.  Keep the CD row
	 * available for future CD packages without making detection mandatory. */
	if (extra.equalsIgnoreCase("CD"))
		return &gameDescriptions[2];
	return &gameDescriptions[1];
}
} // namespace DreamWeb

class DreamWebMetaEngineDetection : public AdvancedMetaEngineDetection<DreamWeb::DreamWebGameDescription> {
public:
	DreamWebMetaEngineDetection():
	AdvancedMetaEngineDetection(DreamWeb::gameDescriptions,
	dreamWebGames) {
		_guiOptions = GUIO5(GUIO_NOMIDI, GAMEOPTION_ORIGINAL_SAVELOAD, GAMEOPTION_BRIGHTPALETTE, GAMEOPTION_TTS_THINGS, GAMEOPTION_TTS_SPEECH);
	}

	const char *getName() const override {
		return "dreamweb";
	}

	const char *getEngineName() const override {
		return "DreamWeb";
	}

	const char *getOriginalCopyright() const override {
		return "DreamWeb (C) Creative Reality";
	}

	const DebugChannelDef *getDebugChannels() const override {
		return debugFlagList;
	}
};

REGISTER_PLUGIN_STATIC(DREAMWEB_DETECTION, PLUGIN_TYPE_ENGINE_DETECTION, DreamWebMetaEngineDetection);
