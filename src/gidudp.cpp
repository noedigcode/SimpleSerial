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

#include "gidudp.h"

GidUdp::GidUdp(QObject *parent) :
    QObject(parent)
{
    connect(&udpSocket, &QUdpSocket::readyRead,
                  this, &GidUdp::udpSocketReadyRead);
}

bool GidUdp::setupUdp(int port)
{
    udpPort = port;

    print("Setting up UDP");

    if (udpSocket.bind(udpPort, QUdpSocket::ShareAddress
                                | QUdpSocket::ReuseAddressHint)) {
        print(QString("UDP socket bound to port %1").arg(udpPort));
        return true;
    } else {
        print(QString("Failed to bind UDP to port %1").arg(udpPort));
        print("Error string: " + udpSocket.errorString());
        return false;
    }
}

void GidUdp::stopUdp()
{
    udpSocket.disconnectFromHost();
}

void GidUdp::udpSocketReadyRead()
{
    while (udpSocket.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket.pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        udpSocket.readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        emit rxMessage(datagram, sender, senderPort);
    }
}

void GidUdp::sendMessage(const QByteArray& msg, const QHostAddress& address, quint16 port)
{
    udpSocket.writeDatagram(msg, address, port);
}

