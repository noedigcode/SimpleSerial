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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "aboutdialog.h"
#include "gidqt5serial.h"
#include "gidtcp.h"
#include "gidudp.h"
#include "settings.h"
#include "version.h"

#include <QBasicTimer>
#include <QCheckBox>
#include <QElapsedTimer>
#include <QFile>
#include <QInputDialog>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QNetworkInterface>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QSettings>
#include <QSpinBox>
#include <QStyleFactory>
#include <QTreeWidgetItem>

class Comms : public QObject
{
    Q_OBJECT

public:

    Comms(QObject* parent) : QObject(parent)
    {

    }
    void setTag(QString tag)
    {
        mTag = tag;
    }
    QString tag()
    {
        return mTag;
    }
    virtual QString type() = 0;
    virtual QString titleText() = 0;
    void send(const QByteArray& data)
    {
        mTxByteCount += data.length();
        doSend(data);
    }
    virtual void close() = 0;
    void onReceive(QByteArray data)
    {
        mRxByteCount += data.length();
        emit dataReceived(data);
    }
    int rxByteCount()
    {
        return mRxByteCount;
    }
    int txByteCount()
    {
        return mTxByteCount;
    }
    void clearCounters()
    {
        mRxByteCount = 0;
        mTxByteCount = 0;
    }

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
    SerialComms(QObject* parent) : Comms(parent)
    {
        setupSerial();
    }
    QString type()
    {
        return "Serial Port";
    }
    QString titleText()
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
    void close() override
    {
        if (serial.s.isOpen()) {
            serial.s.close();
            emit print("Serial port closed.");
            emit closed();
        }
    }
    void setSettings(const QMap<QString, QString> &keyVals)
    {
        serial.setSettings(keyVals);
    }
    QMap<QString, QString> getSettings()
    {
        return serial.getSettings();
    }
    bool wasOpenBefore()
    {
        return mWasOpenBefore;
    }
    void reOpen()
    {
        serial.reOpen();
    }
    GidQt5Serial serial;
private:
    bool mWasOpenBefore = false;
    void setupSerial()
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
    void doSend(const QByteArray& data) override
    {
        serial.s.write(data);
        mTxByteCount += data.length();
    }

private slots:
    void onPortOpened()
    {
        mWasOpenBefore = true;
        emit Comms::opened();
    }
    void onSerialReadyRead()
    {
        Comms::onReceive(serial.s.readAll());
    }
    void onSerialError(QSerialPort::SerialPortError error)
    {
        if (error == QSerialPort::NoError) { return; }
        QString s = QVariant::fromValue(error).toString();
        emit print("Serial port error: " + s);
        emit Comms::errorOccurred(s);
    }
};
typedef QSharedPointer<SerialComms> SerialCommsPtr;

// =============================================================================

class TcpServerComms : public Comms
{
    Q_OBJECT

public:
    TcpServerComms(QObject* parent) : Comms(parent)
    {
        setupTcp();
    }
    QString type()
    {
        return "TCP Server";
    }
    QString titleText()
    {
        QString title = QString("TCP Server (%1)")
                .arg(mPort);
        if (!tcp.isServerListening()) {
            title += " (Closed)";
        }
        return title;
    }
    void close()
    {
        if (tcp.isServerListening()) {
            tcp.stopTcpServer();
            print("TCP server stopped.");
            emit Comms::closed();
        }
    }
    bool startTcpServer(quint16 port)
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
    bool restart()
    {
        return startTcpServer(mPort);
    }

protected:
    GidTcp tcp;
    quint16 mPort = 0;
    void setupTcp()
    {
        connect(&tcp, &GidTcp::print, this, &Comms::print);
        connect(&tcp, &GidTcp::dataReceived,
                this, &TcpServerComms::onTcpDataReceived);
    }
    void doSend(const QByteArray &data)
    {
        tcp.sendMsgToAllClients(data);
    }
private slots:
    void onTcpDataReceived(GidTcp::ConPtr /*con*/, QByteArray msg)
    {
        Comms::onReceive(msg);
    }
};
typedef QSharedPointer<TcpServerComms> TcpServerCommsPtr;

// =============================================================================

