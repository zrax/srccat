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

#include "pager.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>

/* QProcess has no way of spawning a child that we can write to but does not
 * capture the process's stdout (making the pager useless).  Even if it did,
 * it's a bit overkill since this is a generally synchronous command line
 * app.  The following implementation is Unix-specific, but that's where the
 * pager makes the most sense anyway. */

bool PagerProcess::start(const QStringList &command, const QProcessEnvironment &env)
{
    int stdin_pipe[2];
    if (pipe(stdin_pipe) < 0) {
        perror("Failed to open process pipe");
        return false;
    }

    m_pid = fork();
    if (m_pid < 0) {
        perror("fork");
        ::close(stdin_pipe[0]);
        ::close(stdin_pipe[1]);
        return false;
    } else if (m_pid == 0) {
        // The child process
        ::close(stdin_pipe[1]);
        while (dup2(stdin_pipe[0], STDIN_FILENO) < 0 && errno == EINTR) {
            /* try again */
        }
        ::close(stdin_pipe[0]);

        std::vector<char *> cargv;
        cargv.reserve(command.size() + 1);
        for (const auto &arg : command)
            cargv.push_back(strdup(arg.toLocal8Bit().constData()));
        cargv.push_back(Q_NULLPTR);

        for (const auto &var : env.toStringList())
            putenv(strdup(var.toLocal8Bit().constData()));

        execvp(cargv[0], cargv.data());
        perror("execl");
        exit(1);
    } else {
        // The parent process
        m_stdin = stdin_pipe[1];
        ::close(stdin_pipe[0]);
    }

    return QIODevice::open(QIODevice::WriteOnly);
}

int PagerProcess::exec()
{
    if (m_pid < 0)
        return -1;

    close();

    int result;
    pid_t child = waitpid(m_pid, &result, 0);
    if (child == m_pid && WIFEXITED(result))
        return WEXITSTATUS(result);
    if (child < 0)
        perror("waitpid");

    return -1;
}

void PagerProcess::close()
{
    if (m_pid >= 0 && m_stdin != 0) {
        fsync(m_stdin);
        ::close(m_stdin);
        m_stdin = 0;
    }
    QIODevice::close();
}

qint64 PagerProcess::readData(char *data, qint64 maxSize)
{
    (void)data;
    (void)maxSize;

    qFatal("PagerProcess does not support reading");
    return -1;
}

qint64 PagerProcess::writeData(const char *data, qint64 maxSize)
{
    if (m_pid < 0 || m_stdin == 0)
        return -1;

    qint64 bytes;
    while ((bytes = ::write(m_stdin, data, maxSize)) < 0 && errno == EINTR) {
        /* try again */
    }

    if (bytes < 0) {
        perror("PagerProcess::writeData");

        // Block future attempts to write to a failed stream
        ::close(m_stdin);
        m_stdin = 0;
    }
    return bytes;
}

PagerProcess *PagerProcess::create()
{
    QString pagerExe;
    if (!qEnvironmentVariableIsEmpty("SRCCAT_PAGER"))
        pagerExe = QString::fromLocal8Bit(qgetenv("SRCCAT_PAGER"));
    else if (!qEnvironmentVariableIsEmpty("PAGER"))
        pagerExe = QString::fromLocal8Bit(qgetenv("PAGER"));
    else
        pagerExe = QStringLiteral("less");

    PagerProcess *pager = new PagerProcess;

    // Defaults copied from git
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("LESS"), QStringLiteral("FRX"));
    env.insert(QStringLiteral("LV"), QStringLiteral("-c"));

    if (pager->start(QStringList{pagerExe}, env))
        return pager;

    // Pager process failed to start -- notify the caller to use stdout
    delete pager;
    return Q_NULLPTR;
}
