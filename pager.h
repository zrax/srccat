/* This file is part of srccat.
 * Copyright (c) 2017 Michael Hansen
 *
 * srccat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * srccat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with srccat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PAGER_H
#define _PAGER_H

#include <QIODevice>
#include <QProcessEnvironment>

class PagerProcess : public QIODevice
{
public:
    PagerProcess() : m_pid(), m_stdin() { }

    bool start(const QStringList &command, const QProcessEnvironment &env);
    int exec();

    void close() Q_DECL_OVERRIDE;

    static PagerProcess *create();

protected:
    qint64 readData(char *data, qint64 maxSize) Q_DECL_OVERRIDE;
    qint64 writeData(const char *data, qint64 maxSize) Q_DECL_OVERRIDE;

private:
    pid_t m_pid;
    int m_stdin;
};

#endif // _PAGER_H
