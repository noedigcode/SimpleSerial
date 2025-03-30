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

#ifndef GIDQT5SERIAL_H
#define GIDQT5SERIAL_H

#include <QCloseEvent>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QMap>
#include <QSerialPort>
#include <QSerialPortInfo>


namespace Ui {
class GidQt5Serial;
}

class GidQt5Serial : public QMainWindow
{
    Q_OBJECT

public:
    explicit GidQt5Serial(QWidget *parent = 0);
    ~GidQt5Serial();

    void refreshSerialPortList();
    QSerialPort s;
    void open();
    void reOpen();

    QMap<QString, QString> getSettings();
    void setSettings(QMap<QString, QString> settings);

    void setPort(QString port);
    void setBaudrate(int baudrate);
    void setParity(QSerialPort::Parity parity);
    void setDataBits(QSerialPort::DataBits dataBits);
    void setStopBits(QSerialPort::StopBits stopBits);

signals:
    void print(QString msg);
    void portOpened();
    void dialogCancelled();

private:
    Ui::GidQt5Serial *ui;

    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent *event);

    QList<QSerialPortInfo> serialPortList;
    QList<QSerialPort::Parity> parityComboBoxList;
    QList<QSerialPort::StopBits> stopBitsComboBoxList;

    void openSerialPort();

private slots:
    void on_pushButton_OpenPort_clicked();
    void on_pushButton_RefreshPorts_clicked();
    void on_pushButton_Cancel_clicked();
    void on_listWidget_Ports_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidget_Ports_itemClicked(QListWidgetItem *item);
    void on_lineEdit_PortName_returnPressed();
};

#endif // GIDQT5SERIAL_H
