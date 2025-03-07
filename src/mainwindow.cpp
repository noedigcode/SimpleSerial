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
#include "ui_mainwindow.h"

#include "Utilities.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QTime>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    settings(QSettings::NativeFormat, QSettings::UserScope,
             "Noedigcode", "SimpleSerial")
{
    ui->setupUi(this);

    showStartupPage();

    this->resize(Utilities::scaleWithPrimaryScreenScalingFactor(this->size()));

    onToolsVisibilityChanged();
    // Default tools tabs
    ui->tabWidget_tools->setCurrentWidget(ui->tab_options);
    ui->tabWidget_options->setCurrentWidget(ui->tab_displayMode);

    ui->comboBox_send->installEventFilter(this);
    ui->console->installEventFilter(this);

    // Disable combo box auto-complete
    ui->comboBox_send->setCompleter(0);

    loadGeneralSettings();
    setupSerial();
    setupNetwork();

    updateWindowTitle();

    setCommsModeAndUpdateGui(CommsNone);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    serial.close();
    event->accept();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    bool ret = false;

    if (watched == ui->comboBox_send) {

        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if ( (keyEvent->key() == Qt::Key_Enter) ||
                 (keyEvent->key() == Qt::Key_Return) ) {
                on_pushButton_Send_clicked();
                ret = true;
            } else if (keyEvent->key() == Qt::Key_Up) {
                int index = ui->comboBox_send->currentIndex();
                if (ui->comboBox_send->currentText() != ui->comboBox_send->itemText(index)) {
                    // Text has been edited. Revert to last item in list.
                    index = ui->comboBox_send->count() - 1;
                } else {
                    // Go one item up in list.
                    if (index) { index--; }
                }
                ui->comboBox_send->setCurrentIndex(index);
                focusAndSelectSendText();
                ret = true;
            } else if (keyEvent->key() == Qt::Key_Down) {
                int index = ui->comboBox_send->currentIndex();
                if (index < ui->comboBox_send->count() - 1) {
                    index++;
                }
                ui->comboBox_send->setCurrentIndex(index);
                focusAndSelectSendText();
                ret = true;
            } else if (keyEvent->key() == Qt::Key_Escape) {
                ui->comboBox_send->setCurrentText("");
            }
        }

    } else if (watched == ui->console) {

        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if ( (keyEvent->modifiers() == Qt::NoModifier) ||
                 (keyEvent->modifiers() == Qt::ShiftModifier) ) {
                ui->comboBox_send->setFocus();
                QApplication::sendEvent(ui->comboBox_send, keyEvent);
            }
        }

    } else {
        // Pass the event on to the parent class
        ret = QMainWindow::eventFilter(watched, event);
    }

    return ret;
}

void MainWindow::focusAndSelectSendText()
{
    // Set text box focus and select all text
    ui->comboBox_send->setFocus();
    ui->comboBox_send->lineEdit()->selectAll();
}

void MainWindow::onAutoScrollChanged()
{
    ui->console->autoScroll(ui->actionAuto_Scroll->isChecked());
}

void MainWindow::onToolsVisibilityChanged()
{
    ui->tabWidget_tools->setVisible(ui->action_Tools->isChecked());
}

void MainWindow::initActionCheckedSetting(QString settingKey, QAction *action)
{
    // Set the action checked based on the setting, default to the current
    // action state if the setting does not exist.
    action->setChecked(settings.value(settingKey, action->isChecked()).toBool());
    connect(action, &QAction::changed, [=](){
        settings.setValue(settingKey, action->isChecked());
    });
}

void MainWindow::initCheckableSetting(QString settingKey, QAbstractButton* widget)
{
    // Set the checkbox checked based on the setting, default to the current
    // state if the setting does not exist.
    widget->setChecked(settings.value(settingKey, widget->isChecked()).toBool());
    connect(widget, &QCheckBox::toggled, [=](){
        settings.setValue(settingKey, widget->isChecked());
    });
}

void MainWindow::initLineEditSetting(QString settingKey, QLineEdit* lineEdit)
{
    lineEdit->setText(settings.value(settingKey, lineEdit->text()).toString());
    connect(lineEdit, &QLineEdit::textChanged, [=](){
        settings.setValue(settingKey, lineEdit->text());
    });
}

void MainWindow::initSpinBox(QString settingKey, QSpinBox* spinBox)
{
    spinBox->setValue(settings.value(settingKey, spinBox->value()).toInt());
    connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int value)
    {
        settings.setValue(settingKey, value);
    });
}

