#ifndef QSCILEXERSBY_H
#define QSCILEXERSBY_H

#include <QObject>

#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexer.h>
 

class QsciLexerSBY : public QsciLexer
{
    Q_OBJECT

public:
    enum {
        Default = 0,
        Comment = 1,
        Section = 2,
        Text = 3
    };

    QsciLexerSBY(QObject *parent = 0);

    //! Destroys the QsciLexerSBY instance.
    virtual ~QsciLexerSBY();

    //! Returns the name of the language.
    const char *language() const;

    //! Returns the name of the lexer.
    const char *lexer() const;

    QColor defaultColor(int style) const;

    QString description(int style) const;
                                               
private:
    QsciLexerSBY(const QsciLexerSBY &);
    QsciLexerSBY &operator=(const QsciLexerSBY &);
};

#endif
