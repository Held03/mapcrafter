/*
 * Copyright 2012-2016 Moritz Hilscher
 *
 * This file is part of Mapcrafter.
 *
 * Mapcrafter is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mapcrafter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mapcrafter.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SECTIONS_OVERLAY_H_
#define SECTIONS_OVERLAY_H_

#include "../configsection.h"
#include "../validation.h"
#include "../../renderer/rendermode.h"

#include <string>

namespace mapcrafter {
namespace config {

class OverlaySection : public ConfigSection {
public:
	OverlaySection();
	virtual ~OverlaySection();

	virtual std::string getPrettyName() const;
	virtual void dump(std::ostream& out) const;

	std::string getID() const;
	std::string getName() const;
	renderer::OverlayType getType() const;
	bool isBase() const;

protected:
	virtual void preParse(const INIConfigSection& section,
			ValidationList& validation);
	virtual bool parseField(const std::string key, const std::string value,
			ValidationList& validation);
	virtual void postParse(const INIConfigSection& section,
			ValidationList& validation);

private:
	Field<std::string> name;
	Field<bool> base;

	Field<bool> day;
	Field<double> lighting_intensity, lighting_water_intensity;
};

class DummyOverlaySection : public OverlaySection {
};

class HeightOverlaySection : public OverlaySection {
public:
	virtual void dump(std::ostream& out) const;

protected:
	virtual void preParse(const INIConfigSection& section,
			ValidationList& validation);
	virtual bool parseField(const std::string key, const std::string value,
			ValidationList& validation);

private:
	// TODO colors
};

class LightingOverlaySection : public OverlaySection {
public:
	virtual void dump(std::ostream& out) const;

	bool isDay() const;
	double getIntensity() const;
	double getWaterIntensity() const;

protected:
	virtual void preParse(const INIConfigSection& section,
			ValidationList& validation);
	virtual bool parseField(const std::string key, const std::string value,
			ValidationList& validation);

private:
	Field<bool> day;
	Field<double> intensity, water_intensity;
};

class SlimeOverlaySection : public OverlaySection {
public:
	virtual void dump(std::ostream& out) const;

protected:
	virtual void preParse(const INIConfigSection& section,
			ValidationList& validation);
	virtual bool parseField(const std::string key, const std::string value,
			ValidationList& validation);
};

class SpawnOverlaySection : public OverlaySection {
public:
	virtual void dump(std::ostream& out) const;

	bool isDay() const;

protected:
	virtual void preParse(const INIConfigSection& section,
			ValidationList& validation);
	virtual bool parseField(const std::string key, const std::string value,
			ValidationList& validation);

private:
	Field<bool> day;
};

} /* namespace config */
} /* namespace mapcrafter */

#endif /* SECTIONS_OVERLAY_H_ */