void MainWindow::printNetworkAddresses()
{
    QString text = "This computer's IP addresses:\n";

    foreach (QNetworkInterface iface, QNetworkInterface::allInterfaces()) {
        text += QString("%1: ").arg(iface.name());
        QString addresses;
        foreach (QNetworkAddressEntry addr, iface.addressEntries()) {
            if (addr.ip().protocol() != QAbstractSocket::IPv4Protocol) { continue; }
            if (!addresses.isEmpty()) { addresses.append(", "); }
            addresses.append(addr.ip().toString());
        }
        if (addresses.isEmpty()) { addresses = "No addresses"; }
        text += addresses;
        text += "\n";
    }
    if (text.isEmpty()) { text = "No network interfaces"; }

    printTcp(text);
}

void MainWindow::print(QString msg, QColor c)
{
    ui->console->addText(msg + "\n", c);
}

void MainWindow::addDataToConsole(QByteArray data, DataDirection dataDir)
{
    bool timestampEnabled = ui->checkBox_timestamps_enable->isChecked();
    bool timestampAfterNewline = ui->checkBox_timestamps_after_newline->isChecked();
    int timestampTimeLimitMs = ui->spinBox_timestamps_time_ms->text().toInt();

    if (!lastTimestamp.isValid()) { lastTimestamp.start(); }

    bool timestampTimeElapsed = (lastTimestamp.elapsed() > timestampTimeLimitMs);
    if (timestampTimeElapsed) { lastTimestamp.start(); }

    bool timestampShown = false;

    for (int i=0; i < data.count(); i++) {

        unsigned char c = data.at(i);
        bool outputHex = false;
        bool outputNormal = true;

        bool printTimestamp = false;
        if (timestampEnabled) {

            if (dataDir == DataSend) {
                // No newline or time grouping for sending
                if (!timestampShown) {
                    printTimestamp = true;
                }
            } else {
                // When receiving data, extra timestamp options come into play
                // Check if timestamp is only allowed after newline
                if (timestampAfterNewline) {
                    if (ui->console->lastAddedWasNewline()) {
                        printTimestamp = true;
                    } else {
                        printTimestamp = false;
                    }
                } else {
                    // Timestamp not limited by newline, so allow it if it hasn't
                    // been shown yet in this cycle.
                    if (!timestampShown) {
                        printTimestamp = true;
                    }
                }

                // Now check time threshold requirements for timestamp
                if (printTimestamp && timestampTimeLimitMs) {
                    if (!timestampTimeElapsed) {
                        // New timestamp not allowed based on time threshold
                        printTimestamp = false;
                    }
                }
            }
        }

        if (ui->radioButton_displayMode_hex->isChecked()) {
            // Hex mode.
            // Hex mode trumps the rest. All characters are converted to hex.
            // Newlines are treated as normal hex data.
            outputHex = true;
            outputNormal = false;
        } else {
            // ASCII text mode.
            // Take newline and special characters hex into account.
            if (c == '\n') {
                outputNormal = false;
                if (ui->checkBox_showCrLfHex->isChecked()) {
                    outputHex = true;
                }
                if (ui->checkBox_crLfNewline->isChecked()) {
                    // Outputting a newline is allowed
                    outputNormal = true;
                }
            } else if (c == '\r') {
                outputNormal = false;
                if (ui->checkBox_showCrLfHex->isChecked()) {
                    outputHex = true;
                }
            } else if ( ((c < 32) || (c == 127)) && (c != '\t')) {
                // Special character hex
                if (ui->checkBox_showHexForSpecialChars->isChecked()) {
                    outputHex = true;
                    outputNormal = false;
                }
            }
        }

        // If showing send data, add a newline before it if set
        if ((dataDir == DataSend)
            && (!ui->console->lastAddedWasNewline()) // Prevent unnecessary empty lines
            && (i == 0))
        {
            if (ui->checkBox_showSentDataOnSeparateLine->isChecked()) {
                addTextToConsoleAndLogIfEnabled("\n");
            }
        }

        if (printTimestamp) {
            QString t;
            if (!ui->console->lastAddedWasNewline()) {
                t += "\n";
            }
            t += QString("%1: ")
                    .arg(QTime::currentTime().toString("hh:mm:ss:zzz"));
            addTextToConsoleAndLogIfEnabled(t, Qt::blue);
            lastWasHex = false;
            timestampShown = true;
        }

        if (outputHex) {
            QString t;

            // Do not split the hex value across a line ending
            bool addedNewline = false;
            if (ui->console->remainingOnLine() < 4) {
                t += "\n";
                addedNewline = true;
            }

            // Add space before hex value if not at the start of a line
            bool atStartOfLine =    printTimestamp
                                 || addedNewline
                                 || (ui->console->currentLineLength() == 0)
                                 || ui->console->lastAddedWasNewline();

            if (!atStartOfLine) {
                t += " ";
            }

            // Print hex value
            t += QString("%1").arg(c, 2, 16, QChar('0')).toUpper();
            addTextToConsoleAndLogIfEnabled(t, QColor(Qt::red));
            lastWasHex = true;
        }

        // Normal text output
        if (outputNormal) {
            if (c == '\n') {
                lastWasHex = false;
            }

            // If a hex value was last added to the console, add a space to
            // separate it from the following text.
            if (lastWasHex) {
                addTextToConsoleAndLogIfEnabled(" ");
                lastWasHex = false;
            }

            addTextToConsoleAndLogIfEnabled(QString(c), QColor(Qt::black));
        }
        // Newline after showing send data
        if ((dataDir == DataSend)
            && (!ui->console->lastAddedWasNewline()) // Prevent unnecessary empty lines
            && (i == data.length()-1))
        {
            if (ui->checkBox_showSentDataOnSeparateLine->isChecked()) {
                addTextToConsoleAndLogIfEnabled("\n");
                lastWasHex = false;
            }
        }
    }
}

