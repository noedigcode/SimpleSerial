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

#include "gidqt5serial.h"
#include "ui_gidqt5serial.h"


GidQt5Serial::GidQt5Serial(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::GidQt5Serial)
{
    ui->setupUi(this);

    ui->label_PortStatus->clear();

    refreshSerialPortList();

    // Set up Baud rate combo box
    ui->comboBox_BaudRate->addItem(QString::number(QSerialPort::Baud1200));
    ui->comboBox_BaudRate->addItem(QString::number(QSerialPort::Baud2400));
    ui->comboBox_BaudRate->addItem(QString::number(QSerialPort::Baud4800));
    ui->comboBox_BaudRate->addItem(QString::number(QSerialPort::Baud9600));
    ui->comboBox_BaudRate->addItem(QString::number(QSerialPort::Baud19200));
    ui->comboBox_BaudRate->addItem(QString::number(QSerialPort::Baud38400));
    ui->comboBox_BaudRate->addItem(QString::number(QSerialPort::Baud57600));
    ui->comboBox_BaudRate->addItem(QString::number(QSerialPort::Baud115200));
    ui->comboBox_BaudRate->setCurrentIndex(ui->comboBox_BaudRate->count()-1);

    // Set up parity combo box
    parityComboBoxList.append(QSerialPort::NoParity);
    ui->comboBox_Parity->addItem(QVariant::fromValue(QSerialPort::NoParity).toString());
    parityComboBoxList.append(QSerialPort::EvenParity);
    ui->comboBox_Parity->addItem(QVariant::fromValue(QSerialPort::EvenParity).toString());
    parityComboBoxList.append(QSerialPort::OddParity);
    ui->comboBox_Parity->addItem(QVariant::fromValue(QSerialPort::OddParity).toString());
    parityComboBoxList.append(QSerialPort::SpaceParity);
    ui->comboBox_Parity->addItem(QVariant::fromValue(QSerialPort::SpaceParity).toString());
    parityComboBoxList.append(QSerialPort::MarkParity);
    ui->comboBox_Parity->addItem(QVariant::fromValue(QSerialPort::MarkParity).toString());
    ui->comboBox_Parity->setCurrentIndex(0); // Default no parity

    // Set up stop bits combo box
    stopBitsComboBoxList.append(QSerialPort::OneStop);
    ui->comboBox_StopBits->addItem(QVariant::fromValue(QSerialPort::OneStop).toString());
    stopBitsComboBoxList.append(QSerialPort::OneAndHalfStop);
    ui->comboBox_StopBits->addItem(QVariant::fromValue(QSerialPort::OneAndHalfStop).toString());
    stopBitsComboBoxList.append(QSerialPort::TwoStop);
    ui->comboBox_StopBits->addItem(QVariant::fromValue(QSerialPort::TwoStop).toString());
    ui->comboBox_StopBits->setCurrentIndex(0); // Default one stop bit
}

GidQt5Serial::~GidQt5Serial()
{
    delete ui;
}

void GidQt5Serial::closeEvent(QCloseEvent *event)
{
    emit dialogCancelled();
    event->accept();
}

void GidQt5Serial::showEvent(QShowEvent* /*event*/)
{
    ui->lineEdit_PortName->setFocus();
}

void GidQt5Serial::openSerialPort()
{
    if (s.open(QIODevice::ReadWrite)) {
        // Port Opened!
        // NB: Serial port must be opened before it can be configured.

        int baudrate = ui->comboBox_BaudRate->currentText().toInt();
        if ( !s.setBaudRate(baudrate, QSerialPort::AllDirections) ) {
            print("Failed to set baud rate: " + s.errorString());
        }

        QSerialPort::Parity parity = parityComboBoxList.value(ui->comboBox_Parity->currentIndex());
        if ( !s.setParity(parity) ) {
            print("Failed to set parity: " + s.errorString());
        }

        QSerialPort::DataBits databits = (QSerialPort::DataBits)ui->spinBox_DataBits->value();
        if ( !s.setDataBits(databits) ) {
            print("Failed to set data bits: " + s.errorString());
        }

        QSerialPort::StopBits stopbits = stopBitsComboBoxList.value(ui->comboBox_StopBits->currentIndex());
        if ( !s.setStopBits(stopbits) ) {
            print("Failed to set stop bits: " + s.errorString());
        }

        if ( !s.setFlowControl(s.NoFlowControl) ) {
            print("Error setting flow control: " + s.errorString());
        }

        print(QString("Port opened: %1 @ %2 %3 %4 %5 %6")
              .arg(s.portName())
              .arg(QString::number(s.baudRate()))
              .arg(QVariant::fromValue(s.parity()).toString())
              .arg(QVariant::fromValue(s.dataBits()).toString())
              .arg(QVariant::fromValue(s.stopBits()).toString())
              .arg(QVariant::fromValue(s.flowControl()).toString()));

        if (ui->checkBox_setDtrAfterOpen->isChecked()) {
            print("Setting DTR pin");
            s.setDataTerminalReady(true);
        }

        emit portOpened();
        this->hide();

    } else {
        // Error opening port.
        ui->label_PortStatus->setText("Error opening port: " + s.errorString());
    }
}

