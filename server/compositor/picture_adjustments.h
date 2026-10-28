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

#pragma once

#include <filesystem>
#include <mutex>

namespace wivrn
{

// Colour adjustments applied while converting frames for the encoder.
// The dashboard sends changes live over D-Bus (SetPicture); the file ~/.config/wivrn/picture.conf
// only provides the starting values when the server starts. `values` matches the push constants
// in foveation.comp.
class picture_adjustments
{
public:
	struct values
	{
		float exposure = 1;
		float gamma = 1;
		float contrast = 1;
		float saturation = 1;
		float vibrance = 0;
		float black_level = 0;
		float white_level = 1;
		float red_gain = 1;
		float green_gain = 1;
		float blue_gain = 1;
	};

	struct state
	{
		bool enabled = false;
		values v;
	};

	// Called by the compositor every frame; cheap.
	static state current();
	// Called when the dashboard changes a setting. Values are clamped to safe ranges.
	static void set(const state &);

	static std::filesystem::path config_path();
	static state load_file();

private:
	static std::mutex mutex;
	static state current_;
	static bool initialised;
};

} // namespace wivrn
