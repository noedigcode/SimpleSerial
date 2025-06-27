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

#include "gidconsolewidget.h"

#include "Utilities.h"

#include <QElapsedTimer>


GidConsoleWidget::GidConsoleWidget(QWidget *parent) :
    QPlainTextEdit(parent),
    init(true),
    auto_scroll(true)
{
    s = verticalScrollBar();
    cursor = textCursor();
    setCursorTextColor(textColor);

    setFont(Utilities::getMonospaceFont());

    // Line wrapping is done manually in addText()
    this->setLineWrapMode(QPlainTextEdit::NoWrap);

    updateLineWidthInfo();
}

void GidConsoleWidget::addText(QString txt, QColor color)
{
    //procressToPrint({txt, color}); // TODO 2025-06-27 Add switch to enable/disable this
    process({txt, color});
}

bool GidConsoleWidget::isAutoScrollOn()
{
    return auto_scroll;
}

void GidConsoleWidget::autoScroll(bool scroll)
{
    auto_scroll = scroll;
    if (auto_scroll) {
        scrollToBottom();
    }
}

void GidConsoleWidget::scrollToBottom()
{
    s->setValue(s->maximum());
}

bool GidConsoleWidget::lastAddedWasNewline()
{
    return lastWasNewline;
}

int GidConsoleWidget::remainingOnLine()
{
    return mRemainingOnLine;
}

int GidConsoleWidget::currentLineLength()
{
    return mLineLength;
}

void GidConsoleWidget::updateLineWidthInfo()
{
    QFontMetricsF fm(this->font());
    mCharWidth = fm.horizontalAdvance('W');
    int w = this->viewport()->width() - this->verticalScrollBar()->width();
    mMaxLineChars = w / mCharWidth;
    if (mMaxLineChars == 0) { mMaxLineChars = 80; }
}

void GidConsoleWidget::setCursorTextColor(QColor color)
{
    QTextCharFormat f;
    f.setForeground(QBrush(color));
    cursor.setCharFormat(f);
}

void GidConsoleWidget::resizeEvent(QResizeEvent* event)
{
    QPlainTextEdit::resizeEvent(event);

    updateLineWidthInfo();
}

void GidConsoleWidget::procressToPrint(ToPrint tp)
{
    bool start = toPrint.isEmpty();

    int size = 512;
    for (int i = 0; i < tp.txt.count(); i += size) {
        toPrint.append({tp.txt.mid(i, i+size), tp.color});
    }

    if (start) {
        processNext();
    }
}

void GidConsoleWidget::processNext()
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 10) {
        if (toPrint.isEmpty()) { break; }
        process(toPrint.takeFirst());
    }

    if (!toPrint.isEmpty()) {
        QMetaObject::invokeMethod(this, [=]()
        {
            processNext();
        }, Qt::QueuedConnection);
    }
}

void GidConsoleWidget::process(ToPrint tp)
{
    QColor color = tp.color;
    QString txt = tp.txt;

    if (color != textColor) {
        setCursorTextColor(color);
        textColor = color;
    }

    lastWasNewline = false;

    int val = s->value();
    int max = s->maximum();

    bool scroll;
    if (init) {
        scroll = true;
        if (max > 0) { init = false; }
    } else {
        scroll = (val == max);
    }


    int nWritten = 0;
    int readIndex = 0;
    while (nWritten < txt.length()) {

        int from = readIndex;
        bool addNewline = false;
        for (; readIndex < txt.length(); readIndex++) {
            if (txt.at(readIndex) == "\n") {
                mLineLength = 0;
                lastWasNewline = true;
                readIndex++;
                break;
            }
            if (txt.at(readIndex) == '\t') {
                mLineLength += this->tabStopDistance() / mCharWidth + 1;
            } else {
                mLineLength += 1;
            }
            if (mLineLength >= mMaxLineChars) {
                mLineLength = 0;
                addNewline = true;
                readIndex++;
                break;
            }
        }

        int n = readIndex - from;
        cursor.insertText(txt.mid(from, n));
        nWritten += n;
        if (addNewline) {
            cursor.insertText("\n");
            lastWasNewline = true;
        }
    }


    lastWasNewline = txt.endsWith("\n");
    mRemainingOnLine = mMaxLineChars - mLineLength;

    if (scroll && auto_scroll) {
        scrollToBottom();
    }
}