class TcpClientComms : public Comms
{
    Q_OBJECT

public:
    TcpClientComms(QObject* parent) : Comms(parent)
    {
        setupTcp();
    }
    QString type()
    {
        return "TCP Client";
    }
    QString titleText()
    {
        QString title = QString("TCP Client (%1:%2)")
                .arg(mIpAddress)
                .arg(mPort);
        if (!tcp.isConnectedToServer()) {
            title += " (Closed)";
        }
        return title;
    }
    void close()
    {
        tcp.disconnectFromServer();
    }
    void connectToServer(QHostAddress address, quint16 port)
    {
        mIpAddress = address.toString();
        mPort = port;
        tcp.connectToServer(address, port);
    }
    void reConnect()
    {
        connectToServer(QHostAddress(mIpAddress), mPort);
    }

protected:
    GidTcp tcp;
    QString mIpAddress;
    quint16 mPort = 0;
    void setupTcp()
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
    void doSend(const QByteArray &data)
    {
        tcp.sendMsg(data);
    }
private slots:
    void onTcpDataReceived(GidTcp::ConPtr /*con*/, QByteArray data)
    {
        Comms::onReceive(data);
    }
    void onTcpClientConnectedToServer()
    {
        print("Connected to TCP server.");
        emit Comms::opened();
    }
    void onTcpClientError(QString errorString)
    {
        print("TCP client error: " + errorString);
        emit Comms::errorOccurred(errorString);
    }
    void onTcpClientDisconnected()
    {
        print("Disconnected from TCP server.");
        emit Comms::closed();
    }
};
typedef QSharedPointer<TcpClientComms> TcpClientCommsPtr;

// =============================================================================

class UdpComms : public Comms
{
    Q_OBJECT

public:
    UdpComms(QObject* parent) : Comms(parent)
    {
        setupUdp();
    }
    QString type()
    {
        return "UDP";
    }
    QString titleText()
    {
        QString title = "UDP";
        if (mListen) {
            title += QString(" (%1)").arg(mListenPort);
        }
        return title;
    }
    void close()
    {
        udp.stopUdp();
        emit Comms::closed();
    }
    void start(bool listen, quint16 listenPort, bool broadcast, QString sendIp,
               quint16 sendPort)
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
    void restart()
    {
        start(mListen, mListenPort, mUdpSendBroadcast, mUdpSendIp, mUdpSendPort);
    }

protected:
    GidUdp udp;
    bool mListen = false;
    int mListenPort = 0;
    int mUdpSendPort = 0;
    bool mUdpSendBroadcast = false;
    QString mUdpSendIp;

    void setupUdp()
    {
        connect(&udp, &GidUdp::print, this, &Comms::print);
        connect(&udp, &GidUdp::rxMessage, this, &UdpComms::onUdpDataReceived);
    }

    void doSend(const QByteArray &data)
    {
        QHostAddress a;
        if (mUdpSendBroadcast) {
            a = QHostAddress::Broadcast;
        } else {
            a = QHostAddress(mUdpSendIp);
        }

        udp.sendMessage(data, a, mUdpSendPort);
    }
private slots:
    void onUdpDataReceived(QByteArray msg, QHostAddress /*address*/,
                           quint16 /*port*/)
    {
        Comms::onReceive(msg);
    }
};
typedef QSharedPointer<UdpComms> UdpCommsPtr;

// =============================================================================

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    struct StartupOptions {
        QString serialPort;
        int baud = 9600;
        QSerialPort::Parity parity = QSerialPort::NoParity;
        QSerialPort::DataBits dataBits = QSerialPort::Data8;
        QSerialPort::StopBits stopBits = QSerialPort::OneStop;
        QString sendFilePath;
        int sendFileFreqMs = 500;
    };

    explicit MainWindow(StartupOptions options, QWidget *parent = 0);
    ~MainWindow();

    static QString getDefaultGuiStyle(QApplication *app);

