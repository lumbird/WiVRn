/*
 * WiVRn VR streaming
 * Copyright (C) 2024  galister <galister@librevr.org>
 * Copyright (C) 2025  Patrick Nicolas <patricknicolas@laposte.net>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "picture_adjustments.h"

#include "util/u_logging.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>

namespace wivrn
{

std::filesystem::path picture_adjustments::config_path()
{
	if (const char * xdg = std::getenv("XDG_CONFIG_HOME"); xdg and *xdg)
		return std::filesystem::path(xdg) / "wivrn" / "picture.conf";
	if (const char * home = std::getenv("HOME"))
		return std::filesystem::path(home) / ".config" / "wivrn" / "picture.conf";
	return {};
}

std::mutex picture_adjustments::mutex;
picture_adjustments::state picture_adjustments::current_;
bool picture_adjustments::initialised = false;

namespace
{
picture_adjustments::values clamped(picture_adjustments::values v)
{
	auto c = [](float & x, float lo, float hi, float fallback) { x = std::isfinite(x) ? std::clamp(x, lo, hi) : fallback; };
	c(v.exposure, 0.25, 4, 1);
	c(v.gamma, 0.5, 2.5, 1);
	c(v.contrast, 0.5, 2, 1);
	c(v.saturation, 0, 2, 1);
	c(v.vibrance, -1, 1, 0);
	c(v.black_level, 0, 0.3, 0);
	c(v.white_level, 0.5, 1, 1);
	c(v.red_gain, 0.25, 2, 1);
	c(v.green_gain, 0.25, 2, 1);
	c(v.blue_gain, 0.25, 2, 1);
	return v;
}
} // namespace

picture_adjustments::state picture_adjustments::current()
{
	std::lock_guard lock(mutex);
	if (not initialised)
	{
		current_ = load_file();
		initialised = true;
	}
	return current_;
}

void picture_adjustments::set(const state & s)
{
	std::lock_guard lock(mutex);
	current_ = {.enabled = s.enabled, .v = clamped(s.v)};
	initialised = true;
}

picture_adjustments::state picture_adjustments::load_file()
{
	auto path = config_path();
	std::map<std::string, std::string> kv;
	std::ifstream file(path);
	for (std::string line; std::getline(file, line);)
	{
		if (auto hash = line.find('#'); hash != std::string::npos)
			line.erase(hash);
		std::erase_if(line, [](unsigned char c) { return std::isspace(c); });
		auto eq = line.find('=');
		if (eq == std::string::npos or eq == 0)
			continue;
		std::string key = line.substr(0, eq);
		std::ranges::transform(key, key.begin(), [](unsigned char c) { return std::tolower(c); });
		kv[key] = line.substr(eq + 1);
	}

	auto flag = [&](const char * key) {
		auto it = kv.find(key);
		if (it == kv.end())
			return false;
		std::string v = it->second;
		std::ranges::transform(v, v.begin(), [](unsigned char c) { return std::tolower(c); });
		return v == "true" or v == "1" or v == "on" or v == "yes";
	};
	auto number = [&](const char * key, float & out, float lo, float hi) {
		if (auto it = kv.find(key); it != kv.end())
		{
			char * end = nullptr;
			float v = std::strtof(it->second.c_str(), &end);
			if (end != it->second.c_str() and *end == '\0' and std::isfinite(v))
				out = std::clamp(v, lo, hi);
		}
	};

	values v;
	number("exposure", v.exposure, 0.25, 4);
	number("brightness", v.exposure, 0.25, 4);
	number("gamma", v.gamma, 0.5, 2.5);
	number("contrast", v.contrast, 0.5, 2);
	number("saturation", v.saturation, 0, 2);
	number("vibrance", v.vibrance, -1, 1);
	number("black_level", v.black_level, 0, 0.3);
	number("white_level", v.white_level, 0.5, 1);
	number("red", v.red_gain, 0.5, 1.5);
	number("green", v.green_gain, 0.5, 1.5);
	number("blue", v.blue_gain, 0.5, 1.5);
	float temperature = 0;
	number("temperature", temperature, -1, 1);
	v.red_gain *= 1 + 0.15f * temperature;
	v.blue_gain *= 1 - 0.15f * temperature;

	state result{.enabled = flag("enabled"), .v = clamped(v)};
	U_LOG_I("Picture adjustments from %s: %s, brightness %.2f, gamma %.2f, contrast %.2f, saturation %.2f",
	        path.c_str(),
	        result.enabled ? "on" : "off",
	        v.exposure,
	        v.gamma,
	        v.contrast,
	        v.saturation);
	return result;
}

} // namespace wivrn
