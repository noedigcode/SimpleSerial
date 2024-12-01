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

#include "gidtcp.h"

GidTcp::GidTcp(QObject *parent) :
    QObject(parent)
{
    connect(&tcpServer, &QTcpServer::newConnection,
                  this, &GidTcp::onServerNewTcpConnection);
}

GidTcp::~GidTcp()
{

}

bool GidTcp::setupTcpServer(quint16 port)
{
    bool success = false;
    if ( tcpServer.listen(QHostAddress::Any, port) ) {
        print(QString("TCP Server listening on port: %1").arg(port));
        success = true;
    } else {
        print("ERROR: TCP Server failed to start listening: "
              + tcpServer.errorString());
        success = false;
    }
    return success;
}

void GidTcp::stopTcpServer()
{
    foreach (ConPtr con, mServerConnections) {
        con->socket->close();
    }

    tcpServer.close();
}

bool GidTcp::isServerListening()
{
    return tcpServer.isListening();
}

void GidTcp::connectToServer(QHostAddress address, quint16 port)
{
    disconnectFromServer();

    client.reset(new Con());
    client->socket = new QTcpSocket();

    connect(client->socket, &QTcpSocket::connected,
            this, &GidTcp::clientConnected);

    connect(client->socket,
            static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
            this, [=](QAbstractSocket::SocketError /*e*/)
    {
        emit clientConnectionError(client->socket->errorString());
    });

    connect(client->socket, &QTcpSocket::disconnected,
            this, &GidTcp::onClientTcpConnectionClosed);

    connect(client->socket, &QTcpSocket::readyRead,
            this, &GidTcp::onClientDataReadyRead);

    print(QString("Connecting to server %1:%2").arg(ipString(address)).arg(port));
    client->socket->connectToHost(address, port);
}

void GidTcp::disconnectFromServer()
{
    if (!client) { return; }
    if (client->socket->isOpen()) {
        client->socket->blockSignals(true);
        client->socket->disconnectFromHost();
        client->socket->waitForDisconnected(1000);
        client->socket->deleteLater();
    }
    client.reset();
}

bool GidTcp::isConnectedToServer()
{
    bool ret = false;
    if (client) {
        if (client->socket) {
            if (client->socket->isOpen()) {
                ret = true;
            }
        }
    }
    return ret;
}

QList<GidTcp::ConPtr> GidTcp::serverConnections()
{
    return mServerConnections;
}

QString GidTcp::ipString(QHostAddress a)
{
    static QString toRemove = "::ffff:";
    QString ret = a.toString();
    if (ret.startsWith(toRemove)) {
        ret.remove(0, toRemove.length());
    }
    return ret;
}

void GidTcp::onServerNewTcpConnection()
{
    while (tcpServer.hasPendingConnections()) {

        ConPtr con(new Con());
        con->socket = tcpServer.nextPendingConnection();
        con->id = socketIdCounter++;
        mServerConnections.append(con);

        // Weak pointer to prevent memory leak with lambda
        QWeakPointer<Con> conWPtr(con);
        connect(con->socket, &QTcpSocket::disconnected,
                this, [this, conWPtr]()
        {
            ConPtr con(conWPtr);
            if (!con) { return; }
            onServerTcpConnectionClosed(con);
        });

        connect(con->socket, &QTcpSocket::readyRead,
                this, [this, conWPtr]()
        {
            ConPtr con(conWPtr);
            if (!con) { return; }
            onServerConnectionDataReadyRead(con);
        });

        print(QString("New connection: " + con->toString()));
        emit serverNewConnection(con);

    }
}

void GidTcp::onServerTcpConnectionClosed(ConPtr con)
{
    // The TCP connection has been closed. Emit the deleteLater() signal so the
    // socket will be deleted later (when we exit this slot).

    mServerConnections.removeAll(con);
    con->socket->deleteLater();

    print(QString("Connection closed: " + con->toString()));
    emit serverConnectionClosed(con);
}

void GidTcp::onServerConnectionDataReadyRead(ConPtr con)
{
    QByteArray data = con->socket->readAll();
    emit dataReceived(con, data);
}

void GidTcp::onClientDataReadyRead()
{
    if (client && client->socket) {
        QByteArray data = client->socket->readAll();
        emit dataReceived(client, data);
    }
}

void GidTcp::onClientTcpConnectionClosed()
{
    client->socket->deleteLater();
    client.reset();
    emit clientDisconnected();
}

void GidTcp::sendMsg(QByteArray msg)
{
    if (client && client->socket) {
        client->socket->write(msg);
    }
}

void GidTcp::sendMsg(ConPtr con, QByteArray msg)
{
    if (!con) {
        print("ERROR: sendMsg: null connection");
        return;
    }
    if (!con->socket) {
        print("ERROR: sendMsg: null connection socket");
        return;
    }
    con->socket->write(msg);
}

void GidTcp::sendMsgToAllClients(QByteArray msg)
{
    foreach (ConPtr con, mServerConnections) {
        con->socket->write(msg);
    }
}

QString GidTcp::Con::toString()
{
    QString s = QString("id=%1").arg(id);
    if (!socket) {
        s += " (invalid)";
    } else {
        s += QString(" %1:%2")
                .arg(GidTcp::ipString(socket->peerAddress()))
                .arg(socket->peerPort());
    }
    return s;
}
