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

    // Generic receive/send
    enum CommsMode { CommsNone, CommsSerial, CommsTcpServer, CommsTcpClient,
                     CommsUdp };
    CommsMode mCommsMode = CommsNone;
    void setCommsModeAndUpdateGui(CommsMode mode);

    int numBytesRx = 0;
    int numBytesDroppedFromDisplay = 0;
    int numBytesTx = 0;
    bool lastWasHex = false;
    void updateCounterLabels();

    QElapsedTimer lastTimestamp;

    void sendMacro(QString text);

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

private slots:
    void onDataReceived(QByteArray data);
    void sendData(QByteArray data, bool allowEscapeSequenceReplace = true);

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
    void onConsoleZoomChanged();
    // GUI widget slots
    void on_pushButton_Send_clicked();
    void on_checkBox_TimedMessages_Enable_clicked();
    void on_checkBox_AutoReply_Enable_clicked();
    void on_actionScroll_to_Bottom_triggered();
    void on_actionClear_triggered();
    void on_action_Re_Open_SerialPort_triggered();
    void on_action_Close_SerialPort_triggered();
    void on_action_Close_SerialPort_toolbar_triggered();
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

    void on_toolButton_sendFile_browse_clicked();
    void on_pushButton_sendFile_openFolder_clicked();
    void on_checkBox_sendFile_enable_clicked();

    void on_spinBox_maxProcessTimeMs_valueChanged(int value);
    void on_spinBox_displayBacklogLengthMs_valueChanged(int value);

    void on_action_Set_DTR_toggled(bool set);
    void on_action_Set_DTR_toolbar_toggled(bool set);
    void on_action_Set_RTS_toggled(bool set);

    void on_comboBox_macros_append_currentIndexChanged(int index);
    void on_listWidget_macros_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_lineEdit_tcpServer_port_returnPressed();
    void on_lineEdit_tcpClient_ipAddress_returnPressed();
    void on_lineEdit_tcpClient_port_returnPressed();

    void on_spinBox_tabWidth_valueChanged(int value);

    void on_checkBox_showDuck_toggled(bool checked);

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

    void readMacrosFromSettings();
    void saveMacrosToSettings();
    void updateMacroGuiButtonsEnabled();

    void printNetworkAddresses();
};

#endif // MAINWINDOW_H
