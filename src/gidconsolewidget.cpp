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
    QPlainTextEdit(parent)
{
    mCursor = textCursor();

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
    return mAutoScroll;
}

void GidConsoleWidget::autoScroll(bool scroll)
{
    mAutoScroll = scroll;
    if (mAutoScroll) {
        scrollToBottom();
    }
}

void GidConsoleWidget::scrollToBottom()
{
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

bool GidConsoleWidget::cursorIsOnNewLine()
{
    return (currentLineLength() == 0);
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
    mCursor.setCharFormat(f);
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
    /* Some processing is done to improve the efficiency of QPlainTextEdit,
     * which is used to display text.
     *
     * QPlainTextEdit gets very slow when a block of text gets too large with
     * no newlines. To mitigate this, a newline is inserted when a line reaches
     * the widget's edge, i.e. becomes the width of the widget. This is in
     * effect manual line wrapping.
     */

    QColor color = tp.color;
    QString txt = tp.txt;

    setCursorTextColor(color);

    // Workaround for scrolling when widget is not full of text yet.
    bool scroll;
    if (mScrollInit) {
        scroll = true;
        if (verticalScrollBar()->maximum() > 0) { mScrollInit = false; }
    } else {
        scroll = (verticalScrollBar()->value() == verticalScrollBar()->maximum());
    }


    int nWritten = 0;
    int readIndex = 0;
    while (nWritten < txt.length()) {

        int from = readIndex;
        bool addNewline = false;
        for (; readIndex < txt.length(); readIndex++) {
            if (txt.at(readIndex) == "\n") {
                mLineLength = 0;
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
        mCursor.insertText(txt.mid(from, n));
        nWritten += n;
        if (addNewline) {
            mCursor.insertText("\n");
        }
    }

    mRemainingOnLine = mMaxLineChars - mLineLength;

    if (scroll && mAutoScroll) {
        scrollToBottom();
    }
}


