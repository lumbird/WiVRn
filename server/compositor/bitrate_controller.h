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

#include "wivrn_packets.h"

#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>

namespace wivrn
{

// Lowers the video bitrate when frames start arriving late or incomplete, and raises it back
// slowly once the network keeps up, never above the bitrate requested by the headset.
//
// Uses the feedback the headset already sends for every frame: a frame that never reached the
// decoder counts as lost, and the time from sending to receiving the last packet, compared with
// the best recent time, measures how much the network is queueing.
class bitrate_controller
{
	std::mutex mutex;

	const double min_fraction;
	uint32_t max_bitrate = 0; // requested by the headset, 0 until known
	uint32_t bitrate = 0;

	// statistics for the current window
	int frames = 0;
	int lost = 0;
	int64_t queue_delay_sum = 0;

	// shortest transfer time seen recently, the "empty network" reference
	int64_t base_transfer = std::numeric_limits<int64_t>::max();
	int64_t next_base_transfer = std::numeric_limits<int64_t>::max();
	int windows_since_base_reset = 0;

	int good_windows = 0;

	std::optional<uint32_t> end_window(int64_t frame_duration_ns);

public:
	// min_fraction: lowest bitrate allowed, as a fraction of the requested one
	explicit bitrate_controller(double min_fraction);

	// New bitrate requested by the headset; returns the bitrate to use now.
	uint32_t set_max(uint32_t bitrate_bps);

	// Returns a new bitrate when it should change.
	std::optional<uint32_t> on_feedback(const from_headset::feedback &, int64_t frame_duration_ns);
};

} // namespace wivrn
