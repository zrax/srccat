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

#ifndef Q_OS_WIN
#include "pager.h"
#endif

#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Theme>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/SyntaxHighlighter>

#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QMimeDatabase>
#include <QTranslator>
#include <QLibraryInfo>

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

static KSyntaxHighlighting::Definition detect_highlighter_mime(const QString &filename)
{
    using KSyntaxHighlighting::Definition;

    QMimeDatabase mimeDb;
    const auto &mime = mimeDb.mimeTypeForFile(filename);
    if (mime.isDefault() || mime.name() == QStringLiteral("text/plain"))
        return Definition();

    Definition matchDef;
    int matchPriority = std::numeric_limits<int>::min();
    for (const auto &def : syntax_repo()->definitions()) {
        if (def.priority() < matchPriority)
            continue;

        for (const auto &matchType : def.mimeTypes()) {
            if (mime.name() == matchType || mime.aliases().indexOf(matchType) >= 0) {
                matchDef = def;
                matchPriority = def.priority();
                break;
            }
        }
    }

    return matchDef;
}

static KSyntaxHighlighting::Definition detect_highlighter(const QString &filename)
{
    auto definition = syntax_repo()->definitionForFileName(filename);
    if (definition.isValid())
        return definition;

    qDebug("Asking QMimeDatabase about %s", qPrintable(filename));
    return detect_highlighter_mime(filename);
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

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("srccat"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));

    QLocale defaultLocale;
    QTranslator qt_translator;
    qt_translator.load(defaultLocale, QStringLiteral("qt"), QStringLiteral("_"),
                       QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    QCoreApplication::installTranslator(&qt_translator);

    QTranslator translator;
    translator.load(defaultLocale, QStringLiteral(":/srccat"), QStringLiteral("_"));
    QCoreApplication::installTranslator(&translator);

    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("Syntax highlighting cat tool"));
    auto optHelp = parser.addHelpOption();
    auto optVersion = parser.addVersionOption();
    parser.addPositionalArgument(QObject::tr("filename [...]"),
            QObject::tr("File(s) to output, or '-' for stdin"));
#ifndef Q_OS_WIN
    QCommandLineOption optPager(QStringList{"p", "pager"},
            QObject::tr("Pipe output through $PAGER (or \"less\" if unset)"));
#endif
    QCommandLineOption optNumberLines(QStringList{"n", "number"},
            QObject::tr("Number source lines"));
    QCommandLineOption optDark(QStringList{"k", "dark"},
            QObject::tr("Use default dark theme"));
    QCommandLineOption optLight(QStringList{"L", "light"},
            QObject::tr("Use default light theme (default)"));
    QCommandLineOption optTheme(QStringList{"T", "theme"},
            QObject::tr("Set theme by name (overrides --dark and --light)"),
            QObject::tr("theme"));
    QCommandLineOption optSyntax(QStringList{"S", "syntax"},
            QObject::tr("Set syntax defition (default = auto detect)"),
            QObject::tr("lang"));
    QCommandLineOption optColors(QStringList{"C", "colors"},
            QObject::tr("Supported colors (8, 16, 88, 256, true, auto)"),
            QObject::tr("colors"));
    QCommandLineOption optListThemes("theme-list",
            QObject::tr("List all supported themes"));
    QCommandLineOption optListSyntax("syntax-list",
            QObject::tr("List all supported syntax definitions"));
#ifndef Q_OS_WIN
    parser.addOption(optPager);
