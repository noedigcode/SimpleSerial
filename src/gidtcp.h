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

#ifndef GIDTCP_H
#define GIDTCP_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class GidTcp : public QObject
{
    Q_OBJECT
public:
    explicit GidTcp(QObject *parent = 0);
    ~GidTcp();

    class Con {
        friend class GidTcp;
        QString toString();
    private:
        QTcpSocket* socket = nullptr;
        int id = 0;
    };
    typedef QSharedPointer<Con> ConPtr;

    bool setupTcpServer(quint16 port);
    void stopTcpServer();
    bool isServerListening();

    void connectToServer(QHostAddress address, quint16 port);
    void disconnectFromServer();
    bool isConnectedToServer();

    int serverConnectionCount();
    QList<ConPtr> serverConnections();

    static QString ipString(QHostAddress a);

signals:
    void print(QString msg);
    void serverNewConnection(ConPtr con);
    void serverConnectionClosed(ConPtr con);
    void clientConnected();
    void clientConnectionError(QString errorString);
    void clientDisconnected();
    void dataReceived(ConPtr con, QByteArray msg);

public slots:
    void sendMsg(QByteArray msg);
    void sendMsg(ConPtr con, QByteArray msg);
    void sendMsgToAllClients(QByteArray msg);

private:
    QTcpServer tcpServer;
    QList<ConPtr> mServerConnections;
    int socketIdCounter = 0;
    ConPtr client;

private slots:
    void onServerNewTcpConnection();
    void onServerTcpConnectionClosed(ConPtr con);
    void onServerConnectionDataReadyRead(ConPtr con);

    void onClientDataReadyRead();
    void onClientTcpConnectionClosed();

};

#endif // GIDTCP_H