private:
    Ui::MainWindow *ui;
    QString mAutoReplyBuffer;

    SimpleSerialSettings settings;
    void loadGeneralSettings();

    QMenu* mStyleMenu = nullptr;
    QList<QAction*> styleActions;
    void setupGuiStyleMenu();
    void addGuiStyleAction(QString title, QString style);
    void guiStyleActionTriggered(QString style);
    void updateGuiStyleMenuChecks();

    AboutDialog* aboutDialog = nullptr;

    bool mStartupNoCancelButton = true;
    void showStartupPage();
    void showMainPage();

    QString crlfComboboxText(int index);

    QString userWindowTitle;
    void updateWindowTitle();

    void print(QString msg, QColor color);
    void printOnNewLine(QString msg, QColor color);
    enum DataDirection { DataReceive, DataSend };
    void addDataToConsole(QByteArray data, DataDirection dataDir);
    void addNonBreakingTextToConsole(QString text, QColor color,
                                     QBrush background,
                                     bool virtuallyAtLineStart = false,
                                     bool addSpaceBefore = false);
    bool mLastRxDataAddedToConsoleWasNewline = false;
    void addTextToConsoleAndLogIfEnabled(QString text, QColor color,
                                         QBrush background = QBrush());

    QColor normalTextColor();
    QColor timestampColor();
    QColor hexColor();
    QColor systemTextColor();
    QBrush sendBackground();

    int numBytesDroppedFromDisplay = 0;
    void updateDroppedBytesCounterLabel();

    bool lastWasHex = false;
    void updateCounterLabels(CommsPtr comms);

    QElapsedTimer lastTimestamp;

    /* DataDisplayProcessor displays data in the console asynchronously so the
     * rest of the application doesn't block if large amounts af data is
     * displayed.
     * allowedMs specifies the time allowed for blocked processing.
     * Keeping this low will ensure a responsive GUI.
     * displayBacklogLengthMs specifies how much data can pile up before data
     * will be dropped from the buffers.
     * The number of bytes processed is varied dynamically so allowedMs is not
     * exceeded. Processing time is shared between outgoing and incoming data,
     * but when buffers are dropped, outgoing is dropped first.
     * */
    struct DataDisplayProcessor {
        DataDisplayProcessor(MainWindow* mw) : mainWindow(mw) {}
        void processData(QByteArray data, MainWindow::DataDirection dir);
        int allowedMs = 25;
        int displayBacklogLengthMs = 5000;
    private:
        MainWindow* mainWindow = nullptr;
        void processNext();
        int bufferProcessSize = 1024;
        int lastProcessMs = 0;
        QByteArray rxbuffer;
        QByteArray txbuffer;
    } dataDisplay {this};

    // -------------------------------------------------------------------------
    // Comms - sending and receiving
private:
    CommsPtr mainComms;
    void setMainCommsAndUpdateGui(CommsPtr comms);
    QList<CommsPtr> allComms;
    void addComms(CommsPtr comms);
    void closeAndRemove(CommsPtr comms);

    QMap<QTreeWidgetItem*, CommsPtr> treeCommsMap;

private:
    enum SendEscSeqOption {AllowEscapeSequenceReplace, NoEscapeSequenceReplace};
    enum SendShowOption {AllowShowingSentData, DoNotShowSentData};

    void sendToOnly(CommsPtr comms, QByteArray data, SendEscSeqOption escSeqOption);
    void sendToAllExcept(CommsPtr except, QByteArray data,
                         SendEscSeqOption escSeqOption,
                         SendShowOption sendShowOption);
    void sendToAll(QByteArray data, SendEscSeqOption escSeqOption);
    QByteArray replaceEscapeSequences(const QByteArray& data);
    void sendData(CommsPtr only, CommsPtr exclude, QByteArray data,
                  SendEscSeqOption escSeqOption,
                  SendShowOption sendShowOption);

private slots:
    void onCommsPrint(CommsPtr comms, QString msg);
    void onDataReceived(CommsPtr comms, QByteArray data);
    void onCommsChangeWindowTitle(CommsPtr comms);

    // Serial
private:
    SerialCommsPtr createSerialComms();

private slots:
    void onSerialPortOpened(SerialCommsPtr s);
    void onSerialDataTerminalReadyChanged(SerialCommsPtr s, bool set);
    void onSerialRequestToSendChanged(SerialCommsPtr s, bool set);

    // Network
private:
    TcpClientCommsPtr createTcpClientComms();
    TcpServerCommsPtr createTcpServerComms();
    UdpCommsPtr createUdpComms();

    // Logger
private:
    QFile logFile;
    void log(QByteArray data);
    void flushLog();
    void updateLogGui();
    QString logFilePathFromDialog(QString prevFilename);

    // Macros
private:
    void sendMacro(QString text);
    void readMacrosFromSettings();
    void saveMacrosToSettings();
