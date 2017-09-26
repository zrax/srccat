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

#include "esc_highlight.h"

#include <KSyntaxHighlighting/Format>
#include <KSyntaxHighlighting/Theme>
#include <KSyntaxHighlighting/State>

EscCodeHighlighter::EscCodeHighlighter()
    : m_output(stdout), m_palette()
{
}

static void add_code(QByteArray &fmtStart, const char *code)
{
    if (!fmtStart.endsWith('['))
        fmtStart.append(';');
    fmtStart.append(code);
}

void EscCodeHighlighter::applyFormat(int offset, int length,
                                     const KSyntaxHighlighting::Format &format)
{
    if (length == 0)
        return;

    const KSyntaxHighlighting::Theme currentTheme = theme();
    if (format.isDefaultTextStyle(currentTheme)) {
        m_output << m_line.mid(offset, length);
        return;
    }

    QByteArray fmtStart;
    fmtStart.reserve(32);
    fmtStart.append("\033[");

    if (format.isBold(currentTheme))
        add_code(fmtStart, "1");
    if (format.isItalic(currentTheme))
        add_code(fmtStart, "3");
    if (format.isUnderline(currentTheme))
        add_code(fmtStart, "4");
    if (format.isStrikeThrough(currentTheme))
        add_code(fmtStart, "9");

    if (format.hasBackgroundColor(currentTheme))
        add_code(fmtStart, m_palette->background(format.backgroundColor(currentTheme)));
    if (format.hasTextColor(currentTheme))
        add_code(fmtStart, m_palette->foreground(format.textColor(currentTheme)));

    fmtStart.append('m');

    m_output << fmtStart;
    m_output << m_line.mid(offset, length);
    m_output << "\033[0m";
}

void EscCodeHighlighter::highlightFile(QTextStream &in, bool numberLines)
{
    KSyntaxHighlighting::State state;
    int line = 0;

    while (!in.atEnd()) {
        m_line = in.readLine();
        if (numberLines) {
            QString nu = QString::number(++line);
            m_output << "\033[7;37m" << nu.rightJustified(7) << " \033[0m";
        }
        state = highlightLine(m_line, state);
        m_output << "\n";
    }
}
