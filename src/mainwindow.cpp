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


MainWindow::MainWindow(StartupOptions options, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->spinBox_maxProcessTimeMs->setValue(dataDisplay.allowedMs);
    ui->spinBox_displayBacklogLengthMs->setValue(dataDisplay.displayBacklogLengthMs);

    updateMacroGuiButtonsEnabled();

    showStartupPage();

    onToolsVisibilityChanged();
    // Default tools tabs
    ui->tabWidget_tools->setCurrentWidget(ui->tab_options);
    ui->tabWidget_options->setCurrentWidget(ui->tab_displayMode);

    ui->comboBox_send->installEventFilter(this);
    ui->console->installEventFilter(this);
    connect(ui->console, &GidConsoleWidget::fontPointSizeChanged,
            this, &MainWindow::onConsoleZoomChanged);

    // Disable combo box auto-complete
    ui->comboBox_send->setCompleter(0);

    loadGeneralSettings();
    setupSerial();
    setupNetwork();

    updateWindowTitle();

    setCommsModeAndUpdateGui(CommsNone);

    // Handle startup options
    if (!options.serialPort.isEmpty()) {
        printOnNewLine("Startup option: Open serial port: " + options.serialPort,
                       systemTextColor());
        serial.setPort(options.serialPort);
        serial.setBaudrate(options.baud);
        serial.setParity(options.parity);
        serial.setDataBits(options.dataBits);
        serial.setStopBits(options.stopBits);
        on_pushButton_startup_openSerialPort_clicked();
        serial.open();
    }
    if (!options.sendFilePath.isEmpty()) {
        printOnNewLine("Startup option: send file: " + options.sendFilePath,
                       systemTextColor());
        printOnNewLine(QString("Frequency: %1 ms").arg(options.sendFileFreqMs),
                       systemTextColor());

        ui->lineEdit_sendFile_path->setText(options.sendFilePath);
        ui->spinBox_sendFile_ms->setValue(options.sendFileFreqMs);
        ui->checkBox_sendFile_enable->setChecked(true);

        on_checkBox_sendFile_enable_clicked();
    }

    ui->console->setTextMovementMarker("🦆");

    setupGuiStyleMenu();
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::getDefaultGuiStyle(QApplication *app)
{
    QString defaultStyle = "windowsvista";
    if (!QStyleFactory::keys().contains(defaultStyle, Qt::CaseInsensitive)) {
        defaultStyle = app->style()->objectName();
    }
    return defaultStyle;
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

            // For key presses on the console, pass them on to the send text
            // box (and switch focus to it), depending on the modifier states.
            // Shift: pass keys, as user might be typing a capital letter.
            // Keypad: pass keys, as user is using the numpad.
            // Other modifiers: do not pass keys, as user may be doing something
            // like a copy operation.
            Qt::KeyboardModifiers mods = keyEvent->modifiers();
            bool nopass = (mods & Qt::ControlModifier) ||
                    (mods & Qt::AltModifier) ||
                    (mods & Qt::MetaModifier);
            if (!nopass) {
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

void MainWindow::showEvent(QShowEvent* /*event*/)
{
    if (firstShow) {
        this->resize(Utilities::scaleWithScreenScalingFactor(this->screen(),
                                                             this->size()));
        firstShow = false;
    }
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

void MainWindow::initActionCheckedSetting(Settings::SettingPtr setting, QAction *action)
{
    // Set the action checked based on the setting, default to the current
    // action state if the setting does not exist.
    action->setChecked(setting->getOrDefault(action->isChecked()).toBool());
    connect(action, &QAction::changed, [=](){
        setting->set(action->isChecked());
    });
}

void MainWindow::initCheckableSetting(Settings::SettingPtr setting, QAbstractButton* widget)
{
    // Set the checkbox checked based on the setting, default to the current
    // state if the setting does not exist.
    widget->setChecked(setting->getOrDefault(widget->isChecked()).toBool());
    connect(widget, &QCheckBox::toggled, [=](){
        setting->set(widget->isChecked());
    });
}

void MainWindow::initLineEditSetting(Settings::SettingPtr setting, QLineEdit* lineEdit)
{
    lineEdit->setText(setting->getOrDefault(lineEdit->text()).toString());
    connect(lineEdit, &QLineEdit::textChanged, [=](){
        setting->set(lineEdit->text());
    });
}

void MainWindow::initSpinBox(Settings::SettingPtr setting, QSpinBox* spinBox)
{
    spinBox->setValue(setting->getOrDefault(spinBox->value()).toInt());
    connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int value)
    {
        setting->set(value);
    });
}

void MainWindow::readMacrosFromSettings()
{
    const Settings::KeyValList macros = settings.macros->get();

    ui->listWidget_macros->clear();
    for (const Settings::KeyVals &keyVals : macros) {
        ui->listWidget_macros->addItem(keyVals.value(settings.macroValue->key()).toString());
    }
}

void MainWindow::saveMacrosToSettings()
{
    Settings::KeyValList macros;

    for (int i = 0; i < ui->listWidget_macros->count(); i++) {
        QListWidgetItem* item = ui->listWidget_macros->item(i);
        Settings::KeyVals keyVals;
        keyVals.insert(settings.macroValue->key(), item->text());
        macros.append(keyVals);
    }

    settings.macros->set(macros);
}

void MainWindow::updateMacroGuiButtonsEnabled()
{
    // Enable/disable macro buttons based on whether a macro is selected in the list

    bool enable = (ui->listWidget_macros->currentItem() != nullptr);

    ui->pushButton_macros_edit->setEnabled(enable);
    ui->pushButton_macros_remove->setEnabled(enable);
    ui->pushButton_macros_send->setEnabled(enable);
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

void MainWindow::print(QString msg, QColor color)
{
    ui->console->addText(msg + "\n", color);
}

void MainWindow::printOnNewLine(QString msg, QColor color)
{
    if (!ui->console->cursorIsOnNewLine()) {
        print("", color);
    }
    print(msg, color);
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

    for (int i=0; i < data.length(); i++) {

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
                    if (mLastRxDataAddedToConsoleWasNewline) {
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

        // If showing send data, add a newline before it if set
        if ((dataDir == DataSend)
            && (!ui->console->cursorIsOnNewLine()) // Prevent unnecessary empty lines
            && (i == 0))
        {
            if (ui->checkBox_showSentDataOnSeparateLine->isChecked()) {
                addTextToConsoleAndLogIfEnabled("\n", normalTextColor());
            }
        }

        if (printTimestamp) {
            QString t;
            if (!ui->console->cursorIsOnNewLine()) {
                t += "\n";
            }
            QString space = ui->checkBox_timestamps_addTabAfterTimestamp->isChecked() ?
                        "\t" : " ";
            t += QString("%1:%2")
                    .arg(QTime::currentTime().toString("hh:mm:ss:zzz"))
                    .arg(space);
            addTextToConsoleAndLogIfEnabled(t, timestampColor());
            lastWasHex = false;
            timestampShown = true;
        }

        if (ui->radioButton_displayMode_hex->isChecked()) {
            // Hex mode.
            // Hex mode trumps the rest. All characters are converted to hex.
            // Newlines are treated as normal hex data.
            outputHex = true;
            outputNormal = false;
        } else {
            // ASCII text mode.
            // Take special characters hex into account
            bool special = ((c < 32) || (c == 127));
            bool notCrLfTab = (c != '\t') && (c != '\n') && (c != '\r');
            if (special && notCrLfTab) {
                if (ui->checkBox_showHexForSpecialChars->isChecked()) {
                    // Show special character in hex
                    outputHex = true;
                    outputNormal = false;
                }
            }
        }

        QBrush background = (dataDir == DataSend) ? sendBackground() : QBrush();

        if (outputHex) {
            QString hex = QString("%1").arg(c, 2, 16, QChar('0')).toUpper();;
            bool virtuallyAtLineStart =    printTimestamp
                                        || ui->console->cursorIsOnNewLine();
            addNonBreakingTextToConsole(hex, hexColor(), background,
                                        virtuallyAtLineStart, true);
            lastWasHex = true;
        }

        // Normal text output
        if (outputNormal) {

            if (c == '\n') {
                if (ui->checkBox_showCrLfHex->isChecked()) {
                    if (lastWasHex) {
                        addTextToConsoleAndLogIfEnabled(" ", normalTextColor());
                        lastWasHex = false;
                    }
                    addNonBreakingTextToConsole("<LF>", hexColor(), background);
                }
                if (!ui->checkBox_crLfNewline->isChecked()) {
                    // Prevent outputting newline
                    outputNormal = false;
                }
            } else if (c == '\r') {
                outputNormal = false;
                if (ui->checkBox_showCrLfHex->isChecked()) {
                    if (lastWasHex) {
                        addTextToConsoleAndLogIfEnabled(" ", normalTextColor());
                        lastWasHex = false;
                    }
                    addNonBreakingTextToConsole("<CR>", hexColor(), background);
                }
            }

            if (outputNormal) {
                if (lastWasHex) {
                    addTextToConsoleAndLogIfEnabled(" ", normalTextColor());
                    lastWasHex = false;
                }
                addTextToConsoleAndLogIfEnabled(QString(QChar(c)),
                                                normalTextColor(), background);
            }
        }

        // Newline after showing send data
        if ((dataDir == DataSend)
            && (!ui->console->cursorIsOnNewLine()) // Prevent unnecessary empty lines
            && (i == data.length()-1))
        {
            if (ui->checkBox_showSentDataOnSeparateLine->isChecked()) {
                addTextToConsoleAndLogIfEnabled("\n", normalTextColor());
                lastWasHex = false;
            }
        }

        mLastRxDataAddedToConsoleWasNewline = (c == '\n');
    }
}

void MainWindow::addNonBreakingTextToConsole(QString text, QColor color,
                                             QBrush background,
                                             bool virtuallyAtLineStart,
                                             bool addSpaceBefore)
{
    bool addedNewline = false;
    int lenToAdd = text.length();
    if (addSpaceBefore) { lenToAdd += 1; }
    if (ui->console->remainingOnLine() < lenToAdd) {
        text.prepend("\n");
        addedNewline = true;
    }

    // Add space before if set, but only if we are not at the start of a line
    if (addSpaceBefore) {
        bool atStartOfLine = virtuallyAtLineStart || addedNewline;
        if (!atStartOfLine) {
            text.prepend(" ");
        }
    }

    addTextToConsoleAndLogIfEnabled(text, color, background);
}

void MainWindow::addTextToConsoleAndLogIfEnabled(QString text, QColor color,
                                                 QBrush background)
{
    ui->console->addText(text, color, background);
    if (ui->radioButton_log_asDisplayed->isChecked()) {
        log(text.toLocal8Bit());
    }
}

QColor MainWindow::normalTextColor()
{
    return ui->console->palette().color(QPalette::Text);
}

QColor MainWindow::timestampColor()
{
    return Qt::blue;
}

QColor MainWindow::hexColor()
{
    return Qt::red;
}

QColor MainWindow::systemTextColor()
{
    return Qt::darkGray;
}

QBrush MainWindow::sendBackground()
{
    return QBrush(QColor(0, 0, 255, 64));
}

void MainWindow::setCommsModeAndUpdateGui(CommsMode mode)
{
    mCommsMode = mode;

    bool serial = (mode == CommsSerial);
    ui->action_Re_Open_SerialPort->setVisible(serial);
    ui->action_Close_SerialPort->setVisible(serial);
    ui->action_Close_SerialPort_toolbar->setVisible(serial);
    ui->action_Open_Serial_Port->setVisible(serial);
    ui->action_Set_DTR->setVisible(serial);
    ui->action_Set_DTR_toolbar->setVisible(serial);
    ui->action_Set_RTS->setVisible(serial);

    bool tcpServer = (mode == CommsTcpServer);
    ui->action_Restart_TCP_Server->setVisible(tcpServer);
    ui->action_Stop_TCP_Server->setVisible(tcpServer);

    bool tcpClient = (mode == CommsTcpClient);
    ui->action_Reconnect_to_TCP_Server->setVisible(tcpClient);
    ui->action_Disconnect_from_TCP_Server->setVisible(tcpClient);
}

void MainWindow::onDataReceived(QByteArray data)
{
    dataDisplay.processData(data, DataReceive);

    // Display number of received bytes
    numBytesRx += data.length();
    updateCounterLabels();

    // Log raw data if enabled
    if (ui->radioButton_log_raw->isChecked()) {
        log(data);
    }
    flushLog();

    // Auto-reply
    if (ui->checkBox_AutoReply_Enable->isChecked()) {
        for (int i=0; i < data.length(); i++) {
            mAutoReplyBuffer.append( data.at(i) );
            while (mAutoReplyBuffer.length() > ui->lineEdit_AutoReply_rx->text().length()) {
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
}

void MainWindow::sendData(QByteArray data, bool allowEscapeSequenceReplace)
{
    if (allowEscapeSequenceReplace) {
        if (ui->checkBox_sending_replaceEscapeSequences->isChecked()) {
            // Hex
            static QByteArray hex("0123456789abcdef");
            QByteArray tosend;
            QByteArray lowerData = data.toLower();
            QByteArray buffer; // Collect bytes to be converted to hex
            int state = 0;
            for (int i = 0; i < data.length(); i++) {
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

    numBytesTx += data.length();
    updateCounterLabels();

    if (ui->checkBox_showSentDataInConsole->isChecked()) {
        dataDisplay.processData(data, DataSend);
    }

    flushLog();
}

void MainWindow::setupSerial()
{
    // Serial settings
    QMap<QString, QString> serialSettings = settings.getGroupKeyVals("serial");
    serial.setSettings(serialSettings);

    connect(&(serial.s), &QSerialPort::readyRead,
            this, &MainWindow::onSerialReadyRead);
    connect(&(serial.s), &QSerialPort::errorOccurred,
            this, &MainWindow::onSerialError);
    connect(&serial, &GidQt5Serial::print,
            this, &MainWindow::printSerial);
    connect(&serial, &GidQt5Serial::portOpened,
            this, &MainWindow::onSerialPortOpened);
    connect(&(serial.s), &QSerialPort::dataTerminalReadyChanged,
            this, [=](bool set)
    {
        ui->action_Set_DTR->setChecked(set);
        ui->action_Set_DTR_toolbar->setChecked(set);
    });
    connect(&(serial.s), &QSerialPort::requestToSendChanged,
            this, [=](bool set)
    {
        ui->action_Set_RTS->setChecked(set);
    });

    serial.setWindowModality(Qt::ApplicationModal);
    serial.setWindowTitle(QString("%1 %2").arg(APP_NAME).arg(APP_VERSION));
    serial.resize(Utilities::scaleWithScreenScalingFactor(serial.screen(),
                                                          serial.size()));
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

    ui->label_bytesDropped->setText(QString("%1").arg(numBytesDroppedFromDisplay));
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

    // Save serial settings
    settings.setGroupKeyVals("serial", serial.getSettings());

    ui->action_Set_DTR->setChecked(serial.s.isDataTerminalReady());
    ui->action_Set_DTR_toolbar->setChecked(serial.s.isDataTerminalReady());
    ui->action_Set_RTS->setChecked(serial.s.isRequestToSend());

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
    printOnNewLine("[tcp] " + msg, systemTextColor());
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
    printOnNewLine("[udp] " + msg, systemTextColor());
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

void MainWindow::onConsoleZoomChanged()
{
    settings.consoleFontPointSize->set(ui->console->getFontPointSize());
}

void MainWindow::printSerial(QString msg)
{
    printOnNewLine("[serial] " + msg, systemTextColor());
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

void MainWindow::on_action_Close_SerialPort_toolbar_triggered()
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
    numBytesDroppedFromDisplay = 0;
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
        aboutDialog = new AboutDialog(settings.qSettings.fileName(), this);
        aboutDialog->setWindowModality(Qt::ApplicationModal);
    }
    aboutDialog->show();
}

void MainWindow::on_comboBox_SendCRLF_currentIndexChanged(int index)
{
    settings.crLf->set(index);
}

void MainWindow::on_actionWindow_Always_On_Top_toggled(bool arg1)
{
    this->setWindowFlag(Qt::WindowStaysOnTopHint, arg1);
    this->show();
}

void MainWindow::loadGeneralSettings()
{
    // Auto scroll
    initActionCheckedSetting(settings.autoScroll, ui->actionAuto_Scroll);
    onAutoScrollChanged();

    // Send CR/LF
    ui->comboBox_SendCRLF->blockSignals(true);
    ui->comboBox_SendCRLF->setCurrentIndex(settings.crLf->getOrDefault(0).toInt());
    ui->comboBox_SendCRLF->blockSignals(false);

    // Display mode
    initCheckableSetting(settings.displayModeText, ui->radioButton_displayMode_text);
    initCheckableSetting(settings.displayModeHex, ui->radioButton_displayMode_hex);

    // Text mode settings
    initCheckableSetting(settings.hexSpecial, ui->checkBox_showHexForSpecialChars);
    initCheckableSetting(settings.showCrLfHex, ui->checkBox_showCrLfHex);
    initCheckableSetting(settings.newlineForCrLf, ui->checkBox_crLfNewline);

    initSpinBox(settings.tabWidth, ui->spinBox_tabWidth);
    ui->console->setTabCharacterWidth(settings.tabWidth->getOrDefault(
                                          ui->spinBox_tabWidth->value())
                                      .toInt());

    initCheckableSetting(settings.showDuck, ui->checkBox_showDuck);
    ui->console->enableTextMovementMarker(settings.showDuck->getOrDefault().toBool());

    // Replace escape sequences setting
    initCheckableSetting(settings.replaceEscapeSequences,
                        ui->checkBox_sending_replaceEscapeSequences);

    // Show sent data
    initCheckableSetting(settings.showSentData, ui->checkBox_showSentDataInConsole);
    initCheckableSetting(settings.sentDataOnSeparateLine,
                         ui->checkBox_showSentDataOnSeparateLine);

    // TCP server settings
    initLineEditSetting(settings.tcpServerPort, ui->lineEdit_tcpServer_port);

    // TCP client settings
    initLineEditSetting(settings.tcpClientIp, ui->lineEdit_tcpClient_ipAddress);
    initLineEditSetting(settings.tcpClientPort, ui->lineEdit_tcpClient_port);

    // UDP settings
    initCheckableSetting(settings.udpBindForListen, ui->checkBox_udp_bindForListening);
    initLineEditSetting(settings.udpBindPort, ui->lineEdit_udp_listenPort);
    initCheckableSetting(settings.udpSendBroadcast, ui->checkBox_udp_broadcast);
    initLineEditSetting(settings.udpSendIp, ui->lineEdit_udp_sendIpAddress);
    initLineEditSetting(settings.udpSendPort, ui->lineEdit_udp_sendPort);

    // Send file settings
    initLineEditSetting(settings.sendFilePath, ui->lineEdit_sendFile_path);
    initSpinBox(settings.sendFileFrequencyMs, ui->spinBox_sendFile_ms);
    initCheckableSetting(settings.sendFileExcludeEndingNewline, ui->checkBox_sendFile_excludeEndingNewline);
    initCheckableSetting(settings.sendFileSendMsgIfFileEmpty, ui->checkBox_sendFile_sendMsgIfEmpty);
    initLineEditSetting(settings.sendFileMsgIfEmpty, ui->lineEdit_sendFile_msgIfEmpty);

    // Timestamps
    initCheckableSetting(settings.timestampsEnabled, ui->checkBox_timestamps_enable);
    initCheckableSetting(settings.timestampsOnlyAfterNewlines, ui->checkBox_timestamps_after_newline);
    initSpinBox(settings.timestampGroupTimeMs, ui->spinBox_timestamps_time_ms);
    initCheckableSetting(settings.timestampAddTabAfter, ui->checkBox_timestamps_addTabAfterTimestamp);;

    // Macros send CR/LF combo box
    ui->comboBox_macros_append->blockSignals(true);
    ui->comboBox_macros_append->setCurrentIndex(
                settings.macrosSendCrlf->getOrDefault().toInt());
    ui->comboBox_macros_append->blockSignals(false);

    // Macros list
    readMacrosFromSettings();

    // Console font size (i.e. zoom)

    if (settings.consoleFontPointSize->isSet()) {
        int pointSize = settings.consoleFontPointSize->getOrDefault(9).toInt();
        ui->console->setFontPointSize(pointSize);
    }
}

void MainWindow::setupGuiStyleMenu()
{
    mStyleMenu = ui->menuView->addMenu("GUI Style");

    addGuiStyleAction("Default", "");
    mStyleMenu->addSeparator();
    for (const QString& style : QStyleFactory::keys()) {
        addGuiStyleAction(style, style);
    }

    updateGuiStyleMenuChecks();
}

void MainWindow::addGuiStyleAction(QString title, QString style)
{
    QAction* action = mStyleMenu->addAction(title, [=]() {
        guiStyleActionTriggered(style);
    });
    action->setData(style);
    action->setCheckable(true);
    styleActions.append(action);
}

void MainWindow::guiStyleActionTriggered(QString style)
{
    settings.guiStyle->set(style);

    if (style.isEmpty()) {
        style = getDefaultGuiStyle(qApp);
    }
    qApp->setStyle(QStyleFactory::create(style));

    updateGuiStyleMenuChecks();
}

void MainWindow::updateGuiStyleMenuChecks()
{
    QString style = qApp->style()->objectName(); // settings.guiStyle->getOrDefault().toString();

    for (QAction* action : styleActions) {
        action->setChecked(action->data().toString().toLower() == style.toLower());
    }
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
    ui->pushButton_startup_openSerialPort->setFocus();
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
    ui->lineEdit_tcpServer_port->setFocus();
    ui->lineEdit_tcpServer_port->selectAll();
}

void MainWindow::on_pushButton_startup_tcpClient_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_tcpClient);
    ui->lineEdit_tcpClient_ipAddress->setFocus();
    ui->lineEdit_tcpClient_ipAddress->selectAll();
}

void MainWindow::on_pushButton_startup_udp_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_udp);
    ui->lineEdit_udp_listenPort->setFocus();
    ui->lineEdit_udp_listenPort->selectAll();
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

    saveMacrosToSettings();
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

    saveMacrosToSettings();
}

void MainWindow::on_pushButton_macros_remove_clicked()
{
    QListWidgetItem* item = ui->listWidget_macros->currentItem();
    if (!item) { return; }

    delete item;

    saveMacrosToSettings();
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

    saveMacrosToSettings();
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
    QByteArray data;
    if (f.open(QIODevice::ReadOnly)) {
        data = f.readAll();
        f.close();
    }
    // Data will be empty if file could not be opened

    // Remove ending LF or CRLF if setting set
    if (ui->checkBox_sendFile_excludeEndingNewline->isChecked()) {
        if (data.endsWith('\n')) {
            data.remove(data.length() - 1, 1);
            if (data.endsWith('\r')) {
                data.remove(data.length() - 1, 1);
            }
        }
    }


    if (!data.isEmpty()) {
        // If data not empty, send as-is with no escape sequence replacement to
        // respect file content.
        sendData(data, false);
    } else {
        // If data empty (either file empty or could not be loaded), send
        // preset message if setting set
        if (ui->checkBox_sendFile_sendMsgIfEmpty->isChecked()) {
            data = ui->lineEdit_sendFile_msgIfEmpty->text().toUtf8();
            if (!data.isEmpty()) {
                // Allow escape sequence replacement
                sendData(data);
            }
        }
    }
}

void MainWindow::on_spinBox_maxProcessTimeMs_valueChanged(int value)
{
    dataDisplay.allowedMs = value;
}

void MainWindow::on_spinBox_displayBacklogLengthMs_valueChanged(int value)
{
    dataDisplay.displayBacklogLengthMs = value;
}

void MainWindow::DataDisplayProcessor::processData(QByteArray data,
                                                   MainWindow::DataDirection dir)
{
    bool start = (rxbuffer.isEmpty() && txbuffer.isEmpty());

    if (dir == MainWindow::DataReceive) {
        rxbuffer += data;
    } else {
        txbuffer += data;
    }

    if (start) { processNext(); }
}

void MainWindow::DataDisplayProcessor::processNext()
{
    int sizeMin = 32;

    QElapsedTimer timer;
    timer.start();
    int countBefore = rxbuffer.length() + txbuffer.length();
    while (timer.elapsed() < allowedMs) {
        qint64 msBefore = timer.elapsed();

        // Split number of bytes to be processed between incoming and outgoing.
        int nrx = bufferProcessSize / 2;
        int ntx = nrx;
        if (txbuffer.length() < ntx) {
            nrx += ntx - txbuffer.length();
        }
        if (rxbuffer.length() < nrx) {
            ntx += nrx - rxbuffer.length();
        }

        // Process incoming
        QByteArray data = rxbuffer.left(nrx);
        rxbuffer.remove(0, nrx);
        mainWindow->addDataToConsole(data, DataReceive);
        int dataCount = data.length();

        // Process outgoing
        data = txbuffer.left(ntx);
        txbuffer.remove(0, ntx);
        mainWindow->addDataToConsole(data, DataSend);
        dataCount += data.length();

        qint64 msAfter = timer.elapsed();

        // Adjust buffer process size (number of bytes processed per cycle) to
        // keep within allowed time slot
        int dt = qMax(qint64(1), msAfter - msBefore);
        if (dt > 0) {
            int rate = dataCount / dt;
            bufferProcessSize = rate * allowedMs;
            if (bufferProcessSize < sizeMin) { bufferProcessSize = sizeMin; }
        }

        if ((msAfter + dt) > allowedMs) { break; }
        if (rxbuffer.isEmpty() && txbuffer.isEmpty()) { break; }
    }
    lastProcessMs = timer.elapsed();

    // Drop calculation
    int countAfter = rxbuffer.length() + txbuffer.length();
    int bufmax = 0;
    if (countAfter > 0) {
        int ms = timer.elapsed();
        if (ms > 0) {
            float bpms = (countBefore - countAfter) / (float)ms;
            bufmax = bpms * displayBacklogLengthMs;
        }
    }

    if (countAfter > bufmax) {
        int drop = countAfter - bufmax;
        // First drop from send display buffer
        int dropTx = qMin(txbuffer.length(), drop);
        int dropRx = qMin(rxbuffer.length(), drop - dropTx);
        txbuffer.remove(0, dropTx);
        rxbuffer.remove(0, dropRx);
        mainWindow->numBytesDroppedFromDisplay += drop;
        mainWindow->updateCounterLabels();
    }

    // Update GUI information
    mainWindow->ui->label_displayProcessBufferSize->setText(
                QString("%1").arg(bufferProcessSize));
    mainWindow->ui->label_lastDisplayProcessTime->setText(
                QString("%1 ms").arg(lastProcessMs));
    int percent = 0;
    if (bufmax) {
        percent = (float)(rxbuffer.length() + txbuffer.length())
                / (float)bufmax * 100.0;
    }
    mainWindow->ui->label_backlogFill->setText(
                QString("%1 bytes (%2 %)")
                .arg(rxbuffer.length() + txbuffer.length())
                .arg(percent));

    // Queue next call to this function so rest of GUI has a chance to run.
    if (!rxbuffer.isEmpty() || !txbuffer.isEmpty()) {
        QMetaObject::invokeMethod(mainWindow, [=]()
        {
            processNext();
        }, Qt::QueuedConnection);
    }
}

void MainWindow::on_action_Set_DTR_toggled(bool set)
{
    serial.s.setDataTerminalReady(set);
}

void MainWindow::on_action_Set_DTR_toolbar_toggled(bool set)
{
    serial.s.setDataTerminalReady(set);
}

void MainWindow::on_action_Set_RTS_toggled(bool set)
{
    serial.s.setRequestToSend(set);
}

void MainWindow::on_comboBox_macros_append_currentIndexChanged(int index)
{
    settings.macrosSendCrlf->set(index);
}

void MainWindow::on_listWidget_macros_currentItemChanged(QListWidgetItem* /*current*/,
                                                         QListWidgetItem* /*previous*/)
{
    updateMacroGuiButtonsEnabled();
}

void MainWindow::on_lineEdit_tcpServer_port_returnPressed()
{
    on_pushButton_tcpServer_start_clicked();
}

void MainWindow::on_lineEdit_tcpClient_ipAddress_returnPressed()
{
    ui->lineEdit_tcpClient_port->setFocus();
    ui->lineEdit_tcpClient_port->selectAll();
}

void MainWindow::on_lineEdit_tcpClient_port_returnPressed()
{
    on_pushButton_tcpClient_connect_clicked();
}

void MainWindow::on_spinBox_tabWidth_valueChanged(int value)
{
    ui->console->setTabCharacterWidth(value);
}

void MainWindow::on_checkBox_showDuck_toggled(bool checked)
{
    ui->console->enableTextMovementMarker(checked);
}