void MainWindow::addTextToConsoleAndLogIfEnabled(QString text, QColor color)
{
    ui->console->addText(text, color);
    if (ui->radioButton_log_asDisplayed->isChecked()) {
        log(text.toLocal8Bit());
    }
}

void MainWindow::setCommsModeAndUpdateGui(CommsMode mode)
{
    mCommsMode = mode;

    bool serial = (mode == CommsSerial);
    ui->action_Re_Open_SerialPort->setVisible(serial);
    ui->action_Close_SerialPort->setVisible(serial);
    ui->action_Open_Serial_Port->setVisible(serial);

    bool tcpServer = (mode == CommsTcpServer);
    ui->action_Restart_TCP_Server->setVisible(tcpServer);
    ui->action_Stop_TCP_Server->setVisible(tcpServer);

    bool tcpClient = (mode == CommsTcpClient);
    ui->action_Reconnect_to_TCP_Server->setVisible(tcpClient);
    ui->action_Disconnect_from_TCP_Server->setVisible(tcpClient);
}

void MainWindow::onDataReceived(QByteArray data)
{
    // Display number of received bytes
    numBytesRx += data.count();
    updateCounterLabels();

    addDataToConsole(data, DataReceive);
    // Log raw data if enabled
    if (ui->radioButton_log_raw->isChecked()) {
        log(data);
    }

    // Auto-reply
    if (ui->checkBox_AutoReply_Enable->isChecked()) {
        for (int i=0; i < data.count(); i++) {
            mAutoReplyBuffer.append( data.at(i) );
            while (mAutoReplyBuffer.count() > ui->lineEdit_AutoReply_rx->text().count()) {
                mAutoReplyBuffer.remove(0,1);
            }
            if (mAutoReplyBuffer == ui->lineEdit_AutoReply_rx->text()) {
                // Received buffer matches rx lineedit in GUI. Send back msg.
                QString tosend = ui->lineEdit_AutoReply_send->text();
                tosend.append(crlfComboboxText(ui->comboBox_AutoReply_CRLF->currentIndex()));
                sendData( tosend.toLocal8Bit() );
                mAutoReplyBuffer.clear();
            }
        }
    }

    flushLog();
}

void MainWindow::sendData(QByteArray data)
{
    if (ui->checkBox_sending_replaceEscapeSequences->isChecked()) {
        // Hex
        static QByteArray hex("0123456789abcdef");
        QByteArray tosend;
        QByteArray lowerData = data.toLower();
        QByteArray buffer; // Collect bytes to be converted to hex
        int state = 0;
        for (int i = 0; i < data.count(); i++) {
            char c = data.at(i);
            char clower = lowerData.at(i);
            bool addChar = false; // Add current char to send data
            bool addBuffer = false; // Add collected bytes to send data

            if (state == 0) {
                // Wait for start of escape sequence.
                buffer.clear();
                if (c == '\\') {
                    // Start of escape sequence
                    state = 1;
                } else {
                    // Not start of escape sequence. Send char.
                    addChar = true;
                }
            } else if (state == 1) {
                if (hex.contains(clower)) {
                    // Hex character. Collect in buffer.
                    buffer.append(c);
                    state = 2;
                } else {
                    // Cancel escape sequence. Also send collected buffer chars.
                    addChar = true;
                    addBuffer = true;
                    state = 0;
                }
            } else if (state == 2) {
                if (hex.contains(clower)) {
                    // Hex character. Collect in buffer, convert to hex and send.
                    buffer.append(c);
                    tosend.append(buffer.toShort(nullptr, 16));
                } else {
                    // Cancel escape sequence. Also send collected buffer chars.
                    addChar = true;
                    addBuffer = true;
                }
                // End of escape sequence. Reset to wait for next.
                state = 0;
            }

            if (addBuffer) {
                tosend.append("\\");
                tosend.append(buffer);
            }
            if (addChar) { tosend.append(c); }
        }
        data = tosend;
        // Other escape sequences
        data.replace(QByteArray("\\n"), QByteArray("\n"));
        data.replace(QByteArray("\\r"), QByteArray("\r"));
        data.replace(QByteArray("\\t"), QByteArray("\t"));
        // NB: Do \0 after hex above so it doesn't interfere
        data.replace(QByteArray("\\0"), QByteArray(1, '\0'));
        data.replace(QByteArray("\\\\"), QByteArray("\\"));
    }

    switch (mCommsMode) {
    case MainWindow::CommsNone:
        return;
        break;
    case MainWindow::CommsSerial:
        sendSerial(data);
        break;
    case MainWindow::CommsTcpServer:
        sendTcpServer(data);
        break;
    case MainWindow::CommsTcpClient:
        sendTcpClient(data);
        break;
    case MainWindow::CommsUdp:
        sendUdp(data);
        break;
    }

    numBytesTx += data.count();
    updateCounterLabels();

    if (ui->checkBox_showSentDataInConsole->isChecked()) {
        addDataToConsole(data, DataSend);
    }

    flushLog();
}

