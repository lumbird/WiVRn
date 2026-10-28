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

#include "picture_settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>
#include <cmath>

namespace
{
QString config_path()
{
	return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/wivrn/picture.conf";
}
} // namespace

PictureSettings::PictureSettings(QObject * parent) :
        QObject(parent)
{
	load();
	connect(qApp, &QCoreApplication::aboutToQuit, this, &PictureSettings::save);
}

PictureSettings::~PictureSettings()
{
	save();
}

QList<double> PictureSettings::values() const
{
	return {
	        m_brightness,
	        m_gamma,
	        m_contrast,
	        m_saturation,
	        m_vibrance,
	        m_black_level,
	        m_white_level,
	        m_red * (1 + 0.15 * m_temperature),
	        m_green,
	        m_blue * (1 - 0.15 * m_temperature),
	};
}

void PictureSettings::load()
{
	QFile file(config_path());
	if (not file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;
	QTextStream in(&file);
	while (not in.atEnd())
	{
		QString line = in.readLine();
		line = line.left(line.indexOf('#') < 0 ? line.size() : line.indexOf('#')).trimmed();
		auto eq = line.indexOf('=');
		if (eq <= 0)
			continue;
		QString key = line.left(eq).trimmed().toLower();
		QString value = line.mid(eq + 1).trimmed();
		bool ok = false;
		double number = value.toDouble(&ok);
		if (key == "enabled")
			m_enabled = QStringList{"true", "1", "on", "yes"}.contains(value.toLower());
		else if (not ok or not std::isfinite(number))
			continue;
		else if (key == "brightness" or key == "exposure")
			m_brightness = number;
		else if (key == "gamma")
			m_gamma = number;
		else if (key == "contrast")
			m_contrast = number;
		else if (key == "saturation")
			m_saturation = number;
		else if (key == "vibrance")
			m_vibrance = number;
		else if (key == "black_level")
			m_black_level = number;
		else if (key == "white_level")
			m_white_level = number;
		else if (key == "temperature")
			m_temperature = number;
		else if (key == "red")
			m_red = number;
		else if (key == "green")
			m_green = number;
		else if (key == "blue")
			m_blue = number;
	}
}

void PictureSettings::save()
{
	if (not dirty)
		return;
	dirty = false;
	QString path = config_path();
	QDir().mkpath(QFileInfo(path).absolutePath());
	QSaveFile file(path);
	if (not file.open(QIODevice::WriteOnly | QIODevice::Text))
		return;
	QTextStream out(&file);
	auto n = [](double v) { return QString::number(v, 'f', 2); };
	out << "# WiVRn picture adjustments. Written by the dashboard; applied live by the server.\n"
	    << "# enabled = false gives the stock picture with no extra work.\n"
	    << "enabled = " << (m_enabled ? "true" : "false") << "\n"
	    << "brightness = " << n(m_brightness) << "\n"
	    << "gamma = " << n(m_gamma) << "\n"
	    << "contrast = " << n(m_contrast) << "\n"
	    << "saturation = " << n(m_saturation) << "\n"
	    << "vibrance = " << n(m_vibrance) << "\n"
	    << "black_level = " << n(m_black_level) << "\n"
	    << "white_level = " << n(m_white_level) << "\n"
	    << "temperature = " << n(m_temperature) << "\n"
	    << "red = " << n(m_red) << "\n"
	    << "green = " << n(m_green) << "\n"
	    << "blue = " << n(m_blue) << "\n";
	file.commit();
}

template <typename T>
void PictureSettings::set(T & member, T value)
{
	if (member == value)
		return;
	member = value;
	dirty = true;
	Q_EMIT changed();
}

void PictureSettings::set_enabled(bool v)
{
	set(m_enabled, v);
}
void PictureSettings::set_brightness(double v)
{
	set(m_brightness, v);
}
void PictureSettings::set_gamma(double v)
{
	set(m_gamma, v);
}
void PictureSettings::set_contrast(double v)
{
	set(m_contrast, v);
}
void PictureSettings::set_saturation(double v)
{
	set(m_saturation, v);
}
void PictureSettings::set_vibrance(double v)
{
	set(m_vibrance, v);
}
void PictureSettings::set_blackLevel(double v)
{
	set(m_black_level, v);
}
void PictureSettings::set_whiteLevel(double v)
{
	set(m_white_level, v);
}
void PictureSettings::set_temperature(double v)
{
	set(m_temperature, v);
}

void PictureSettings::reset()
{
	m_brightness = m_gamma = m_contrast = m_saturation = m_white_level = m_red = m_green = m_blue = 1;
	m_vibrance = m_black_level = m_temperature = 0;
	dirty = true;
	Q_EMIT changed();
}
