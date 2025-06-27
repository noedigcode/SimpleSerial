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

    // -------------------------------------------------------------------------
    // Set up command line options

    QCommandLineParser parser;
    parser.addHelpOption();

    QCommandLineOption versionOption({"v", "version"}, "Display version information");
    parser.addOption(versionOption);

    QCommandLineOption serialPortOption({"s", "serial"}, "Serial port to open", "serial");
    parser.addOption(serialPortOption);

    QCommandLineOption baudOption({"b", "baud"}, "Serial buad rate in bps. Default: 9600", "baud");
    parser.addOption(baudOption);

    QMap<QString, QSerialPort::Parity> parities = {
        {"no", QSerialPort::NoParity},
        {"even", QSerialPort::EvenParity},
        {"odd", QSerialPort::OddParity},
        {"space", QSerialPort::SpaceParity},
        {"mark", QSerialPort::MarkParity}
    };
    QString parityOptionText =
            QString("Serial port parity option, one of: %1. Default: no")
            .arg(parities.keys().join(", "));
    QCommandLineOption parityOption("parity", parityOptionText, "parity", "no");
    parser.addOption(parityOption);

    QMap<QString, QSerialPort::DataBits> dataBits = {
        {"5", QSerialPort::Data5},
        {"6", QSerialPort::Data6},
        {"7", QSerialPort::Data7},
        {"8", QSerialPort::Data8}
    };
    QString dataBitsOptionText =
            QString("Serial port data bits option, one of: %1. Default: 8")
            .arg(dataBits.keys().join(", "));
    QCommandLineOption dataBitsOption("databits", dataBitsOptionText, "databits", "8");
    parser.addOption(dataBitsOption);

    QMap<QString, QSerialPort::StopBits> stopBits = {
        {"1", QSerialPort::OneStop},
        {"1.5", QSerialPort::OneAndHalfStop},
        {"2", QSerialPort::TwoStop}
    };
    QString stopBitsOptionText =
            QString("Serial port stop bits option, one of: %1. Default: 1")
            .arg(stopBits.keys().join(", "));
    QCommandLineOption stopBitsOption("stopbits", stopBitsOptionText, "stopbits", "1");
    parser.addOption(stopBitsOption);

    QCommandLineOption sendFileOption(
                "sendfile",
                "Specify the path of a file which will be sent periodically.",
                "sendfile");
    parser.addOption(sendFileOption);

    QCommandLineOption sendFileFreqOption(
                "sendfilefreq",
                "Frequency in milliseconds at which file content will be sent. Default: 500",
                "sendfilefreq", "500");
    parser.addOption(sendFileFreqOption);

    // -------------------------------------------------------------------------
    // Process command line options

    parser.process(a);

    if (parser.isSet(versionOption)) {
        // Version was already printed at start
        return 0;
    }

    MainWindow::StartupOptions mwOptions;

    mwOptions.sendFilePath = parser.value(sendFileOption.valueName());
    QString sendFileFreqOptionValue = parser.value(sendFileFreqOption.valueName());
    bool ok = false;
    int sendFileFreq = sendFileFreqOptionValue.toInt(&ok);
    if (ok && (sendFileFreq > 0)) {
        mwOptions.sendFileFreqMs = sendFileFreq;
    } else {
        print(QString("Invalid value for sendfilefreq: %1, "
                      "expected positive integer value in milliseconds.")
              .arg(sendFileFreqOptionValue));
    }

    // -------------------------------------------------------------------------
    // Process serial port command line options

    QString serialPortOptionValue = parser.value(serialPortOption.valueName());
    if (!serialPortOptionValue.isEmpty()) {
        mwOptions.serialPort = serialPortOptionValue;
    } else {
        print("Serial port name must be specified for serial option.");
    }

    QString baudOptionValue = parser.value(baudOption.valueName());
    int baudRate = baudOptionValue.toInt(&ok);
    if (ok && (baudRate > 0)) {
        mwOptions.baud = baudRate;
    } else {
        print(QString("Invalid value for baud: " + baudOptionValue + ", expected positive integer."));
    }

    QString parityOptionValue = parser.value(parityOption.valueName());
    if (parities.contains(parityOptionValue)) {
        mwOptions.parity = parities.value(parityOptionValue);
    } else {
        print("Invalid value for parity: " + parityOptionValue + ", expected one of: " + parities.keys().join(", "));
    }

    QString dataBitsOptionValue = parser.value(dataBitsOption.valueName());
    if (dataBits.contains(dataBitsOptionValue)) {
        mwOptions.dataBits = dataBits.value(dataBitsOptionValue);
    } else {
        print("Invalid value for data bits: " + dataBitsOptionValue + ", expected one of: " + dataBits.keys().join(", "));
    }

    QString stopBitsOptionValue = parser.value(stopBitsOption.valueName());
    if (stopBits.contains(stopBitsOptionValue)) {
        mwOptions.stopBits = stopBits.value(stopBitsOptionValue);
    } else {
        print("Invalid value for stop bits: " + stopBitsOptionValue + ", expected one of: " + stopBits.keys().join(", "));
    }

    // -------------------------------------------------------------------------
    // Run application

    MainWindow w(mwOptions);
    w.show();

    return a.exec();
}
