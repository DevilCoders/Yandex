#pragma once

#include <tools/idl/flex_bison/bison.h>

#include <contrib/tools/flex-old/FlexLexer.h>

class BisonedFlexLexer : public yyFlexLexer {
public:
    BisonedFlexLexer(std::istream* arg_yyin) : yyFlexLexer(arg_yyin) {}

    // generated
    int yylex(YYSTYPE* lval, YYLTYPE* llocp);

private:
    virtual int yylex() {
        // not implemented
        return 0;
    }
};