void MainWindow::setupSerial()
{
    // Serial settings
    QMap<QString, QString> serialSettings;
    settings.beginGroup("serial");
    foreach (QString key, settings.allKeys()) {
        serialSettings.insert(key, settings.value(key).toString());
    }
    settings.endGroup();
    serial.setSettings(serialSettings);

    connect(&(serial.s), &QSerialPort::readyRead,
            this, &MainWindow::onSerialReadyRead);
    connect(&(serial.s), &QSerialPort::errorOccurred,
            this, &MainWindow::onSerialError);
    connect(&serial, &GidQt5Serial::print,
            this, &MainWindow::printSerial);
    connect(&serial, &GidQt5Serial::portOpened,
            this, &MainWindow::onSerialPortOpened);

    serial.setWindowModality(Qt::ApplicationModal);
    serial.setWindowTitle(QString("%1 %2").arg(APP_NAME).arg(APP_VERSION));
    serial.resize(Utilities::scaleWithPrimaryScreenScalingFactor(serial.size()));
}

void MainWindow::sendSerial(QByteArray data)
{
    serial.s.write(data);
}

void MainWindow::closeSerialPort()
{
    if (serial.s.isOpen()) {
        serial.s.close();
        printSerial("Serial port closed.");
        updateWindowTitle();
    }
}

void MainWindow::updateCounterLabels()
{
    ui->label_bytesRx->setText(QString::number(numBytesRx));
    ui->label_bytesTx->setText(QString::number(numBytesTx));
}

void MainWindow::sendMacro(QString text)
{
    text += crlfComboboxText(ui->comboBox_macros_append->currentIndex());
    sendData(text.toLocal8Bit());
}

void MainWindow::onSerialReadyRead()
{
    onDataReceived(serial.s.readAll());
}

void MainWindow::onSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) { return; }
    QString s = QVariant::fromValue(error).toString();
    printSerial("Serial port error: " + s);
}

void MainWindow::onSerialPortOpened()
{
    setCommsModeAndUpdateGui(CommsSerial);
    updateWindowTitle();

    focusAndSelectSendText();

    // Save settings
    QMap<QString, QString> serialSettings = serial.getSettings();
    settings.beginGroup("serial");
    foreach (QString key, serialSettings.keys()) {
        settings.setValue(key, serialSettings.value(key));
    }
    settings.endGroup();

    showMainPage();
}

void MainWindow::setupNetwork()
{
    // TCP
    connect(&tcp, &GidTcp::print, this, &MainWindow::printTcp);
    connect(&tcp, &GidTcp::dataReceived, this, &MainWindow::onTcpDataReceived);
    connect(&tcp, &GidTcp::clientConnected,
            this, &MainWindow::onTcpClientConnectedToServer);
    connect(&tcp, &GidTcp::clientDisconnected,
            this, &MainWindow::onTcpClientDisconnected);
    connect(&tcp, &GidTcp::clientConnectionError,
            this, &MainWindow::onTcpClientError);

    // UDP
    connect(&udp, &GidUdp::print, this, &MainWindow::printUdp);
    connect(&udp, &GidUdp::rxMessage, this, &MainWindow::onUdpDataReceived);
}

void MainWindow::sendTcpServer(QByteArray data)
{
    tcp.sendMsgToAllClients(data);
}

void MainWindow::sendTcpClient(QByteArray data)
{
    tcp.sendMsg(data);
}

void MainWindow::stopTcpServer()
{
    if (tcp.isServerListening()) {
        tcp.stopTcpServer();
        printTcp("TCP server stopped.");
        updateWindowTitle();
    }
}

