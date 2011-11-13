/************************************************************************
 *                                                                      *
 * Author: Lukasz Marek <lukasz.m.luki@gmail.com>                       *
 *                                                                      *
 * This file is part of BlackMilord.                                    *
 *                                                                      *
 * BlackMilord is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * BlackMilord is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the         *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with BlackMilord. If not, see http://www.gnu.org/licenses/     *
 *                                                                      *
 ************************************************************************/

#include "PlainTextEditor.h"
#include "Gui.h"
#include "StatusBar.h"
#include <QDebug>
#include <QMenu>
#include <QLayout>
#include <QApplication>
#include <QTextCursor>
#include <QTextDocument>

#include <AspellWrapper.h>
#include <HighlighterManager.h>
#include <Book.h>

PlainTextEditor::PlainTextEditor(QWidget * parent) :
    QPlainTextEdit(parent),
    m_undoStack(this)
{
    connect(&Preferences::instance(), SIGNAL(settingsChanged()), this, SLOT(applySettings()));
    connect(document(), SIGNAL(contentsChange(int, int, int)), this, SLOT(contentsChangeSlot(int, int, int)));
    connect(document(), SIGNAL(contentsChanged()), this, SLOT(contentsChangedSlot()));
    connect(&m_undoStack, SIGNAL(canUndo(bool)), this, SLOT(canUndoSlot(bool)));
    connect(&m_undoStack, SIGNAL(canRedo(bool)), this, SLOT(canRedoSlot(bool)));
    connect(this, SIGNAL(updateRequest(const QRect &, int)), this, SLOT(updateRequestSlot(const QRect &, int)));
    setUndoRedoEnabled(false);
    installEventFilter(this);
}

PlainTextEditor::~PlainTextEditor()
{
}

int PlainTextEditor::firstVisibleBlock() const
{
    return QPlainTextEdit::firstVisibleBlock().blockNumber();
}

bool PlainTextEditor::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == this) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->matches(QKeySequence::Undo)) {
                if (m_undoStack.canUndo()) {
                    undo();
                }
                return true;
            }
            else if (keyEvent->matches(QKeySequence::Redo)) {
                if (m_undoStack.canRedo()) {
                    redo();
                }
                return true;
            }
        }
        return false;
    }
    return QPlainTextEdit::eventFilter(watched, event);
}

void PlainTextEditor::contextMenuEvent(QContextMenuEvent * event)
{
    QMenu *menu = new QMenu();

    QAction *action = menu->addAction(tr("Undo"), this, SLOT(undo()), QKeySequence::Undo);
    action->setEnabled(m_undoStack.canUndo());
    action = menu->addAction(tr("Redo"), this, SLOT(redo()), QKeySequence::Redo);
    action->setEnabled(m_undoStack.canRedo());

    menu->addSeparator();
    action = menu->addAction(tr("Copy"), this, SLOT(copy()), QKeySequence::Copy);
    action->setEnabled(hasSelection());
    action = menu->addAction(tr("Cut"), this, SLOT(cut()), QKeySequence::Cut);
    action->setEnabled(hasSelection());
    action = menu->addAction(tr("Paste"), this, SLOT(paste()), QKeySequence::Paste);
    action->setEnabled(canPaste());

    menu->addSeparator();
    menu->addAction(tr("Select All"), this, SLOT(selectAll()), QKeySequence::SelectAll);

    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        if (ASpellWrapper::instance().isLoaded()) {
            cursor = cursorForPosition(event->pos());
            setTextCursor(cursor);
            cursor.select(QTextCursor::WordUnderCursor);
            if (!ASpellWrapper::instance().checkWord(cursor.selectedText())) {
                const QStringList &hints = ASpellWrapper::instance().hints(cursor.selectedText());
                if (hints.size() > 0) {
                    connect(menu, SIGNAL(triggered(QAction*)), SLOT(applyHintSlot(QAction *)));
                    menu->addSeparator();
                    int size = hints.size() < 10 ? hints.size() : 10;
                    for (int i = 0; i < size; ++i) {
                        QAction *action = menu->addAction(hints[i]);
                        action->setData(true);
                    }
                }
            }
        }
    }

    menu->exec(event->globalPos());
    delete menu;
}

void PlainTextEditor::updateRequestSlot(const QRect &rect, int dy)
{
    Q_UNUSED(rect);
    if (dy != 0) {
        int firstVisibleBlockNumber = QPlainTextEdit::firstVisibleBlock().blockNumber();
        int lastVisibleBlockNumber = cursorForPosition(QPoint(viewport()->width(), viewport()->height())).blockNumber();

        for (int blockNumber = firstVisibleBlockNumber; blockNumber <= lastVisibleBlockNumber; ++blockNumber ) {
            HighlighterManager::instance().registerBlockToHighlight(findBlockByNumber(blockNumber), false);
        }
    }
}

void PlainTextEditor::applySettings()
{
    setFont(Preferences::instance().getEditorFont());
}

void PlainTextEditor::redo()
{
    m_undoStack.redo();
}

void PlainTextEditor::undo()
{
    m_undoStack.undo();
}

void PlainTextEditor::clearRedoUndoHistory()
{
    m_undoStack.clear();
}

void PlainTextEditor::contentsChangedSlot()
{
    Gui::statusBar()->setStatusBarDocLength(QString::number(m_textReadOnly.size()));
    emit contentsChanged();
    if (m_textReadOnly.isEmpty()) {
        HighlighterManager::instance().cancelHighlighting();
    }
}

