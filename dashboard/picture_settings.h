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

#include <QObject>
#include <qqmlintegration.h>

// Picture adjustments for the stream. Changes are sent live to the server over D-Bus (see SettingsPage.qml);
// ~/.config/wivrn/picture.conf is read when the dashboard starts and written when it quits, and the
// server reads it once at start-up.
class PictureSettings : public QObject
{
	Q_OBJECT
	QML_NAMED_ELEMENT(PictureSettings)
	QML_SINGLETON

	Q_PROPERTY(bool enabled MEMBER m_enabled WRITE set_enabled NOTIFY changed)
	Q_PROPERTY(double brightness MEMBER m_brightness WRITE set_brightness NOTIFY changed)
	Q_PROPERTY(double gamma MEMBER m_gamma WRITE set_gamma NOTIFY changed)
	Q_PROPERTY(double contrast MEMBER m_contrast WRITE set_contrast NOTIFY changed)
	Q_PROPERTY(double saturation MEMBER m_saturation WRITE set_saturation NOTIFY changed)
	Q_PROPERTY(double vibrance MEMBER m_vibrance WRITE set_vibrance NOTIFY changed)
	Q_PROPERTY(double blackLevel MEMBER m_black_level WRITE set_blackLevel NOTIFY changed)
	Q_PROPERTY(double whiteLevel MEMBER m_white_level WRITE set_whiteLevel NOTIFY changed)
	Q_PROPERTY(double temperature MEMBER m_temperature WRITE set_temperature NOTIFY changed)

	bool m_enabled = false;
	double m_brightness = 1;
	double m_gamma = 1;
	double m_contrast = 1;
	double m_saturation = 1;
	double m_vibrance = 0;
	double m_black_level = 0;
	double m_white_level = 1;
	double m_temperature = 0;
	// Not on the page, but kept when saving so hand-edited values survive.
	double m_red = 1;
	double m_green = 1;
	double m_blue = 1;

	bool dirty = false;

	void load();
	void save();
	template <typename T>
	void set(T & member, T value);

public:
	explicit PictureSettings(QObject * parent = nullptr);
	~PictureSettings() override;

	// brightness, gamma, contrast, saturation, vibrance, black level, white level, red, green, blue
	// (the order the server's SetPicture expects), with colour temperature folded into red and blue.
	Q_INVOKABLE QList<double> values() const;

	void set_enabled(bool v);
	void set_brightness(double v);
	void set_gamma(double v);
	void set_contrast(double v);
	void set_saturation(double v);
	void set_vibrance(double v);
	void set_blackLevel(double v);
	void set_whiteLevel(double v);
	void set_temperature(double v);

	// Back to stock values (the switch stays as it is).
	Q_INVOKABLE void reset();

Q_SIGNALS:
	void changed();
};