void MainWindow::disconnectFromTcpServer()
{
    tcp.disconnectFromServer();
}

void MainWindow::printTcp(QString msg)
{
    print("[tcp] " + msg, Qt::darkGray);
}

void MainWindow::onTcpDataReceived(GidTcp::ConPtr /*con*/, QByteArray data)
{
    onDataReceived(data);
}

void MainWindow::onTcpClientConnectedToServer()
{
    printTcp("Connected to TCP server.");
    updateWindowTitle();
}

void MainWindow::onTcpClientError(QString errorString)
{
    printTcp("TCP client error: " + errorString);
    updateWindowTitle();
}

void MainWindow::onTcpClientDisconnected()
{
    printTcp("Disconnected from TCP server.");
    updateWindowTitle();
}

void MainWindow::sendUdp(QByteArray data)
{
    QHostAddress a;
    if (mUdpSendBroadcast) {
        a = QHostAddress::Broadcast;
    } else {
        a = QHostAddress(mUdpSendIp);
    }

    udp.sendMessage(data, a, mUdpSendPort);
}

void MainWindow::stopUdp()
{
    udp.stopUdp();
    updateWindowTitle();
}

void MainWindow::printUdp(QString msg)
{
    print("[udp] " + msg, Qt::darkGray);
}

void MainWindow::onUdpDataReceived(QByteArray msg, QHostAddress /*address*/,
                                   quint16 /*port*/)
{
    onDataReceived(msg);
}

void MainWindow::log(QByteArray data)
{
    if (logFile.isOpen()) {
        qint64 n = logFile.write(data);
        if (n == -1) {
            ui->lineEdit_log_status->setText(
                        QString("Log error: %1")
                        .arg(logFile.errorString()));
            ui->pushButton_log_indicator->setText("Log Error");
        }
    }
}

void MainWindow::flushLog()
{
    if (logFile.isOpen()) {
        logFile.flush();
    }
}

void MainWindow::updateLogGui()
{
    bool logging = logFile.isOpen();

    ui->lineEdit_log_path->setEnabled(!logging);
    ui->toolButton_log_browse->setEnabled(!logging);

    if (logging) {
        ui->pushButton_log_startStop->setText("Stop");
    } else {
        ui->pushButton_log_startStop->setText("Start Log");
    }
}

QString MainWindow::logFilePathFromDialog(QString prevFilename)
{
    return QFileDialog::getSaveFileName(this,
                                        "New Log File",
                                        prevFilename,
                                        "Text file (*.txt);;All files (*.*)");
}

void MainWindow::printSerial(QString msg)
{
    print("[serial] " + msg, Qt::darkGray);
}

void MainWindow::on_pushButton_Send_clicked()
{
    QString origText = ui->comboBox_send->currentText();
    QString tosend = origText + crlfComboboxText(ui->comboBox_SendCRLF->currentIndex());
    sendData(tosend.toLocal8Bit());

    // Add text to combo box (original text without CR/LF added)
    // But don't add it again if it's the same as the last sent one
    if (origText != ui->comboBox_send->itemText(ui->comboBox_send->count() - 1)) {
        ui->comboBox_send->addItem(origText);
        ui->comboBox_send->setCurrentIndex(ui->comboBox_send->count()-1);
    }

    focusAndSelectSendText();
}

/* User clicked checkbox to enable or disable timed messages. */
void MainWindow::on_checkBox_TimedMessages_Enable_clicked()
{
    if (ui->checkBox_TimedMessages_Enable->isChecked()) {
        timedMsgTimer.start( ui->spinBox_TimedMsgs_ms->value(), this );
    } else {
        if (timedMsgTimer.isActive()) { timedMsgTimer.stop(); }
    }
}

/* Called on every timer tick. */
void MainWindow::timerEvent(QTimerEvent* ev)
{
    if (ev->timerId() == timedMsgTimer.timerId()) {
        onTimedMsgTimer();
    } else if (ev->timerId() == sendFileTimer.timerId()) {
        onSendFileTimer();
    }
}

/* User clicked checkbox to enable or disable auto-reply. */
void MainWindow::on_checkBox_AutoReply_Enable_clicked()
{
    mAutoReplyBuffer.clear();
}

void MainWindow::on_actionScroll_to_Bottom_triggered()
{
    ui->console->scrollToBottom();
}

void MainWindow::on_actionClear_triggered()
{
    ui->console->clear();
}

void MainWindow::on_action_Re_Open_SerialPort_triggered()
{
    serial.reOpen();
}

void MainWindow::on_action_Close_SerialPort_triggered()
{
    closeSerialPort();
}

void MainWindow::on_actionAuto_Scroll_changed()
{
    onAutoScrollChanged();
}

