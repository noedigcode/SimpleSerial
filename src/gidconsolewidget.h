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

#ifndef GIDCONSOLEWIDGET_H
#define GIDCONSOLEWIDGET_H

#include <QObject>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextCursor>

class GidConsoleWidget : public QPlainTextEdit
{
    Q_OBJECT
public:
    GidConsoleWidget(QWidget *parent = 0);

    void addText(QString txt, QColor color = Qt::black);
    bool isAutoScrollOn();
    void autoScroll(bool scroll);
    void scrollToBottom();

    bool lastAddedWasNewline();
    int remainingOnLine();
    int currentLineLength();

private:
    bool init;
    QScrollBar* s;
    QTextCursor cursor;
    bool auto_scroll;
    QColor textColor {Qt::black};
    bool lastWasNewline = false;

    int mMaxLineChars = 80;
    int mLineLength = 0;
    int mRemainingOnLine = 0;
    float mCharWidth = 1;
    void updateLineWidthInfo();

    void setCursorTextColor(QColor color);

    void resizeEvent(QResizeEvent* event);
};

#endif // GIDCONSOLEWIDGET_H