private slots:
    void on_pushButton_macros_send_clicked();
    void on_pushButton_macros_add_clicked();
    void on_pushButton_macros_edit_clicked();
    void on_pushButton_macros_remove_clicked();
    void on_pushButton_macros_addMultiple_clicked();
    void on_listWidget_macros_itemDoubleClicked(QListWidgetItem *item);
    void on_comboBox_macros_append_currentIndexChanged(int index);

private slots:
    void onConsoleZoomChanged();
    // GUI widget slots
    void on_pushButton_Send_clicked();
    void on_checkBox_TimedMessages_Enable_clicked();
    void on_checkBox_AutoReply_Enable_clicked();
    void on_actionScroll_to_Bottom_triggered();
    void on_actionClear_triggered();

    void on_pushButton_startup_openSerialPort_clicked();
    void on_action_Open_Serial_Port_triggered();
    void on_action_Re_Open_SerialPort_triggered();
    void on_action_Close_SerialPort_triggered();
    void on_action_Close_SerialPort_toolbar_triggered();

    void on_actionAuto_Scroll_changed();
    void on_pushButton_clearCounters_clicked();
    void on_actionSet_Window_Title_triggered();
    void on_actionAbout_triggered();
    void on_comboBox_SendCRLF_currentIndexChanged(int index);
    void on_actionWindow_Always_On_Top_toggled(bool arg1);

    void on_pushButton_startup_tcpServer_clicked();
    void on_pushButton_tcpServer_start_clicked();
    void on_pushButton_tcpServer_cancel_clicked();
    void on_action_Stop_TCP_Server_triggered();
    void on_action_Restart_TCP_Server_triggered();

    void on_pushButton_startup_tcpClient_clicked();
    void on_pushButton_tcpClient_connect_clicked();
    void on_pushButton_tcpClient_cancel_clicked();
    void on_action_Disconnect_from_TCP_Server_triggered();
    void on_action_Reconnect_to_TCP_Server_triggered();

    void on_pushButton_startup_udp_clicked();
    void on_pushButton_udp_start_clicked();
    void on_pushButton_udp_cancel_clicked();

    void on_action_New_Connection_triggered();

    void on_pushButton_log_startStop_clicked();
    void on_toolButton_log_browse_clicked();
    void on_pushButton_log_openFolder_clicked();
    void on_pushButton_log_indicator_clicked();

    void on_action_Tools_triggered();

    void on_toolButton_sendFile_browse_clicked();
    void on_pushButton_sendFile_openFolder_clicked();
    void on_checkBox_sendFile_enable_clicked();

    void on_spinBox_maxProcessTimeMs_valueChanged(int value);
    void on_spinBox_displayBacklogLengthMs_valueChanged(int value);

    void on_action_Set_DTR_toggled(bool set);
    void on_action_Set_DTR_toolbar_toggled(bool set);
    void on_action_Set_RTS_toggled(bool set);

    void on_lineEdit_tcpServer_port_returnPressed();
    void on_lineEdit_tcpClient_ipAddress_returnPressed();
    void on_lineEdit_tcpClient_port_returnPressed();

    void on_spinBox_tabWidth_valueChanged(int value);

    void on_checkBox_showDuck_toggled(bool checked);

    void on_pushButton_forward_add_clicked();

    void on_pushButton_startupCancel_clicked();

    void on_pushButton_forward_close_clicked();

    void on_pushButton_forward_reOpen_clicked();

    void on_pushButton_forward_remove_clicked();

private:
    QBasicTimer timedMsgTimer;
    void onTimedMsgTimer();
    QBasicTimer sendFileTimer;
    void onSendFileTimer();
    void timerEvent(QTimerEvent *ev);

    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *watched, QEvent *event);

    bool firstShow = true;
    void showEvent(QShowEvent *event);

    void focusAndSelectSendText();

    void onAutoScrollChanged();
    void onToolsVisibilityChanged();

    void initActionCheckedSetting(Settings::SettingPtr setting, QAction* action);
    void initCheckableSetting(Settings::SettingPtr setting, QAbstractButton* widget);
    void initLineEditSetting(Settings::SettingPtr setting, QLineEdit* lineEdit);
    void initSpinBox(Settings::SettingPtr setting, QSpinBox* spinBox);

    void setWidgetsEnabledOnItemSelected(QTreeWidget* tw, QList<QWidget*> toEnable);
    void setWidgetsEnabledOnItemSelected(QListWidget* lw, QList<QWidget*> toEnable);

    void printNetworkAddresses();
};

#endif // MAINWINDOW_H
