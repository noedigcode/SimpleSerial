#include "comms.h"



Comms::Comms(QObject *parent) : QObject(parent)
{

}

void Comms::setTag(QString tag)
{
    mTag = tag;
}

QString Comms::tag()
{
    return mTag;
}

void Comms::send(const QByteArray &data)
{
    mTxByteCount += data.length();
    doSend(data);
}

void Comms::onReceive(QByteArray data)
{
    mRxByteCount += data.length();
    emit dataReceived(data);
}

int Comms::rxByteCount()
{
    return mRxByteCount;
}

int Comms::txByteCount()
{
    return mTxByteCount;
}

void Comms::clearCounters()
{
    mRxByteCount = 0;
    mTxByteCount = 0;
}

SerialComms::SerialComms(QObject *parent) : Comms(parent)
{
    setupSerial();
}

QString SerialComms::type()
{
    return "Serial Port";
}

QString SerialComms::titleText()
{
    QString title;
    if (serial.s.isOpen()) {
        title = QString("%1 (%2)")
                .arg(serial.s.portName())
                .arg(serial.s.baudRate());
    } else {
        title = QString("%1 (Closed)")
                .arg(serial.s.portName());
    }
    return title;
}

void SerialComms::close()
{
    if (serial.s.isOpen()) {
        serial.s.close();
        emit print("Serial port closed.");
        emit closed();
    }
}

void SerialComms::setSettings(const QMap<QString, QString> &keyVals)
{
    serial.setSettings(keyVals);
}

QMap<QString, QString> SerialComms::getSettings()
{
    return serial.getSettings();
}

bool SerialComms::wasOpenBefore()
{
    return mWasOpenBefore;
}

void SerialComms::reOpen()
{
    serial.reOpen();
}

void SerialComms::setupSerial()
{
    connect(&(serial.s), &QSerialPort::readyRead,
            this, &SerialComms::onSerialReadyRead);
    connect(&(serial.s), &QSerialPort::errorOccurred,
            this, &SerialComms::onSerialError);
    connect(&serial, &GidQt5Serial::print,
            this, &SerialComms::print);
    connect(&serial, &GidQt5Serial::portOpened,
            this, &SerialComms::onPortOpened);
}

void SerialComms::doSend(const QByteArray &data)
{
    serial.s.write(data);
    mTxByteCount += data.length();
}

void SerialComms::onPortOpened()
{
    mWasOpenBefore = true;
    emit Comms::opened();
}

void SerialComms::onSerialReadyRead()
{
    Comms::onReceive(serial.s.readAll());
}

void SerialComms::onSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) { return; }
    QString s = QVariant::fromValue(error).toString();
    emit print("Serial port error: " + s);
    emit Comms::errorOccurred(s);
}

TcpServerComms::TcpServerComms(QObject *parent) : Comms(parent)
{
    setupTcp();
}

QString TcpServerComms::type()
{
    return "TCP Server";
}

QString TcpServerComms::titleText()
{
    QString title = QString("TCP Server (%1)")
            .arg(mPort);
    if (!tcp.isServerListening()) {
        title += " (Closed)";
    }
    return title;
}

void TcpServerComms::close()
{
    if (tcp.isServerListening()) {
        tcp.stopTcpServer();
        print("TCP server stopped.");
        emit Comms::closed();
    }
}

bool TcpServerComms::startTcpServer(quint16 port)
{
    mPort = port;
    bool ok = tcp.setupTcpServer(port);
    if (ok) {
        emit Comms::opened();
    } else {
        emit Comms::errorOccurred(tcp.errorString());
    }
    return ok;
}

bool TcpServerComms::restart()
{
    return startTcpServer(mPort);
}

void TcpServerComms::setupTcp()
{
    connect(&tcp, &GidTcp::print, this, &Comms::print);
    connect(&tcp, &GidTcp::dataReceived,
            this, &TcpServerComms::onTcpDataReceived);
}

