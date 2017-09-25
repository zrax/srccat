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

#include <QByteArray>
#include <QColor>
#include <QVector>

class EscPalette
{
public:
    static const EscPalette *Palette8();
    static const EscPalette *Palette16();
    static const EscPalette *Palette88();
    static const EscPalette *Palette256();
    static const EscPalette *TrueColor();

    QByteArray foreground(const QColor &color) const;
    QByteArray background(const QColor &color) const;

private:
    EscPalette() : m_state(_Initializing) { }

    enum _PaletteState
    {
        _Initializing,
        _Compiled,
        _TrueColor,
    };
    _PaletteState m_state;

    struct ColorCode
    {
        QColor m_color;
        QByteArray m_foreFormat;
        QByteArray m_backFormat;
    };
    struct KDNode
    {
        ColorCode m_color;
        KDNode *m_left;
        KDNode *m_right;
    };
    KDNode *m_colorTree;

    static KDNode *build_kdtree(QVector<ColorCode> colors, int depth = 0);
    bool isCompiled() const { return m_state != _Initializing; }
    void compile(const QVector<ColorCode> &colors);

    ColorCode findClosest(const QColor &ref) const;
};
