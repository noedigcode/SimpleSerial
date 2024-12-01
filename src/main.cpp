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

#include "mainwindow.h"
#include "version.h"

#include <QApplication>
#include <QCommandLineParser>

#include <iostream>

void print(QString msg)
{
    std::cout << msg.toStdString() << std::endl;
}

void printVersion()
{
    print(QString(APP_NAME));
    print(QString("Version %1").arg(APP_VERSION));
    print(QString("Gideon van der Kolf %1-%2").arg(APP_YEAR_FROM).arg(APP_YEAR));
    print("");
    print("Compiled with Qt " + QString(QT_VERSION_STR));
    print("");
}

int main(int argc, char *argv[])
{
    printVersion();

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication a(argc, argv);
    QApplication::setApplicationName(APP_NAME);
    QApplication::setApplicationVersion(APP_VERSION);

    QCommandLineParser parser;
    parser.addHelpOption();

    QCommandLineOption versionOption({"v", "version"}, "Display version information");
    parser.addOption(versionOption);

    parser.process(a);

    if (parser.isSet(versionOption)) {
        // Version was already printed at start
        return 0;
    }

    MainWindow w;
    w.show();

    return a.exec();
}
