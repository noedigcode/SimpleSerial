/******************************************************************************
 *
 * This file is part of SimpleSerial.
 * Copyright (C) 2024 Gideon van der Kolf
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
 *****************************************************************************/

#include "Utilities.h"

#include <QFontDatabase>
#include <QFontInfo>
#include <QGuiApplication>
#include <QScreen>


QFont Utilities::getMonospaceFont()
{
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    if (!QFontInfo(font).fixedPitch()) {
        // Try backup method
        QStringList families({"monospace", "consolas", "courier new", "courier"});
        foreach (QString family, families) {
            font.setFamily(family);
            if (QFontInfo(font).fixedPitch()) { break; }
        }
    }
    return font;
}

QSize Utilities::scaleWithPrimaryScreenScalingFactor(QSize size)
{
    static const qreal baselineDpi = 96.0;

    qreal scalingFactor = QGuiApplication::primaryScreen()->logicalDotsPerInch()
                          / baselineDpi;
    return size * scalingFactor;
}
