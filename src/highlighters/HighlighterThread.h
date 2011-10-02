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

#ifndef OPEN_EBOOK_EDITOR_HIGHLIGHTER_THREAD_H
#define OPEN_EBOOK_EDITOR_HIGHLIGHTER_THREAD_H

#include <QThread>

class HighlighterThread :
    public QThread
{
    Q_OBJECT
public:
    explicit HighlighterThread(QObject *parent = 0);
    virtual ~HighlighterThread();
protected:
    void highlightBlock();
    void customEvent(QEvent *event);};

#endif /* OPEN_EBOOK_EDITOR_HIGHLIGHTER_THREAD_H */