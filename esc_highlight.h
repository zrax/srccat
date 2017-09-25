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

#ifndef _ESC_HIGHLIGHT_H
#define _ESC_HIGHLIGHT_H

#include "esc_color.h"

#include <KSyntaxHighlighting/AbstractHighlighter>
#include <QTextStream>

class EscCodeHighlighter : public KSyntaxHighlighting::AbstractHighlighter
{
public:
    EscCodeHighlighter();

    void setPalette(const EscPalette *pal) { m_palette = pal; }

    void applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format) Q_DECL_OVERRIDE;

    void highlightFile(QTextStream &in, bool numberLines);

private:
    const EscPalette *m_palette;
    QTextStream m_output;
    QString m_line;
};

#endif
