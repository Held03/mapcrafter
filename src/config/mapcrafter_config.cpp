/*
 * Copyright 2012, 2013 Moritz Hilscher
 *
 * This file is part of mapcrafter.
 *
 * mapcrafter is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mapcrafter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mapcrafter.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mapcrafter_config.h"

#include "../util.h"
#include "validation.h"

namespace mapcrafter {
namespace config {

bool WorldSection::parse(const ConfigSection& section, const fs::path& config_dir, ValidationList& validation) {
	if (input_dir.load(validation, section, "input_dir")) {
		input_dir.setValue(BOOST_FS_ABSOLUTE(input_dir.getValue(), config_dir));
		if(!fs::is_directory(input_dir.getValue()))
			validation.push_back(ValidationMessage::error("'input_dir' must be an existing directory! '"
					+ input_dir.getValue().string() + "' does not exist!"));
	}

	if (!global) {
		input_dir.require(validation, "You have to specify an input directory ('input_dir')!");
	}

	return isValidationValid(validation);
}

bool MapSection::parse(const ConfigSection& section, const fs::path& config_dir, ValidationList& validation) {
	name_short = section.getName();
	name_long = section.has("name") ? section.get("name") : name_short;

	world.load(validation, section, "world");

	if (texture_dir.load(validation, section, "texture_dir")) {
		texture_dir.setValue(BOOST_FS_ABSOLUTE(texture_dir.getValue(), config_dir));
		if (!fs::is_directory(texture_dir.getValue()))
			validation.push_back(ValidationMessage::error("'texture_dir' must be an existing directory! '"
					+ texture_dir.getValue().string() + "' does not exist!"));
	} else if (!util::findTextureDir().empty())
		texture_dir.setValue(util::findTextureDir());
	else if (!global)
		texture_dir.require(validation, "You have to specify a texture directory ('texture_dir')!");

	if (rotations.load(validation, section, "rotations", "top-left")) {
		std::string str = rotations.getValue();
		std::stringstream ss;
		ss << str;
		std::string elem;
		while (ss >> elem) {
			int r = stringToRotation(elem);
			if (r != -1)
				rotations_set.insert(r);
			else
				validation.push_back(ValidationMessage::error("Invalid rotation '" + elem + "'!"));
		}
	}

	if (rendermode.load(validation, section, "rendermode", "normal")) {
		std::string r = rendermode.getValue();
		if (r != "normal" && r != "daylight" && r != "nightlight" && r != "cave")
			validation.push_back(ValidationMessage::error("'rendermode' must be one of: normal, daylight, nightlight, cave"));
	}

	if (texture_size.load(validation, section, "texture_size", 12))
		if (texture_size.getValue() <= 0 || texture_size.getValue() > 32)
			validation.push_back(ValidationMessage::error("'texture_size' must a number between 1 and 32!"));

	render_unknown_blocks.load(validation, section, "render_unkown_blocks", false);
	render_leaves_transparent.load(validation, section, "render_leaves_transparent", true);
	render_biomes.load(validation, section, "render_biomes", true);
	use_image_timestamps.load(validation, section, "use_image_timestamps", true);

	if (!global) {
		world.require(validation, "You have to specify a world ('world')!");
	}

	return isValidationValid(validation);
}

bool MapcrafterConfigFile::parse(const std::string& filename, ValidationMap& validation) {
	ConfigFile config;
	ValidationMessage msg;
	if (!config.loadFile(filename, msg)) {
		validation.push_back(std::make_pair("Configuration file", makeValidationList(msg)));
		return false;
	}

	fs::path config_dir = fs::path(filename).parent_path();

	bool ok = true;

	ValidationList general_msgs;
	output_dir.load(general_msgs, config.getRootSection(), "output_dir");
	if (output_dir.isLoaded())
		output_dir.setValue(BOOST_FS_ABSOLUTE(output_dir.getValue(), config_dir));
	output_dir.require(general_msgs, "You have to specify an output directory ('output_dir')!");
	if (template_dir.load(general_msgs, config.getRootSection(), "template_dir")) {
		template_dir.setValue(BOOST_FS_ABSOLUTE(template_dir.getValue(), config_dir));
		if (!fs::is_directory(template_dir.getValue()))
			general_msgs.push_back(ValidationMessage::error("'template_dir' must be an existing directory! '"
					+ template_dir.getValue().string() + "' does not exist!"));
	} else if (!util::findTemplateDir().empty())
		template_dir.setValue(util::findTemplateDir());
	else
		template_dir.require(general_msgs, "You have to specify a template directory ('template_dir')!");

	validation.push_back(std::make_pair("Configuration file", general_msgs));

	if (config.hasSection("global", "worlds")) {
		ValidationList msgs;
		ok = world_global.parse(config.getSection("global", "worlds"), config_dir, msgs) && ok;
		if (!msgs.empty())
			validation.push_back(std::make_pair("Global world configuration", msgs));
		if (!ok)
			return false;
	}

	if (config.hasSection("global", "maps")) {
		ValidationList msgs;
		ok = map_global.parse(config.getSection("global", "maps"), config_dir, msgs) && ok;
		if (!msgs.empty())
			validation.push_back(std::make_pair("Global map configuration", msgs));
		if (!ok)
			return false;
	}

	auto sections = config.getSections();

	for (auto it = sections.begin(); it != sections.end(); ++it)
		if (it->getType() != "world" && it->getType() != "map"
				&& it->getNameType() != "global:worlds"
				&& it->getNameType() != "global:maps") {
			validation.push_back(std::make_pair("Section '" + it->getName() + "' with type '" + it->getType() + "'",
					makeValidationList(ValidationMessage::warning("Unknown section type!"))));
		}

	for (auto it = sections.begin(); it != sections.end(); ++it) {
		if (it->getType() != "world")
			continue;
		ValidationList msgs;
		WorldSection world = world_global;
		world.setGlobal(false);
		ok = world.parse(*it, config_dir, msgs) && ok;

		if (hasWorld(it->getName())) {
			msgs.push_back(ValidationMessage::error("World name '" + it->getName() + "' already used!"));
			ok = false;
		} else
			worlds[it->getName()] = world;

		validation.push_back(std::make_pair("World section '" + it->getName() + "'", msgs));
	}

	for (auto it = sections.begin(); it != sections.end(); ++it) {
		if (it->getType() != "map")
			continue;
		ValidationList msgs;
		MapSection map = map_global;
		map.setGlobal(false);
		ok = map.parse(*it, config_dir, msgs) && ok;

		if (hasMap(it->getName())) {
			msgs.push_back(ValidationMessage::error("Map name '" + it->getName() + "' already used!"));
			ok = false;
		} else
			maps.push_back(map);

		validation.push_back(std::make_pair("Map section '" + it->getName() + "'", msgs));
	}

	return ok;
}

std::string rotationsToString(std::set<int> rotations) {
	std::string str;
	for (auto it = rotations.begin(); it != rotations.end(); ++it)
		if (*it >= 0 && *it < 4)
			str += " " + ROTATION_NAMES[*it];
	util::trim(str);
	return str;
}

void dumpWorldSection(std::ostream& out, const WorldSection& section) {
	out << "  input_dir = " << section.getInputDir().string() << std::endl;
}

void dumpMapSection(std::ostream& out, const MapSection& section) {
	out << "  name = " << section.getLongName() << std::endl;
	out << "  world = " << section.getWorld() << std::endl;
	out << "  texture_dir = " << section.getTextureDir() << std::endl;
	out << "  rotations = " << rotationsToString(section.getRotations()) << std::endl;
	out << "  rendermode = " << section.getRendermode() << std::endl;
	out << "  texture_size = " << section.getTextureSize() << std::endl;
	out << "  render_unknown_blocks = " << section.renderUnknownBlocks() << std::endl;
	out << "  render_leaves_transparent = " << section.renderLeavesTransparent() << std::endl;
	out << "  render_biomes = " << section.renderBiomes() << std::endl;
	out << "  use_image_timestamps = " << section.useImageTimestamps() << std::endl;
}

void MapcrafterConfigFile::dump(std::ostream& out) const {
	out << "General:" << std::endl;
	out << "  output_dir = " << output_dir.getValue().string() << std::endl;
	out << "  template_dir = " << template_dir.getValue().string() << std::endl;
	out << std::endl;

	out << "Global world configuration:" << std::endl;
	dumpWorldSection(out, world_global);
	out << std::endl;

	out << "Global map configuration:" << std::endl;
	dumpMapSection(out, map_global);
	out << std::endl;

	for (auto it = worlds.begin(); it != worlds.end(); ++it) {
		out << "World '" << it->first << "':" << std::endl;
		dumpWorldSection(out, it->second);
		out << std::endl;
	}

	for (auto it = maps.begin(); it != maps.end(); ++it) {
		out << "Map '" << it->getShortName() << "':" << std::endl;
		dumpMapSection(out, *it);
		out << std::endl;
	}
}

bool MapcrafterConfigFile::hasMap(const std::string& map) const {
	for (auto it = maps.begin(); it != maps.end(); ++it)
		if (it->getShortName() == map)
			return true;
	return false;
}

const MapSection& MapcrafterConfigFile::getMap(const std::string& map) const {
	for (auto it = maps.begin(); it != maps.end(); ++it)
		if (it->getShortName() == map)
			return *it;
	throw std::out_of_range("Map not found!");
}

MapcrafterConfigHelper::MapcrafterConfigHelper() {
}

MapcrafterConfigHelper::MapcrafterConfigHelper(const MapcrafterConfigFile& config)
	: config(config) {
	auto maps = config.getMaps();
	for (auto it = maps.begin(); it != maps.end(); ++it)
		for (int i = 0; i < 4; i++)
			render_behaviors[it->getShortName()][i] = RENDER_AUTO;

	auto worlds = config.getWorlds();
	for (auto it = worlds.begin(); it != worlds.end(); ++it)
		world_zoomlevels[it->first] = 0;
}

MapcrafterConfigHelper::~MapcrafterConfigHelper() {
}

std::string MapcrafterConfigHelper::generateTemplateJavascript() const {
	std::string js = "";

	auto maps = config.getMaps();
	for (auto it = maps.begin(); it != maps.end(); ++it) {
		std::string world_name = BOOST_FS_FILENAME(config.getWorld(it->getWorld()).getInputDir());

		js += "\"" + it->getShortName() + "\" : {\n";
		js += "\tname: \"" + it->getLongName() + "\",\n";
		js += "\tworldName: \"" + world_name + "\",\n";
		js += "\ttextureSize: " + util::str(it->getTextureSize()) + ",\n";
		js += "\ttileSize: " + util::str(32 * it->getTextureSize()) + ",\n";
		js += "\tmaxZoom: " + util::str(getMapZoomlevel(it->getShortName())) + ",\n";
		js += "\trotations: [";
		auto rotations = it->getRotations();
		for (auto it2 = rotations.begin(); it2 != rotations.end(); ++it2)
			js += util::str(*it2) + ",";
		js += "],\n";
		js += "},";
	}

	return js;
}


const std::set<int>& MapcrafterConfigHelper::getUsedRotations(const std::string& world) const {
	return world_rotations.at(world);
}

void MapcrafterConfigHelper::setUsedRotations(const std::string& world, const std::set<int>& rotations) {
	for (auto it = rotations.begin(); it != rotations.end(); ++it)
		world_rotations[world].insert(*it);
}

int MapcrafterConfigHelper::getWorldZoomlevel(const std::string& world) const {
	return world_zoomlevels.at(world);
}

int MapcrafterConfigHelper::getMapZoomlevel(const std::string& map) const {
	if (!map_zoomlevels.count(map))
		return 0;
	return map_zoomlevels.at(map);
}

void MapcrafterConfigHelper::setWorldZoomlevel(const std::string& world, int zoomlevel) {
	world_zoomlevels[world] = zoomlevel;
}

void MapcrafterConfigHelper::setMapZoomlevel(const std::string& map, int zoomlevel) {
	map_zoomlevels[map] = zoomlevel;
}

int MapcrafterConfigHelper::getRenderBehavior(const std::string& map, int rotation) const {
	return render_behaviors.at(map).at(rotation);
}

void MapcrafterConfigHelper::setRenderBehavior(const std::string& map, int rotation, int behavior) {
	if (rotation == -1)
		for (size_t i = 0; i < 4; i++)
			render_behaviors[map][i] = behavior;
	else
		render_behaviors[map][rotation] = behavior;
}

bool MapcrafterConfigHelper::isCompleteRenderSkip(const std::string& map) const {
	const std::set<int>& rotations = config.getMap(map).getRotations();
	for (auto it = rotations.begin(); it != rotations.end(); ++it)
		if (getRenderBehavior(map, *it) != RENDER_SKIP)
			return false;
	return true;
}

bool MapcrafterConfigHelper::isCompleteRenderForce(const std::string& map) const {
	const std::set<int>& rotations = config.getMap(map).getRotations();
	for (auto it = rotations.begin(); it != rotations.end(); ++it)
		if (getRenderBehavior(map, *it) != RENDER_FORCE)
			return false;
	return true;
}

bool nextRenderBehaviorSplit(std::string& string, std::string& world, std::string& rotation) {
	if (string.empty()) {
		world = "";
		rotation = "";
		return false;
	}

	size_t pos = string.find(",");
	std::string sub = string;
	if (pos != std::string::npos) {
		sub = string.substr(0, pos);
		string = string.substr(pos+1);
	} else {
		string = "";
	}

	world = sub;
	pos = world.find(":");
	if (pos != std::string::npos) {
		rotation = world.substr(pos+1);
		world = world.substr(0, pos);
	} else {
		rotation = "";
	}

	return true;
}

void MapcrafterConfigHelper::setRenderBehaviors(std::string maps, int behavior) {
	std::string map, rotation;

	while (nextRenderBehaviorSplit(maps, map, rotation)) {
		int r = stringToRotation(rotation, ROTATION_NAMES_SHORT);
		if (!config.hasMap(map)) {
			std::cout << "Warning: Unknown map '" << map << "'." << std::endl;
			continue;
		}

		if (!rotation.empty()) {
			if (r == -1) {
				std::cout << "Warning: Unknown rotation '" << rotation << "'." << std::endl;
				continue;
			}
			if (!config.getMap(map).getRotations().count(r)) {
				std::cout << "Warning: Map '" << map << "' does not have rotation '" << rotation << "'." << std::endl;
				continue;
			}
		}

		if (r != -1)
			render_behaviors[map][r] = behavior;
		else
			std::fill(&render_behaviors[map][0], &render_behaviors[map][4], behavior);
	}
}

void MapcrafterConfigHelper::parseRenderBehaviors(bool skip_all, const std::string& render_skip,
		const std::string& render_auto, const std::string& render_force) {
	if (!skip_all)
		setRenderBehaviors(render_skip, RENDER_SKIP);
	else
		for (size_t i = 0; i < config.getMaps().size(); i++)
			for (int j = 0; j < 4; j++)
				render_behaviors[config.getMaps()[i].getShortName()][j] = RENDER_SKIP;
	setRenderBehaviors(render_auto, RENDER_AUTO);
	setRenderBehaviors(render_force, RENDER_FORCE);
}

} /* namespace config */
} /* namespace mapcrafter */