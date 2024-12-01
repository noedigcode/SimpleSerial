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


AboutDialog::AboutDialog(QString settingsText, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    this->resize(Utilities::scaleWithPrimaryScreenScalingFactor(this->size()));

    // Hide the context help button in the title bar
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setWindowTitle(QString("About %1").arg(APP_NAME));

    // Add version text
    QString html = ui->textBrowser->toHtml();
    html.replace("%APP_NAME%", APP_NAME);
    html.replace("%VERSION%", APP_VERSION);
    html.replace("%YEAR_FROM%", APP_YEAR_FROM);
    html.replace("%YEAR%", APP_YEAR);
    html.replace("%SETTINGS_PATH%", settingsText);
    ui->textBrowser->setHtml(html);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::on_pushButton_clicked()
{
    this->hide();
}