void MainWindow::on_pushButton_clearCounters_clicked()
{
    numBytesRx = 0;
    numBytesTx = 0;
    updateCounterLabels();
}

void MainWindow::on_actionSet_Window_Title_triggered()
{
    userWindowTitle = QInputDialog::getText(this, "Set Window Title", "Title");
    updateWindowTitle();
}

void MainWindow::on_actionAbout_triggered()
{
    if (!aboutDialog) {
        aboutDialog = new AboutDialog(settings.fileName(), this);
        aboutDialog->setWindowModality(Qt::ApplicationModal);
    }
    aboutDialog->show();
}

void MainWindow::on_comboBox_SendCRLF_currentIndexChanged(int index)
{
    settings.setValue("crlf", index);
}

void MainWindow::on_actionWindow_Always_On_Top_toggled(bool arg1)
{
    this->setWindowFlag(Qt::WindowStaysOnTopHint, arg1);
    this->show();
}

void MainWindow::loadGeneralSettings()
{
    // Auto scroll
    initActionCheckedSetting(settingAutoScroll, ui->actionAuto_Scroll);
    onAutoScrollChanged();

    // Send CR/LF
    ui->comboBox_SendCRLF->blockSignals(true);
    ui->comboBox_SendCRLF->setCurrentIndex(settings.value("crlf").toInt());
    ui->comboBox_SendCRLF->blockSignals(false);

    // Display mode
    initCheckableSetting(settingDisplayModeText, ui->radioButton_displayMode_text);
    initCheckableSetting(settingDisplayModeHex, ui->radioButton_displayMode_hex);

    // Text mode settings
    initCheckableSetting(settingHexSpecial, ui->checkBox_showHexForSpecialChars);
    initCheckableSetting(settingShowCrLfHex, ui->checkBox_showCrLfHex);
    initCheckableSetting(settingNewlineForCrLf, ui->checkBox_crLfNewline);

    // Replace escape sequences setting
    initCheckableSetting(settingReplaceEscapeSequences,
                        ui->checkBox_sending_replaceEscapeSequences);

    // Show sent data
    initCheckableSetting(settingShowSentData, ui->checkBox_showSentDataInConsole);
    initCheckableSetting(settingSentDataOnSeparateLine,
                         ui->checkBox_showSentDataOnSeparateLine);

    // TCP server settings
    initLineEditSetting(settingTcpServerPort, ui->lineEdit_tcpServer_port);

    // TCP client settings
    initLineEditSetting(settingTcpClientIp, ui->lineEdit_tcpClient_ipAddress);
    initLineEditSetting(settingTcpClientPort, ui->lineEdit_tcpClient_port);

    // UDP settings
    initCheckableSetting(settingUdpBindForListen, ui->checkBox_udp_bindForListening);
    initLineEditSetting(settingUdpBindPort, ui->lineEdit_udp_listenPort);
    initCheckableSetting(settingUdpSendBroadcast, ui->checkBox_udp_broadcast);
    initLineEditSetting(settingUdpSendIp, ui->lineEdit_udp_sendIpAddress);
    initLineEditSetting(settingUdpSendPort, ui->lineEdit_udp_sendPort);

    // Send file settings
    initLineEditSetting(settingSendFilePath, ui->lineEdit_sendFile_path);
    initSpinBox(settingSendFileFrequencyMs, ui->spinBox_sendFile_ms);
}

void MainWindow::updateWindowTitle()
{
    if (!userWindowTitle.isEmpty()) {
        setWindowTitle(userWindowTitle);
    } else {
        QString title;
        if (mCommsMode == CommsSerial) {
            if (serial.s.isOpen()) {
                title = QString("%1 (%2)")
                        .arg(serial.s.portName())
                        .arg(serial.s.baudRate());
            } else {
                title = QString("%1 (Closed)")
                        .arg(serial.s.portName());
            }
        } else if (mCommsMode == CommsTcpServer) {
            title = QString("TCP Server (%1)")
                    .arg(ui->lineEdit_tcpServer_port->text());
            if (!tcp.isServerListening()) {
                title += " (Closed)";
            }
        } else if (mCommsMode == CommsTcpClient) {
            title = QString("TCP Client (%1:%2)")
                    .arg(ui->lineEdit_tcpClient_ipAddress->text())
                    .arg(ui->lineEdit_tcpClient_port->text());
            if (!tcp.isConnectedToServer()) {
                title += " (Closed)";
            }
        } else if (mCommsMode == CommsUdp) {
            title = "UDP";
            if (ui->checkBox_udp_bindForListening->isChecked()) {
                title += QString(" (%1)").arg(ui->lineEdit_udp_listenPort->text());
            }
        }
        if (!title.isEmpty()) {
            title += " - ";
        }
        title += QString("%1 %2").arg(APP_NAME).arg(APP_VERSION);
        setWindowTitle(title);
    }

}

