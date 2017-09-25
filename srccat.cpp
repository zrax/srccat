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

#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Theme>
#include <KSyntaxHighlighting/Definition>

#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <KF5/KSyntaxHighlighting/SyntaxHighlighter>

KSyntaxHighlighting::Repository *syntax_repo()
{
    static KSyntaxHighlighting::Repository s_repo;
    return &s_repo;
}

const EscPalette *detect_palette()
{
    // This is far from perfect, especially since so many terminals
    // (intentionally) lie about what they are

    QByteArray reportedTerm = qgetenv("TERM");
    QByteArray colorTerm = qgetenv("COLORTERM");
    if (colorTerm == "truecolor") {
        return EscPalette::TrueColor();
    } else if (reportedTerm.isEmpty()) {
        // Can't tell what we're running; could be Windows cmd.exe or some other
        // app with minimal color support.
        return EscPalette::Palette8();
    } else if (reportedTerm == "linux" || reportedTerm == "aterm") {
        return EscPalette::Palette8();
    } else if (reportedTerm.startsWith("xterm") && !reportedTerm.endsWith("256color")) {
        return EscPalette::Palette16();
    } else if (reportedTerm.startsWith("rxvt") && !reportedTerm.endsWith("256color")) {
        return EscPalette::Palette88();
    } else {
        // Could still be a true color terminal, but this should work
        // well enough for most (known) terminals not handled above
        return EscPalette::Palette256();
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("srccat"));
    app.setApplicationVersion(QStringLiteral("1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Syntax highlighting cat tool"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("filename [...]"),
            QCoreApplication::translate("main", "File(s) to output, or '-' for stdin"));
    QCommandLineOption optNumberLines(QStringList{"n", "number"},
            QCoreApplication::translate("main", "Number source lines"));
    QCommandLineOption optDark(QStringList{"k", "dark"},
            QCoreApplication::translate("main", "Use default dark theme"));
    QCommandLineOption optTheme(QStringList{"T", "theme"},
            QCoreApplication::translate("main", "Set theme by name (overrides --dark)"),
            QCoreApplication::translate("main", "theme"));
    QCommandLineOption optSyntax(QStringList{"S", "syntax"},
            QCoreApplication::translate("main", "Set syntax defition (default = auto detect)"),
            QCoreApplication::translate("main", "lang"));
    QCommandLineOption optColors(QStringList{"C", "colors"},
            QCoreApplication::translate("main", "Supported colors (8, 16, 88, 256, true, auto)"),
            QCoreApplication::translate("main", "colors"));
    QCommandLineOption optListThemes("theme-list",
            QCoreApplication::translate("main", "List all supported themes"));
    QCommandLineOption optListSyntax("syntax-list",
            QCoreApplication::translate("main", "List all supported syntax definitions"));
    parser.addOption(optNumberLines);
    parser.addOption(optDark);
    parser.addOption(optTheme);
    parser.addOption(optSyntax);
    parser.addOption(optColors);
    parser.addOption(optListThemes);
    parser.addOption(optListSyntax);

    parser.process(app);
    const QStringList files = parser.positionalArguments();

    if (parser.isSet(optListThemes)) {
        puts(QCoreApplication::translate("main", "Supported themes:").toLocal8Bit().constData());
        auto sortedThemes = syntax_repo()->themes();
        std::sort(sortedThemes.begin(), sortedThemes.end(),
                  [](const KSyntaxHighlighting::Theme &l,
                     const KSyntaxHighlighting::Theme &r) {
                      return QString::compare(l.name(), r.name(), Qt::CaseInsensitive) < 0;
                  });
        for (const auto &theme : sortedThemes)
            printf("  - %s\n", theme.name().toLocal8Bit().constData());
        ::exit(0);
    }
    if (parser.isSet(optListSyntax)) {
        puts(QCoreApplication::translate("main", "Supported Syntax Definitions:").toLocal8Bit().constData());
        auto sortedDefs = syntax_repo()->definitions();
        std::sort(sortedDefs.begin(), sortedDefs.end(),
                  [](const KSyntaxHighlighting::Definition &l,
                     const KSyntaxHighlighting::Definition &r) {
                      return QString::compare(l.name(), r.name(), Qt::CaseInsensitive) < 0;
                  });
        for (const auto &def : sortedDefs)
            printf("  - %s\n", def.name().toLocal8Bit().constData());
        ::exit(0);
    }

    EscCodeHighlighter highlighter;

    KSyntaxHighlighting::Theme theme;
    if (parser.isSet(optTheme))
        theme = syntax_repo()->theme(parser.value(optTheme));
    if (!theme.isValid()) {
        theme = syntax_repo()->defaultTheme(
                parser.isSet(optDark) ? KSyntaxHighlighting::Repository::DarkTheme
                                      : KSyntaxHighlighting::Repository::LightTheme);
    }
    highlighter.setTheme(theme);

    const EscPalette *palette;
    if (parser.isSet(optColors)) {
        QString colorType = parser.value(optColors);
        if (colorType == "auto")
            palette = detect_palette();
        else if (colorType == "true")
            palette = EscPalette::TrueColor();
        else if (colorType == "8")
            palette = EscPalette::Palette8();
        else if (colorType == "16")
            palette = EscPalette::Palette16();
        else if (colorType == "88")
            palette = EscPalette::Palette88();
        else if (colorType == "256")
            palette = EscPalette::Palette256();
        else {
            fprintf(stderr, "Invalid color option: %s\n", colorType.toLocal8Bit().constData());
            fputs("Supported values are: 8, 16, 88, 256, true, auto\n", stderr);
            return 1;
        }
    } else {
        palette = detect_palette();
    }
    highlighter.setPalette(palette);

    if (parser.isSet(optSyntax))
        highlighter.setDefinition(syntax_repo()->definitionForName(parser.value(optSyntax)));

    for (const QString &file : files) {
        if (!parser.isSet(optSyntax))
            highlighter.setDefinition(syntax_repo()->definitionForFileName(file));

        highlighter.highlightFile(file);
    }

    return 0;
}
