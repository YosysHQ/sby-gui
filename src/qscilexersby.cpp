#include "qscilexersby.h"


// The ctor.
QsciLexerSBY::QsciLexerSBY(QObject *parent)
    : QsciLexer(parent)
{
}


// The dtor.
QsciLexerSBY::~QsciLexerSBY()
{
}


// Returns the language name.
const char *QsciLexerSBY::language() const
{
    return "SBY";
}


// Returns the lexer name.
const char *QsciLexerSBY::lexer() const
{
    return "sby";
}

QColor QsciLexerSBY::defaultColor(int style) const
{
    switch (style)
    {
    case Default:
        return QColor(0x00,0x00,0x00);

    case Comment:
        return QColor(0x80,0x80,0x80);

    case Section:
        return QColor(0xff,0x00,0x00);

    case Text:
        return QColor(0x00,0x00,0x00);
    }

    return QsciLexer::defaultColor(style);
}

QString QsciLexerSBY::description(int style) const
{
    switch (style)
    {
    case Default:
        return tr("Default");

    case Comment:
        return tr("Comment");

    case Section:
        return tr("Section");

    case Text:
        return tr("Text");
    }

    return QString();
}