void MainWindow::showStartupPage()
{
    ui->stackedWidget->setCurrentWidget(ui->page_startup);
    ui->mainToolBar->setVisible(false);
}

void MainWindow::showMainPage()
{
    ui->stackedWidget->setCurrentWidget(ui->page_main);
    ui->mainToolBar->setVisible(true);
}

QString MainWindow::crlfComboboxText(int index)
{
    QString ret;
    switch (index) {
    case 0:
        // Nothing
        break;
    case 1:
        // Send CR
        ret.append("\r");
        break;
    case 2:
        // Send LF
        ret.append("\n");
        break;
    case 3:
        // Send CR+LF
        ret.append("\r\n");
        break;
    }
    return ret;
}

void MainWindow::on_pushButton_startup_openSerialPort_clicked()
{
    on_action_Open_Serial_Port_triggered();
}

void MainWindow::on_action_Open_Serial_Port_triggered()
{
    serial.refreshSerialPortList();
    serial.show();
}

void MainWindow::on_pushButton_startup_tcpServer_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_tcpServer);
}

void MainWindow::on_pushButton_startup_tcpClient_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_tcpClient);
}

void MainWindow::on_pushButton_startup_udp_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_udp);
}

void MainWindow::on_pushButton_tcpServer_start_clicked()
{
    // Start TCP server

    bool ok;
    int port = ui->lineEdit_tcpServer_port->text().toInt(&ok);
    if (!ok) { return; }

    if (tcp.setupTcpServer(port)) {
        printNetworkAddresses();
    }
    setCommsModeAndUpdateGui(CommsTcpServer);
    updateWindowTitle();

    showMainPage();
}

void MainWindow::on_pushButton_tcpClient_connect_clicked()
{
    // Connect to TCP server

    QString ip = ui->lineEdit_tcpClient_ipAddress->text();
    bool ok;
    int port = ui->lineEdit_tcpClient_port->text().toInt(&ok);
    if (!ok) { return; }

    tcp.connectToServer(QHostAddress(ip), port);
    setCommsModeAndUpdateGui(CommsTcpClient);
    updateWindowTitle();

    showMainPage();
}

void MainWindow::on_pushButton_udp_start_clicked()
{
    // Setup UDP

    bool ok;
    int listenPort = ui->lineEdit_udp_listenPort->text().toInt(&ok);
    if (!ok) { return; }

    mUdpSendPort = ui->lineEdit_udp_sendPort->text().toInt(&ok);
    if (!ok) { return; }

    // Bind to port for listening
    if (ui->checkBox_udp_bindForListening->isChecked()) {
        udp.setupUdp(listenPort);
    }

    // Setup sending
    mUdpSendBroadcast = ui->checkBox_udp_broadcast->isChecked();
    mUdpSendIp = ui->lineEdit_udp_sendIpAddress->text();

    setCommsModeAndUpdateGui(CommsUdp);
    printUdp("UDP mode initialised");
    updateWindowTitle();

    showMainPage();
}

void MainWindow::on_pushButton_tcpServer_cancel_clicked()
{
    showStartupPage();
}

void MainWindow::on_pushButton_tcpClient_cancel_clicked()
{
    showStartupPage();
}

void MainWindow::on_pushButton_udp_cancel_clicked()
{
    showStartupPage();
}

void MainWindow::on_action_New_Connection_triggered()
{
    // Close all current connections
    closeSerialPort();
    stopTcpServer();
    disconnectFromTcpServer();
    stopUdp();

    setCommsModeAndUpdateGui(CommsNone);
    updateWindowTitle();

    showStartupPage();
}

void MainWindow::on_action_Stop_TCP_Server_triggered()
{
    stopTcpServer();
}

void MainWindow::on_action_Restart_TCP_Server_triggered()
{
    stopTcpServer();
    on_pushButton_tcpServer_start_clicked();
}

void MainWindow::on_action_Disconnect_from_TCP_Server_triggered()
{
    disconnectFromTcpServer();
}

void MainWindow::on_action_Reconnect_to_TCP_Server_triggered()
{
    disconnectFromTcpServer();
    on_pushButton_tcpClient_connect_clicked();
}