void TcpServerComms::doSend(const QByteArray &data)
{
    tcp.sendMsgToAllClients(data);
}

void TcpServerComms::onTcpDataReceived(GidTcp::ConPtr /*con*/, QByteArray msg)
{
    Comms::onReceive(msg);
}

TcpClientComms::TcpClientComms(QObject *parent) : Comms(parent)
{
    setupTcp();
}

QString TcpClientComms::type()
{
    return "TCP Client";
}

QString TcpClientComms::titleText()
{
    QString title = QString("TCP Client (%1:%2)")
            .arg(mIpAddress)
            .arg(mPort);
    if (!tcp.isConnectedToServer()) {
        title += " (Closed)";
    }
    return title;
}

void TcpClientComms::close()
{
    tcp.disconnectFromServer();
    print("Disconnected from TCP server.");
}

void TcpClientComms::connectToServer(QHostAddress address, quint16 port)
{
    mIpAddress = address.toString();
    mPort = port;
    tcp.connectToServer(address, port);
}

void TcpClientComms::reConnect()
{
    connectToServer(QHostAddress(mIpAddress), mPort);
}

void TcpClientComms::setupTcp()
{
    connect(&tcp, &GidTcp::print, this, &Comms::print);
    connect(&tcp, &GidTcp::dataReceived,
            this, &TcpClientComms::onTcpDataReceived);
    connect(&tcp, &GidTcp::clientConnected,
            this, &TcpClientComms::onTcpClientConnectedToServer);
    connect(&tcp, &GidTcp::clientDisconnected,
            this, &TcpClientComms::onTcpClientDisconnected);
    connect(&tcp, &GidTcp::clientConnectionError,
            this, &TcpClientComms::onTcpClientError);
}

void TcpClientComms::doSend(const QByteArray &data)
{
    tcp.sendMsg(data);
}

void TcpClientComms::onTcpDataReceived(GidTcp::ConPtr /*con*/, QByteArray data)
{
    Comms::onReceive(data);
}

void TcpClientComms::onTcpClientConnectedToServer()
{
    print("Connected to TCP server.");
    emit Comms::opened();
}

void TcpClientComms::onTcpClientError(QString errorString)
{
    print("TCP client error: " + errorString);
    emit Comms::errorOccurred(errorString);
}

void TcpClientComms::onTcpClientDisconnected()
{
    print("Disconnected from TCP server.");
    emit Comms::closed();
}

UdpComms::UdpComms(QObject *parent) : Comms(parent)
{
    setupUdp();
}

QString UdpComms::type()
{
    return "UDP";
}

QString UdpComms::titleText()
{
    QString title = "UDP";
    if (mListen) {
        title += QString(" (%1)").arg(mListenPort);
    }
    return title;
}

void UdpComms::close()
{
    udp.stopUdp();
    emit Comms::closed();
}

void UdpComms::start(bool listen, quint16 listenPort, bool broadcast, QString sendIp, quint16 sendPort)
{
    mUdpSendBroadcast = broadcast;
    mUdpSendIp = sendIp;
    mUdpSendPort = sendPort;
    mListen = listen;
    mListenPort = listenPort;
    if (listen) {
        udp.setupUdp(listenPort);
    }
}

void UdpComms::restart()
{
    start(mListen, mListenPort, mUdpSendBroadcast, mUdpSendIp, mUdpSendPort);
}

void UdpComms::setupUdp()
{
    connect(&udp, &GidUdp::print, this, &Comms::print);
    connect(&udp, &GidUdp::rxMessage, this, &UdpComms::onUdpDataReceived);
}

void UdpComms::doSend(const QByteArray &data)
{
    QHostAddress a;
    if (mUdpSendBroadcast) {
        a = QHostAddress::Broadcast;
    } else {
        a = QHostAddress(mUdpSendIp);
    }

    udp.sendMessage(data, a, mUdpSendPort);
}

void UdpComms::onUdpDataReceived(QByteArray msg, QHostAddress, quint16)
{
    Comms::onReceive(msg);
}
