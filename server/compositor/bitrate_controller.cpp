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

#include "bitrate_controller.h"

#include "util/u_logging.h"
#include "util/u_time.h"

#include <algorithm>

namespace wivrn
{

namespace
{
// A window is one second of frames.
constexpr int64_t window_ns = U_TIME_1S_IN_NS;
// Decrease when more than this fraction of frames is lost...
constexpr double max_loss = 0.02;
// ...or when frames queue for more than this fraction of a frame on average.
constexpr double max_queue_delay = 0.5;
constexpr double decrease = 0.85;
constexpr double increase = 1.05;
// Clean windows needed before increasing.
constexpr int windows_before_increase = 3;
// The reference transfer time is refreshed this often, to follow clock drift and route changes.
constexpr int base_reset_windows = 10;
} // namespace

bitrate_controller::bitrate_controller(double min_fraction) :
        min_fraction(std::clamp(min_fraction, 0.05, 1.0))
{}

uint32_t bitrate_controller::set_max(uint32_t bitrate_bps)
{
	std::lock_guard lock(mutex);
	max_bitrate = bitrate_bps;
	bitrate = bitrate == 0 ? bitrate_bps : std::min(bitrate, bitrate_bps);
	return bitrate;
}

std::optional<uint32_t> bitrate_controller::on_feedback(const from_headset::feedback & feedback, int64_t frame_duration_ns)
{
	// Only the main stream.
	if (feedback.stream_index != 0 or frame_duration_ns <= 0)
		return std::nullopt;

	std::lock_guard lock(mutex);
	if (max_bitrate == 0 or (last_frame and feedback.frame_index <= last_frame))
		return std::nullopt;
	last_frame = feedback.frame_index;

	++frames;
	if (not feedback.sent_to_decoder or not feedback.received_last_packet)
		++lost;
	else
	{
		int64_t transfer = feedback.received_last_packet - feedback.send_begin;
		base_transfer = std::min(base_transfer, transfer);
		next_base_transfer = std::min(next_base_transfer, transfer);
		queue_delay_sum += transfer - base_transfer;
	}

	if (frames * frame_duration_ns < window_ns)
		return std::nullopt;
	return end_window(frame_duration_ns);
}

std::optional<uint32_t> bitrate_controller::end_window(int64_t frame_duration_ns)
{
	int received = frames - lost;
	double loss = double(lost) / frames;
	double queue_delay = received ? double(queue_delay_sum) / received : 0;

	frames = lost = 0;
	queue_delay_sum = 0;
	if (++windows_since_base_reset >= base_reset_windows)
	{
		base_transfer = next_base_transfer;
		next_base_transfer = std::numeric_limits<int64_t>::max();
		windows_since_base_reset = 0;
	}

	uint32_t min_bitrate = max_bitrate * min_fraction;
	uint32_t old = bitrate;
	if (loss > max_loss or queue_delay > max_queue_delay * frame_duration_ns)
	{
		good_windows = 0;
		bitrate = std::max<uint32_t>(bitrate * decrease, min_bitrate);
	}
	else if (++good_windows >= windows_before_increase)
	{
		good_windows = 0;
		bitrate = std::min<uint32_t>(bitrate * increase, max_bitrate);
	}

	if (bitrate == old)
		return std::nullopt;
	U_LOG_I("Adaptive bitrate: %.1f Mbit/s (%.1f%% lost, %.1fms queueing)",
	        bitrate * 1e-6,
	        loss * 100,
	        queue_delay * 1e-6);
	return bitrate;
}

} // namespace wivrn