void MainWindow::on_pushButton_log_startStop_clicked()
{
    if (logFile.isOpen()) {
        // Stop
        logFile.close();
        ui->lineEdit_log_status->setText("Logging stopped. Log file closed.");
        ui->pushButton_log_indicator->setText("Not Logging");
    } else {
        // Start

        QString filename = ui->lineEdit_log_path->text();
        // If filename is empty or already exists, ask user to select new name
        // NB: Don't simply call toolbutton function, because if the user cancels
        //     this process must cancel, even if the text box has a name in it.
        QFileInfo fi(filename);
        if (filename.isEmpty() || fi.exists()) {
            filename = logFilePathFromDialog(filename);
        }
        if (filename.isEmpty()) { return; }

        ui->lineEdit_log_path->setText(filename);
        logFile.setFileName(ui->lineEdit_log_path->text());
        if (!logFile.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "Log File Error",
                                  QString("Error creating new log file: %1")
                                  .arg(logFile.errorString()));
            ui->lineEdit_log_status->setText(
                        QString("Error opening log file: %1")
                        .arg(logFile.errorString()));
            ui->pushButton_log_indicator->setText("Not Logging");
            return;
        }
        ui->lineEdit_log_status->setText("Logging to file.");
        ui->pushButton_log_indicator->setText("Logging");
    }

    updateLogGui();
}

void MainWindow::on_toolButton_log_browse_clicked()
{
    QString path = logFilePathFromDialog(ui->lineEdit_log_path->text());

    if (path.isEmpty()) { return; }

    ui->lineEdit_log_path->setText(path);
}

void MainWindow::on_pushButton_macros_send_clicked()
{
    QListWidgetItem* item = ui->listWidget_macros->currentItem();
    if (!item) { return; }

    sendMacro(item->text());
}

void MainWindow::on_action_Tools_triggered()
{
    onToolsVisibilityChanged();
}

void MainWindow::on_pushButton_macros_add_clicked()
{
    QString text = QInputDialog::getText(this, "Macro", "Text");
    if (text.isEmpty()) { return; }
    ui->listWidget_macros->addItem(text);
}

void MainWindow::on_pushButton_macros_edit_clicked()
{
    QListWidgetItem* item = ui->listWidget_macros->currentItem();
    if (!item) { return; }

    QString text = QInputDialog::getText(this, "Macro", "Text",
                                         QLineEdit::Normal,
                                         item->text());
    if (text.isEmpty()) { return; }
    item->setText(text);
}

void MainWindow::on_pushButton_macros_remove_clicked()
{
    QListWidgetItem* item = ui->listWidget_macros->currentItem();
    if (!item) { return; }

    delete item;
}

void MainWindow::on_pushButton_macros_addMultiple_clicked()
{
    QString text = QInputDialog::getMultiLineText(this,
                                                  "Macros",
                                                  "One macro per line");
    foreach (QString line, text.split("\n")) {
        if (line.isEmpty()) { continue; }
        ui->listWidget_macros->addItem(line);
    }
}

void MainWindow::on_listWidget_macros_itemDoubleClicked(QListWidgetItem *item)
{
    sendMacro(item->text());
}

void MainWindow::on_pushButton_log_openFolder_clicked()
{
    QString path = QFileInfo(ui->lineEdit_log_path->text()).path();
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void MainWindow::on_pushButton_log_indicator_clicked()
{
    ui->action_Tools->setChecked(true);
    onToolsVisibilityChanged();
    ui->tabWidget_tools->setCurrentWidget(ui->tab_log);
}

void MainWindow::on_toolButton_sendFile_browse_clicked()
{
    QString path = QFileDialog::getOpenFileName(
                this,
                "File to send",
                ui->lineEdit_log_path->text());

    if (path.isEmpty()) { return; }

    ui->lineEdit_sendFile_path->setText(path);
}

void MainWindow::on_pushButton_sendFile_openFolder_clicked()
{
    QString path = QFileInfo(ui->lineEdit_sendFile_path->text()).path();
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void MainWindow::on_checkBox_sendFile_enable_clicked()
{
    if (ui->checkBox_sendFile_enable->isChecked()) {
        sendFileTimer.start(ui->spinBox_sendFile_ms->value(), this);
    } else {
        if (sendFileTimer.isActive()) { sendFileTimer.stop(); }
    }
}

void MainWindow::onTimedMsgTimer()
{
    static int i = 0;
    QString newline = crlfComboboxText(ui->comboBox_timeMsgs_CRLF->currentIndex());

    if (ui->radioButton_TimedMsgs_sendInt->isChecked()) {
        QString msg = QString("%1 %2").arg(i).arg(newline);
        sendData( msg.toLocal8Bit() );
        i++;
        if (i>100) {
            i = 0;
        }
    } else {
        QString msg = ui->lineEdit_TimedMsgs_msg->text() + newline;
        sendData( msg.toLocal8Bit() );
    }
}

void MainWindow::onSendFileTimer()
{
    QString path = ui->lineEdit_sendFile_path->text();
    if (path.isEmpty()) { return; }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }
    QByteArray data = f.readAll();
    f.close();

    sendData(data);
}

