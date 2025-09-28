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

#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "Utilities.h"
#include "version.h"

#include <QFile>


AboutDialog::AboutDialog(QString settingsText, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    this->resize(Utilities::scaleWithPrimaryScreenScalingFactor(this->size()));

    // Hide the context help button in the title bar
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setWindowTitle(QString("About %1").arg(APP_NAME));

    QString text = ui->label_appname->text();
    text.replace("%APP_NAME%", APP_NAME);
    ui->label_appname->setText(text);

    text = ui->label_appInfo->text();
    text.replace("%VERSION%", APP_VERSION);
    text.replace("%YEAR_FROM%", APP_YEAR_FROM);
    text.replace("%YEAR%", APP_YEAR);
    text.replace("%SETTINGS_PATH%", settingsText);
    ui->label_appInfo->setText(text);

    QString changelog = "Could not load changelog";
    QFile f("://changelog");
    if (f.open(QIODevice::ReadOnly)) {
        changelog = f.readAll();
    }
    ui->textBrowser->setMarkdown(changelog);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::on_pushButton_clicked()
{
    this->hide();
}

