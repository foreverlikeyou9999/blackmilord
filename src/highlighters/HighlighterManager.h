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

#ifndef BLACK_MILORD_HIGHLIGHTER_MANAGER_H
#define BLACK_MILORD_HIGHLIGHTER_MANAGER_H

#include <QObject>
#include <QSharedPointer>
#include <QList>
#include <QEvent>
#include "AbstractHighlighter.h"

class QTextBlock;
class PlainTextEditor;
class HighlighterThread;

class HighlighterManager :
    public QObject
{
    Q_OBJECT

    HighlighterManager();
    virtual ~HighlighterManager();
public:
    static HighlighterManager& instance();
    void registerBlockToHighlight(const QTextBlock &block, bool important);
    void cancelHighlighting();
    void rehighlight();
    QVector<AbstractHighlighter*> getHighlighters() const;

protected:
    void highlightBlock(int blockIndex, const AbstractHighlighter::MultiFormatList &formatting);
    void customEvent(QEvent *event);

private:
    int m_inProgress;
    QVector<AbstractHighlighter*> m_highlighters;
    HighlighterThread *m_highlighterThread;

private slots:
    void applySettings();
};

#endif /* BLACK_MILORD_HIGHLIGHTER_MANAGER_H */
