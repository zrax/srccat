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
#include <KSyntaxHighlighting/SyntaxHighlighter>

#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

#include <magic.h>

struct magic_t_RAII
{
    magic_t m_magic;

    magic_t_RAII(magic_t magic) : m_magic(magic) { }
    ~magic_t_RAII() { magic_close(m_magic); }

    operator magic_t() { return m_magic; }
    bool operator!() const { return !m_magic; }
};


static KSyntaxHighlighting::Repository *syntax_repo()
{
    static KSyntaxHighlighting::Repository s_repo;
    return &s_repo;
}

static const EscPalette *detect_palette()
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

static QString detect_mime_type(const QString &filename)
{
    magic_t_RAII magic = magic_open(MAGIC_MIME_TYPE | MAGIC_SYMLINK);
    if (!magic) {
        qDebug("Could not initialize libmagic");
        return QString::null;
    }

    if (magic_load(magic, Q_NULLPTR) < 0) {
        qDebug("Could not load magic database: %s", magic_error(magic));
        return QString::null;
    }

    const QByteArray filenameEncoded = QFile::encodeName(filename);
    const char *mime = magic_file(magic, filenameEncoded.constData());
    if (!mime) {
        qDebug("Could not get MIME type from libmagic: %s", magic_error(magic));
        return QString::null;
    }
    return QString::fromLatin1(mime);
}

static KSyntaxHighlighting::Definition detect_highlighter_mime(const QString &filename)
{
    using KSyntaxHighlighting::Definition;

    const QString mime = detect_mime_type(filename);
    if (mime == QStringLiteral("text/plain"))
        return Definition();

    const QStringList mimeParts = mime.split(QLatin1Char('/'));
    if (mimeParts.size() == 0 || mimeParts.last().isEmpty())
        return Definition();

    QVector<Definition> candidates;
    for (const auto &def : syntax_repo()->definitions()) {
        for (const auto &matchType : def.mimeTypes()) {
            // Only compare the second part, to avoid missing matches due to
            // disagreement about whether XML markup should be text/xml or
            // application/xml (for example)
            const QStringList matchParts = matchType.split(QLatin1Char('/'));
            if (matchParts.last().compare(mimeParts.last(), Qt::CaseInsensitive) == 0) {
                candidates.append(def);
                break;
            }
        }
    }

    if (candidates.isEmpty())
        return Definition();

    std::partial_sort(candidates.begin(), candidates.begin() + 1, candidates.end(),
                      [](const Definition &left, const Definition &right) {
        return left.priority() > right.priority();
    });

    return candidates.first();
}

static KSyntaxHighlighting::Definition detect_highlighter(const QString &filename)
{
    // If libmagic finds a match, it's probably more likely to be correct
    // than the filename match, which is easily fooled
    // (e.g. idle3.6 is not a Troff Mandoc file)
    auto definition = detect_highlighter_mime(filename);
    if (definition.isValid())
        return definition;
    return syntax_repo()->definitionForFileName(filename);
}

static bool environ_to_bool(const char *varName)
{
    if (qEnvironmentVariableIsEmpty(varName))
        return false;

    const auto value = qgetenv(varName).toUpper();
    if (value == "0" || value == "F" || value == "FALSE")
        return false;
    return true;
}

#define trMain(text) QCoreApplication::translate("main", text)

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("srccat"));
    app.setApplicationVersion(QStringLiteral("1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(trMain("Syntax highlighting cat tool"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(trMain("filename [...]"),
            trMain("File(s) to output, or '-' for stdin"));
    QCommandLineOption optNumberLines(QStringList{"n", "number"},
            trMain("Number source lines"));
    QCommandLineOption optDark(QStringList{"k", "dark"},
            trMain("Use default dark theme"));
    QCommandLineOption optLight(QStringList{"L", "light"},
            trMain("Use default light theme (default)"));
    QCommandLineOption optTheme(QStringList{"T", "theme"},
            trMain("Set theme by name (overrides --dark and --light)"),
            trMain("theme"));
    QCommandLineOption optSyntax(QStringList{"S", "syntax"},
            trMain("Set syntax defition (default = auto detect)"),
            trMain("lang"));
    QCommandLineOption optColors(QStringList{"C", "colors"},
            trMain("Supported colors (8, 16, 88, 256, true, auto)"),
            trMain("colors"));
    QCommandLineOption optListThemes("theme-list",
            trMain("List all supported themes"));
    QCommandLineOption optListSyntax("syntax-list",
            trMain("List all supported syntax definitions"));
    parser.addOption(optNumberLines);
    parser.addOption(optDark);
    parser.addOption(optLight);
    parser.addOption(optTheme);
    parser.addOption(optSyntax);
    parser.addOption(optColors);
    parser.addOption(optListThemes);
    parser.addOption(optListSyntax);

    parser.process(app);
    const QStringList files = parser.positionalArguments();

    if (parser.isSet(optListThemes)) {
        puts(trMain("Supported themes:").toLocal8Bit().constData());
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
        puts(trMain("Supported Syntax Definitions:").toLocal8Bit().constData());
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
    if (!theme.isValid() && qEnvironmentVariableIsSet("SRCCAT_THEME")) {
        const auto themeName = qgetenv("SRCCAT_THEME");
        theme = syntax_repo()->theme(QString::fromUtf8(themeName));
    }
    if (!theme.isValid()) {
        auto defaultTheme = KSyntaxHighlighting::Repository::LightTheme;
        if (parser.isSet(optDark) || (environ_to_bool("SRCCAT_DARK") && !parser.isSet(optLight)))
            defaultTheme = KSyntaxHighlighting::Repository::DarkTheme;
        theme = syntax_repo()->defaultTheme(defaultTheme);
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
            fprintf(stderr,
                    trMain("Invalid color option: %s\n").toLocal8Bit().constData(),
                    colorType.toLocal8Bit().constData());
            fputs(trMain("Supported values are: 8, 16, 88, 256, true, auto\n").toLocal8Bit().constData(),
                  stderr);
            return 1;
        }
    } else {
        palette = detect_palette();
    }
    highlighter.setPalette(palette);

    if (parser.isSet(optSyntax))
        highlighter.setDefinition(syntax_repo()->definitionForName(parser.value(optSyntax)));

    const bool numberLines = parser.isSet(optNumberLines) || environ_to_bool("SRCCAT_NUMBER");

    for (const QString &file : files) {
        if (!parser.isSet(optSyntax))
            highlighter.setDefinition(detect_highlighter(file));

        if (file == "-") {
            QTextStream stream(stdin);
            highlighter.highlightFile(stream, numberLines);
        } else {
            QFile in(file);
            if (in.open(QIODevice::ReadOnly)) {
                QTextStream stream(&in);
                highlighter.highlightFile(stream, numberLines);
            } else {
                fprintf(stderr, trMain("Could not open %s for reading\n").toLocal8Bit().constData(),
                        file.toLocal8Bit().constData());
            }
        }
    }

    return 0;
}
