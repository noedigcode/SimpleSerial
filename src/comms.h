#ifndef COMMS_H
#define COMMS_H

#include "gidqt5serial.h"
#include "gidtcp.h"
#include "gidudp.h"

#include <QObject>

class Comms : public QObject
{
    Q_OBJECT
public:
    Comms(QObject* parent);
    void setTag(QString tag);
    QString tag();
    virtual QString type() = 0;
    virtual QString titleText() = 0;
    virtual QString errorString() = 0;
    void send(const QByteArray& data);
    virtual void close() = 0;
    void onReceive(QByteArray data);
    int rxByteCount();
    int txByteCount();
    void clearCounters();

signals:
    void print(QString msg);
    void dataReceived(QByteArray data);
    void opened();
    void closed();
    void errorOccurred(QString error);

protected:
    int mRxByteCount = 0;
    int mTxByteCount = 0;
    QString mTag;

    virtual void doSend(const QByteArray& data) = 0;
};
typedef QSharedPointer<Comms> CommsPtr;

// =============================================================================

class SerialComms : public Comms
{
    Q_OBJECT
public:
    SerialComms(QObject* parent);
    QString type();
    QString titleText();
    QString errorString();
    void close() override;
    void setSettings(const QMap<QString, QString> &keyVals);
    QMap<QString, QString> getSettings();
    bool wasOpenBefore();
    void reOpen();
    GidQt5Serial serial;

private:
    bool mWasOpenBefore = false;
    void setupSerial();
    void doSend(const QByteArray& data) override;

private slots:
    void onPortOpened();
    void onSerialReadyRead();
    void onSerialError(QSerialPort::SerialPortError error);
};
typedef QSharedPointer<SerialComms> SerialCommsPtr;

// =============================================================================

class TcpServerComms : public Comms
{
    Q_OBJECT
public:
    TcpServerComms(QObject* parent);
    QString type();
    QString titleText();
    QString errorString();
    void close();
    bool startTcpServer(quint16 port);
    bool restart();

protected:
    GidTcp tcp;
    quint16 mPort = 0;
    void setupTcp();
    void doSend(const QByteArray &data);
private slots:
    void onTcpDataReceived(GidTcp::ConPtr con, QByteArray msg);
};
typedef QSharedPointer<TcpServerComms> TcpServerCommsPtr;

// =============================================================================

class TcpClientComms : public Comms
{
    Q_OBJECT
public:
    TcpClientComms(QObject* parent);
    QString type();
    QString titleText();
    QString errorString();
    void close();
    void connectToServer(QHostAddress address, quint16 port);
    void reConnect();

protected:
    GidTcp tcp;
    QString mIpAddress;
    quint16 mPort = 0;
    void setupTcp();
    void doSend(const QByteArray &data);
private slots:
    void onTcpDataReceived(GidTcp::ConPtr con, QByteArray data);
    void onTcpClientConnectedToServer();
    void onTcpClientError(QString errorString);
    void onTcpClientDisconnected();
};
typedef QSharedPointer<TcpClientComms> TcpClientCommsPtr;

// =============================================================================

class UdpComms : public Comms
{
    Q_OBJECT
public:
    UdpComms(QObject* parent);
    QString type();
    QString titleText();
    QString errorString();
    void close();
    void start(bool listen, quint16 listenPort, bool broadcast, QString sendIp,
               quint16 sendPort);
    void restart();

protected:
    GidUdp udp;
    bool mListen = false;
    int mListenPort = 0;
    int mUdpSendPort = 0;
    bool mUdpSendBroadcast = false;
    QString mUdpSendIp;

    void setupUdp();
    void doSend(const QByteArray &data);
private slots:
    void onUdpDataReceived(QByteArray msg, QHostAddress address, quint16 port);
};
typedef QSharedPointer<UdpComms> UdpCommsPtr;


#endif // COMMS_H
