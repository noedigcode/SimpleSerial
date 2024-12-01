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

#ifndef GIDUDP_H
#define GIDUDP_H

#include <QObject>
#include <QUdpSocket>

class GidUdp : public QObject
{
    Q_OBJECT
public:
    explicit GidUdp(QObject *parent = 0);
    bool setupUdp(int port);
    void stopUdp();

private:
    QUdpSocket udpSocket;
    int udpPort;

private slots:
    void udpSocketReadyRead();

signals:
    void print(QString msg);
    void rxMessage(QByteArray msg, QHostAddress address, quint16 port);

public slots:
    void sendMessage(const QByteArray& msg, const QHostAddress& address, quint16 port);
};

#endif // GIDUDP_H
