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
#include <QDebug>
#include <QMenu>

#include <AspellWrapper.h>
#include <HighlighterManager.h>
#include <Book.h>

PlainTextEditor::PlainTextEditor(QWidget * parent) :
    QPlainTextEdit(parent),
    m_undoStack(this)
{
    HighlighterManagerFactory::createInstance(this);
    connect(document(), SIGNAL(contentsChange(int, int, int)),
        this, SLOT(contentsChange(int, int, int)));
    connect(document(), SIGNAL(contentsChanged()),
        this, SLOT(contentsChanged()));
    connect(&m_undoStack, SIGNAL(canUndo(bool)),
        this, SLOT(canUndoSlot(bool)));
    connect(&m_undoStack, SIGNAL(canRedo(bool)),
        this, SLOT(canRedoSlot(bool)));
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
    menu->addAction(tr("Copy"), this, SLOT(copy()), QKeySequence::Copy);
    menu->addAction(tr("Cut"), this, SLOT(cut()), QKeySequence::Cut);
    menu->addAction(tr("Paste"), this, SLOT(paste()), QKeySequence::Paste);

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
                    connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(applyHint(QAction *)));
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

void PlainTextEditor::contentsChanged()
{
    if (toPlainText().isEmpty()) {
        HighlighterManagerFactory::instance().cancelHighlighting();
    }
}

void PlainTextEditor::contentsChange(int position, int charsRemoved, int charsAdded)
{
    Q_UNUSED(charsRemoved);
    if (Book::instance().isFileOpened()) {
        HighlighterManagerFactory::instance().registerBlockToHighlight(position, position + charsAdded, true);
    }
    else {
        //Opening in progress, use rehighlight instead
        //rehighlight has set whole document to be rehighlighted with low priority
        HighlighterManagerFactory::instance().rehighlight();
    }
}

void PlainTextEditor::applyHint(QAction *action)
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