void GidQt5Serial::refreshSerialPortList()
{
    QStringList previousPorts;
    for (int i = 0; i < ui->listWidget_Ports->count(); i++) {
        previousPorts.append(ui->listWidget_Ports->item(i)->text());
    }
    ui->listWidget_Ports->clear();

    for (const QSerialPortInfo& s : QSerialPortInfo::availablePorts()) {
        QString name = s.portName();

        #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        // NOTE: Qt6 removed isBusy(). This could be done manually by trying to
        // open each port. For now it is not done.
        #else
        if (s.isBusy()) {
            name.append(" (busy)");
        }
        #endif

        QListWidgetItem* item = new QListWidgetItem();
        item->setText(name);
        if (!previousPorts.contains(name)) {
            // Highlight item if new
            item->setBackground(QBrush(Qt::yellow));
            item->setForeground(QBrush(Qt::black));
            // Add to start of list
            ui->listWidget_Ports->insertItem(0, item);
        } else {
            ui->listWidget_Ports->addItem(item);
        }
    }
}

void GidQt5Serial::open()
{
    on_pushButton_OpenPort_clicked();
}

void GidQt5Serial::reOpen()
{
    if (s.isOpen()) {
        s.close();
    }
    openSerialPort();
}

QMap<QString, QString> GidQt5Serial::getSettings()
{
    QMap<QString, QString> s;

    s.insert("baudrate", ui->comboBox_BaudRate->currentText());
    s.insert("parityIndex", QString::number(ui->comboBox_Parity->currentIndex()));
    s.insert("databits", QString::number(ui->spinBox_DataBits->value()));
    s.insert("stopbitsIndex", QString::number(ui->comboBox_StopBits->currentIndex()));
    s.insert("setDtrAfterOpen", QVariant(ui->checkBox_setDtrAfterOpen->isChecked()).toString());

    return s;
}

void GidQt5Serial::setSettings(QMap<QString, QString> settings)
{
    ui->comboBox_BaudRate->setCurrentText(settings.value("baudrate",
                                    ui->comboBox_BaudRate->currentText()));

    ui->comboBox_Parity->setCurrentIndex(settings.value("parityIndex",
                QString::number(ui->comboBox_Parity->currentIndex())).toInt());

    ui->spinBox_DataBits->setValue(settings.value("databits",
                QString::number(ui->spinBox_DataBits->value())).toInt());

    ui->comboBox_StopBits->setCurrentIndex(settings.value("stopbitsIndex",
                QString::number(ui->comboBox_StopBits->currentIndex())).toInt());

    bool setDtrAfterOpen = QVariant(settings.value("setDtrAfterOpen")).toBool();
    ui->checkBox_setDtrAfterOpen->setChecked(setDtrAfterOpen);
}

void GidQt5Serial::setPort(QString port)
{
    ui->lineEdit_PortName->setText(port);
}

void GidQt5Serial::setBaudrate(int baudrate)
{
    ui->comboBox_BaudRate->setCurrentText(QString::number(baudrate));
}

void GidQt5Serial::setParity(QSerialPort::Parity parity)
{
    ui->comboBox_Parity->setCurrentText(QVariant::fromValue(parity).toString());
}

void GidQt5Serial::setDataBits(QSerialPort::DataBits dataBits)
{
    ui->spinBox_DataBits->setValue((int)dataBits);
}

void GidQt5Serial::setStopBits(QSerialPort::StopBits stopBits)
{
    ui->comboBox_StopBits->setCurrentText(
                QVariant::fromValue(stopBits).toString());
}

void GidQt5Serial::on_pushButton_OpenPort_clicked()
{
    if (ui->lineEdit_PortName->text().isEmpty()) {
        ui->label_PortStatus->setText("Specify a serial port.");
        return;
    }
    ui->label_PortStatus->clear();

    if (s.isOpen()) {
        s.close();
    }

    s.setPortName(ui->lineEdit_PortName->text());

    openSerialPort();
}

void GidQt5Serial::on_pushButton_RefreshPorts_clicked()
{
    refreshSerialPortList();
}

void GidQt5Serial::on_pushButton_Cancel_clicked()
{
    emit dialogCancelled();
    this->hide();
}

void GidQt5Serial::on_listWidget_Ports_itemDoubleClicked(QListWidgetItem* /*item*/)
{
    on_pushButton_OpenPort_clicked();
}

void GidQt5Serial::on_listWidget_Ports_itemClicked(QListWidgetItem *item)
{
    ui->lineEdit_PortName->setText(item->text());
}

void GidQt5Serial::on_lineEdit_PortName_returnPressed()
{
    on_pushButton_OpenPort_clicked();
}