void PlainTextEditor::contentsChangeSlot(int position, int charsRemoved, int charsAdded)
{
    m_textReadOnly = QPlainTextEdit::toPlainText();
    emit contentsChange(position, charsRemoved, charsAdded);

    //highlighting code
    const QTextBlock &first = findBlock(position);
    const QTextBlock &last = findBlock(position + charsAdded);
    int firstBlockNumber = first.blockNumber();
    int lastBlockNumber = last.blockNumber();
    if (firstBlockNumber == lastBlockNumber) {
        //Check is simple and fast. Make it this way.
        HighlighterManager::instance().registerBlockToHighlight(first, true);
    }
    else if (firstBlockNumber == lastBlockNumber + 1) {
        //Occurs a lot when pressing enter
        HighlighterManager::instance().registerBlockToHighlight(first, true);
        HighlighterManager::instance().registerBlockToHighlight(last, true);
    }
    else {
        int firstVisibleBlockNumber = QPlainTextEdit::firstVisibleBlock().blockNumber();
        int lastVisibleBlockNumber = cursorForPosition(QPoint(viewport()->width(), viewport()->height())).blockNumber();

        if (firstVisibleBlockNumber > firstBlockNumber) {
            firstBlockNumber = firstVisibleBlockNumber;
        }
        if (lastVisibleBlockNumber < lastBlockNumber) {
            lastBlockNumber = lastVisibleBlockNumber;
        }

        for (int blockNumber = firstBlockNumber; blockNumber <= lastBlockNumber; ++blockNumber ) {
            HighlighterManager::instance().registerBlockToHighlight(findBlockByNumber(blockNumber), true);
        }
    }
}

void PlainTextEditor::applyHintSlot(QAction *action)
{
    if (!action->data().isNull()) {
        QTextCursor cursor = textCursor();
        cursor.select(QTextCursor::WordUnderCursor);
        cursor.insertText(action->text());
    }
}

void PlainTextEditor::canUndoSlot(bool value)
{
    emit canUndo(value);
}

void PlainTextEditor::canRedoSlot(bool value)
{
    emit canRedo(value);
}

void PlainTextEditor::replace(int position, int length, const QString &after)
{
    Q_ASSERT(position >= 0);
    Q_ASSERT(position + length <= m_textReadOnly.length());

    QTextCursor cursor = textCursor();
    cursor.setPosition(position);
    cursor.setPosition(position + length, QTextCursor::KeepAnchor);
    cursor.insertText(after);
}

int PlainTextEditor::getCursorPosition() const
{
    return textCursor().position();
}

void PlainTextEditor::setCursorPosition(int position)
{
    if (getCursorPosition() != position) {
        QTextCursor cursor = textCursor();
        cursor.setPosition(position);
        setTextCursor(cursor);
    }
}

void PlainTextEditor::setCursorPositionToStart()
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::Start);
    setTextCursor(cursor);
}

void PlainTextEditor::setCursorPositionToEnd()
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
}

int PlainTextEditor::getSelectionStart() const
{
    Q_ASSERT(hasSelection());
    return textCursor().selectionStart();
}

int PlainTextEditor::getSelectionEnd() const
{
    Q_ASSERT(hasSelection());
    return textCursor().selectionEnd();
}

bool PlainTextEditor::hasSelection() const
{
    return textCursor().hasSelection();
}

void PlainTextEditor::setSelection(int selectionStart, int selectionEnd)
{
    QTextCursor cursor = textCursor();
    int oldSelectionStart = cursor.selectionStart();
    int oldSelectionEnd = cursor.selectionEnd();
    Q_ASSERT(selectionStart >= 0 && selectionEnd >= 0);
    if (oldSelectionStart != selectionStart || oldSelectionEnd != selectionEnd) {
        cursor.setPosition(selectionStart);
        cursor.setPosition(selectionEnd, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    }
}

void PlainTextEditor::clearSelection()
{
    textCursor().clearSelection();
}

const QString& PlainTextEditor::toPlainText() const
{
    return m_textReadOnly;
}

void PlainTextEditor::setPlainText(const QString &text)
{
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::Document);
    cursor.insertText(text);
}

QTextBlock PlainTextEditor::findBlockByNumber(int blockNumber) const
{
    return document()->findBlockByNumber(blockNumber);
}

QTextBlock PlainTextEditor::findBlock(int pos) const
{
    return document()->findBlock(pos);
}

void PlainTextEditor::setTextCursor(const QTextCursor& cursor)
{
    QPlainTextEdit::setTextCursor(cursor);
}

QTextCursor PlainTextEditor::textCursorForBlock(int blockNumber) const
{
    Q_ASSERT(blockNumber < blockCount());
    return QTextCursor(document()->findBlockByNumber(blockNumber));
}

QTextCursor PlainTextEditor::textCursor() const
{
    return QPlainTextEdit::textCursor();
}

int PlainTextEditor::blockCount() const
{
    return QPlainTextEdit::blockCount();
}

bool PlainTextEditor::blockSignals(bool block)
{
    document()->blockSignals(block);
    return QObject::blockSignals(block);
}

void PlainTextEditor::setFocus()
{
    QWidget::setFocus();
}

void PlainTextEditor::setEnabled(bool enabled)
{
    QWidget::setEnabled(enabled);
}

bool PlainTextEditor::isEnabled() const
{
    return QWidget::isEnabled();
}

void PlainTextEditor::setModified(bool modified)
{
    document()->setModified(modified);
}

bool PlainTextEditor::isModified() const
{
    return document()->isModified();
}

QWidget* PlainTextEditor::asWidget()
{
    return static_cast<QWidget*>(this);
}

QObject* PlainTextEditor::asObject()
{
    return static_cast<QObject*>(this);
}

