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
    // Line wrapping is done manually in addText()
    this->setLineWrapMode(QPlainTextEdit::NoWrap);

    setFont(Utilities::getMonospaceFont());

    updateLineWidthInfo();
}

void GidConsoleWidget::addText(QString txt, QColor color, QBrush background)
{
    //procressToPrint({txt, color, background}); // TODO 2025-06-27 Add switch to enable/disable this
    process({txt, color, background});
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

void GidConsoleWidget::setFont(const QFont &font)
{
    int oldPointSize = this->font().pointSize();
    QPlainTextEdit::setFont(font);
    if (oldPointSize != font.pointSize()) {
        onFontPointSizeChanged();
    } else {
        updateLineWidthInfo();
    }
}

int GidConsoleWidget::getFontPointSize()
{
    return this->font().pointSize();
}

void GidConsoleWidget::setFontPointSize(int pointSize)
{
    QFont font = this->font();
    font.setPointSize(pointSize);
    setFont(font);
}

void GidConsoleWidget::setTabCharacterWidth(int chars)
{
    if (chars < 0) { return; }
    mCharsPerTab = chars;
    updateLineWidthInfo();
}

void GidConsoleWidget::enableTextMovementMarker(bool enable)
{
    mEnableTextMovementMarker = enable;
}

void GidConsoleWidget::setTextMovementMarker(QString marker)
{
    mTextMovementMarker = marker;
}

void GidConsoleWidget::zoomIn(int range)
{
    QPlainTextEdit::zoomIn(range);
    onFontPointSizeChanged();
}

void GidConsoleWidget::zoomOut(int range)
{
    QPlainTextEdit::zoomOut(range);
    onFontPointSizeChanged();
}

void GidConsoleWidget::updateLineWidthInfo()
{
    QFontMetricsF fm(this->font());
    mCharWidth = fm.horizontalAdvance('W');

    // Characters per line
    int w = this->viewport()->width() - this->verticalScrollBar()->width();
    mMaxLineChars = w / mCharWidth;
    if (mMaxLineChars == 0) { mMaxLineChars = 80; }

    // Update tab stop distance from character width
    this->setTabStopDistance(mCharWidth * mCharsPerTab);

    mMaxRows = this->viewport()->height() / fm.height();
}

bool GidConsoleWidget:: isItTimeToAddTextMovementMarker()
{
    mRowCounter++;
    if (mEnableTextMovementMarker && this->isVisible()) {
        if (mRowCounter >= mMaxRows - 4) {
            mRowCounter = 0;
            return true;
        }
    }
    return false;
}

void GidConsoleWidget::setCursorTextColor(QColor color, QBrush background)
{
    QTextCharFormat f;
    f.setForeground(QBrush(color));
    f.setBackground(background);
    mCursor.setCharFormat(f);
}

void GidConsoleWidget::resizeEvent(QResizeEvent* event)
{
    // Before resizing determine whether we are scrolled to the bottom.
    bool scroll = (verticalScrollBar()->value() == verticalScrollBar()->maximum());

    QPlainTextEdit::resizeEvent(event);

    updateLineWidthInfo();

    // If we were scrolled to the bottom before resizing, and auto scroll is on,
    // restore the scroll position to the bottom.
    if (mAutoScroll && scroll) {
        scrollToBottom();
    }
}

void GidConsoleWidget::wheelEvent(QWheelEvent *e)
{
    QPlainTextEdit::wheelEvent(e);
    if (e->modifiers() & Qt::ControlModifier) {
        // Ctrl + mouse wheel = zoom
        onFontPointSizeChanged();
    }
}

void GidConsoleWidget::procressToPrint(ToPrint tp)
{
    bool start = toPrint.isEmpty();

    int size = 512;
    for (int i = 0; i < tp.txt.count(); i += size) {
        toPrint.append({tp.txt.mid(i, i+size), tp.color, tp.backround});
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

    QString txt = tp.txt;

    setCursorTextColor(tp.color, tp.backround);

    // Workaround for scrolling when widget is not full of text yet.
    bool scroll;
    if (mScrollInit) {
        scroll = true;
        if (verticalScrollBar()->maximum() > 0) { mScrollInit = false; }
    } else {
        scroll = (verticalScrollBar()->value() == verticalScrollBar()->maximum());
    }


    QString towrite;
    for (int readIndex = 0; readIndex < txt.length(); readIndex++) {

        int toadd = 0;
        QChar c = txt.at(readIndex);
        if (c == '\n') {
            toadd = 0;
            mLineLength = 0;

            if (isItTimeToAddTextMovementMarker()) {
                towrite += "\n" + mTextMovementMarker;
            }
        } else if (c == '\t') {
            toadd = mCharsPerTab - (mLineLength % mCharsPerTab);
        } else {
            toadd = 1;
        }

        if (mLineLength + toadd > mMaxLineChars) {
            towrite += '\n';
            mLineLength = toadd;

            if (isItTimeToAddTextMovementMarker()) {
                towrite += mTextMovementMarker + "\n";
            }
        } else {
            mLineLength += toadd;
        }
        towrite += c;
    }
    mCursor.insertText(towrite);

    mRemainingOnLine = mMaxLineChars - mLineLength;

    if (scroll && mAutoScroll) {
        scrollToBottom();
    }
}

void GidConsoleWidget::onFontPointSizeChanged()
{
    updateLineWidthInfo();
    emit fontPointSizeChanged();
}


