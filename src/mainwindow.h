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
#include "gidqt5serial/gidqt5serial.h"
#include "gidtcp.h"
#include "gidudp.h"
#include "version.h"

#include <QBasicTimer>
#include <QCheckBox>
#include <QElapsedTimer>
#include <QInputDialog>
#include <QMainWindow>
#include <QMap>
#include <QNetworkInterface>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QSettings>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QString mAutoReplyBuffer;

    QSettings settings;
    void loadGeneralSettings();

    AboutDialog* aboutDialog = nullptr;

    void showStartupPage();
    void showMainPage();

    QString crlfComboboxText(int index);

    QString userWindowTitle;
    void updateWindowTitle();

    void print(QString msg, QColor c = Qt::black);
    enum DataDirection { DataReceive, DataSend };
    void addDataToConsole(QByteArray data, DataDirection dataDir);
    void addTextToConsoleAndLogIfEnabled(QString text, QColor color = Qt::black);

    // Generic receive/send
    enum CommsMode { CommsNone, CommsSerial, CommsTcpServer, CommsTcpClient,
                     CommsUdp };
    CommsMode mCommsMode = CommsNone;
    void setCommsModeAndUpdateGui(CommsMode mode);

    int numBytesRx = 0;
    int numBytesTx = 0;
    bool lastWasHex = false;
    void updateCounterLabels();

    QElapsedTimer lastTimestamp;

    void sendMacro(QString text);

private slots:
    void onDataReceived(QByteArray data);
    void sendData(QByteArray data);

    // Serial
private:
    GidQt5Serial serial;
    void setupSerial();
    void sendSerial(QByteArray data);
    void closeSerialPort();
private slots:
    void onSerialReadyRead();
    void onSerialError(QSerialPort::SerialPortError error);
    void printSerial(QString msg);
    void onSerialPortOpened();

    // Network
private:
    void setupNetwork();

    GidTcp tcp;
    void sendTcpServer(QByteArray data);
    void sendTcpClient(QByteArray data);
    void stopTcpServer();
    void disconnectFromTcpServer();
private slots:
    void printTcp(QString msg);
    void onTcpDataReceived(GidTcp::ConPtr con, QByteArray data);
    void onTcpClientConnectedToServer();
    void onTcpClientError(QString errorString);
    void onTcpClientDisconnected();

private:
    GidUdp udp;
    int mUdpSendPort = 0;
    bool mUdpSendBroadcast = false;
    QString mUdpSendIp;
    void sendUdp(QByteArray data);
    void stopUdp();
private slots:
    void printUdp(QString msg);
    void onUdpDataReceived(QByteArray msg, QHostAddress address, quint16 port);

    // Logger
private:
    QFile logFile;
    void log(QByteArray data);
    void flushLog();
    void updateLogGui();
    QString logFilePathFromDialog(QString prevFilename);

private slots:
    // GUI widget slots
    void on_pushButton_Send_clicked();
    void on_checkBox_TimedMessages_Enable_clicked();
    void on_checkBox_AutoReply_Enable_clicked();
    void on_actionScroll_to_Bottom_triggered();
    void on_actionClear_triggered();
    void on_action_Re_Open_SerialPort_triggered();
    void on_action_Close_SerialPort_triggered();
    void on_actionAuto_Scroll_changed();
    void on_pushButton_clearCounters_clicked();
    void on_actionSet_Window_Title_triggered();
    void on_actionAbout_triggered();
    void on_comboBox_SendCRLF_currentIndexChanged(int index);
    void on_actionWindow_Always_On_Top_toggled(bool arg1);
    void on_pushButton_startup_openSerialPort_clicked();
    void on_action_Open_Serial_Port_triggered();
    void on_pushButton_startup_tcpServer_clicked();
    void on_pushButton_startup_tcpClient_clicked();
    void on_pushButton_startup_udp_clicked();
    void on_pushButton_tcpServer_start_clicked();
    void on_pushButton_tcpClient_connect_clicked();
    void on_pushButton_udp_start_clicked();
    void on_pushButton_tcpServer_cancel_clicked();
    void on_pushButton_tcpClient_cancel_clicked();
    void on_pushButton_udp_cancel_clicked();
    void on_action_New_Connection_triggered();
    void on_action_Stop_TCP_Server_triggered();
    void on_action_Restart_TCP_Server_triggered();
    void on_action_Disconnect_from_TCP_Server_triggered();
    void on_action_Reconnect_to_TCP_Server_triggered();
    void on_pushButton_log_startStop_clicked();
    void on_toolButton_log_browse_clicked();
    void on_pushButton_macros_send_clicked();
    void on_action_Tools_triggered();
    void on_pushButton_macros_add_clicked();
    void on_pushButton_macros_edit_clicked();
    void on_pushButton_macros_remove_clicked();
    void on_pushButton_macros_addMultiple_clicked();
    void on_listWidget_macros_itemDoubleClicked(QListWidgetItem *item);
    void on_pushButton_log_openFolder_clicked();
    void on_pushButton_log_indicator_clicked();

private:
    QBasicTimer timer;
    void timerEvent(QTimerEvent *ev);

    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *watched, QEvent *event);

    void focusAndSelectSendText();

    void onAutoScrollChanged();
    void onToolsVisibilityChanged();

    void initActionCheckedSetting(QString settingKey, QAction* action);
    void initCheckableSetting(QString settingKey, QAbstractButton* widget);
    void initLineEditSetting(QString settingKey, QLineEdit* lineEdit);

    void printNetworkAddresses();

    const QString settingAutoScroll = "autoScroll";
    const QString settingCrLf = "crlf";
    const QString settingDisplayModeText = "displayModeText";
    const QString settingDisplayModeHex = "displayModeHex";
    const QString settingHexSpecial = "hexSpecial";
    const QString settingShowCrLfHex = "showCrLfHex";
    const QString settingNewlineForCrLf = "newlineForCrLf";
    const QString settingReplaceEscapeSequences = "replaceEscapeSequences";
    const QString settingShowSentData = "showSentData";
    const QString settingSentDataOnSeparateLine = "sentDataOnSeparateLine";
    const QString settingTcpServerPort = "tcpServerPort";
    const QString settingTcpClientIp = "tcpClientIp";
    const QString settingTcpClientPort = "tcpClientPort";
    const QString settingUdpBindForListen = "udpBindForListen";
    const QString settingUdpBindPort = "udpBindPort";
    const QString settingUdpSendBroadcast = "udpSendBroadcast";
    const QString settingUdpSendIp = "udpSendIp";
    const QString settingUdpSendPort = "udpSendPort";
};

#endif // MAINWINDOW_H