#endif
    parser.addOption(optNumberLines);
    parser.addOption(optDark);
    parser.addOption(optLight);
    parser.addOption(optTheme);
    parser.addOption(optSyntax);
    parser.addOption(optColors);
    parser.addOption(optListThemes);
    parser.addOption(optListSyntax);

    if (!parser.parse(QCoreApplication::arguments())) {
        fprintf(stderr, "%s\n", qPrintable(parser.errorText()));
        ::exit(1);
    }
    if (parser.isSet(optVersion))
        parser.showVersion();
    if (parser.isSet(optHelp)) {
        printf("%s\n", qPrintable(parser.helpText()));
        puts(qPrintable(QObject::tr("Environment Variables:")));
        puts(qPrintable(QObject::tr("  SRCCAT_DARK            1 = Use the dark theme (-k) by default")));
        puts(qPrintable(QObject::tr("  SRCCAT_NUMBER          1 = Enable line numbering (-n) by default")));
        puts(qPrintable(QObject::tr("  SRCCAT_PAGER           <path> = Set a pager program (overriding $PAGER)\n"
                                    "                         and enable it (-p) by default")));
        puts(qPrintable(QObject::tr("  SRCCAT_THEME           <name> = Set a default theme (-T <name>)")));
        ::exit(0);
    }

    const QStringList files = parser.positionalArguments();

    if (parser.isSet(optListThemes)) {
        puts(qPrintable(QObject::tr("Supported themes:")));
        auto sortedThemes = syntax_repo()->themes();
        std::sort(sortedThemes.begin(), sortedThemes.end(),
                  [](const KSyntaxHighlighting::Theme &l,
                     const KSyntaxHighlighting::Theme &r) {
                      return QString::compare(l.name(), r.name(), Qt::CaseInsensitive) < 0;
                  });
        for (const auto &theme : sortedThemes)
            printf("  - %s\n", qPrintable(theme.name()));
        ::exit(0);
    }
    if (parser.isSet(optListSyntax)) {
        puts(qPrintable(QObject::tr("Supported Syntax Definitions:")));
        auto sortedDefs = syntax_repo()->definitions();
        std::sort(sortedDefs.begin(), sortedDefs.end(),
                  [](const KSyntaxHighlighting::Definition &l,
                     const KSyntaxHighlighting::Definition &r) {
                      return QString::compare(l.name(), r.name(), Qt::CaseInsensitive) < 0;
                  });
        for (const auto &def : sortedDefs)
            printf("  - %s\n", qPrintable(def.name()));
        ::exit(0);
    }

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
            fputs(qPrintable(QObject::tr("Invalid color option: %1\n").arg(colorType)),
                  stderr);
            fputs(qPrintable(QObject::tr("Supported values are: 8, 16, 88, 256, true, auto\n")),
                  stderr);
            return 1;
        }
    } else {
        palette = detect_palette();
    }

#ifndef Q_OS_WIN
    // Needs to be declared before outputStream, so that outputStream gets
    // deleted before pagerProcess in case there is any lingering output
    std::unique_ptr<PagerProcess> pagerProcess;
#endif
    std::unique_ptr<QTextStream> outputStream;

#ifndef Q_OS_WIN
    if (parser.isSet(optPager) || !qEnvironmentVariableIsEmpty("SRCCAT_PAGER")) {
        pagerProcess.reset(PagerProcess::create());
        if (pagerProcess)
            outputStream.reset(new QTextStream(pagerProcess.get()));
    }
#endif
    if (!outputStream)
        outputStream.reset(new QTextStream(stdout));

    EscCodeHighlighter highlighter(*outputStream);
    highlighter.setTheme(theme);
    highlighter.setPalette(palette);

    if (parser.isSet(optSyntax))
        highlighter.setDefinition(syntax_repo()->definitionForName(parser.value(optSyntax)));

    const bool numberLines = parser.isSet(optNumberLines) || environ_to_bool("SRCCAT_NUMBER");

    int exitStatus = 0;

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
                fputs(qPrintable(QObject::tr("Could not open %1 for reading\n").arg(file)),
                      stderr);
                exitStatus = 1;
            }
        }
    }

#ifndef Q_OS_WIN
    if (pagerProcess) {
        int pagerStatus = pagerProcess->exec();
        if (pagerStatus != 0)
            exitStatus = pagerStatus;
    }
#endif

    return exitStatus;
}
