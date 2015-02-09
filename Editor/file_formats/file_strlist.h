/*
 * Platformer Game Engine by Wohlstand, a free platform for game making
 * Copyright (c) 2014-2015 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FILE_STRLIST_H
#define FILE_STRLIST_H

#include <QStringList>

class FileStringList:public QObject
{
    Q_OBJECT

public:
    FileStringList();

    FileStringList(QString fileData);
    ~FileStringList();

    void addData(QString fileData);
    QString readLine();
    bool isEOF();
    bool atEnd();
private:
    QStringList buffer;
    long lineID;
};

#endif // FILE_STRLIST_H

