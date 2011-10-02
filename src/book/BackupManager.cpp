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

#include "BackupManager.h"
#include <QDebug>
#include <QString>
#include <QFileInfo>
#include <QFile>
#include <QDir>

BackupManager::BackupManager()
{
}

BackupManager::~BackupManager()
{
}

bool BackupManager::create(const QString &fileName)
{
    QFileInfo info(fileName);
    QString copyFileName;
    quint32 index = 0;
    while (index < 0xFFFFFFFF) {
        copyFileName = info.path() +
                       QDir::separator() +
                       info.baseName() +
                       "_" +
                       QString::number(index++) +
                       "." +
                       info.completeSuffix();
        if (!QFile::exists(copyFileName)) {
            break;
        }
    }
    if (index == 0xFFFFFFFF) {
        return false;
    }
    qDebug() << "saving copy to" << copyFileName;
    if (!QFile::copy(fileName, copyFileName)) {
        return false;
    }
    return true;
}
