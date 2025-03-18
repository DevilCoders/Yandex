%{

#include <assert.h>
#include <stdlib.h>

//#include <iostream>
#include <math.h>

#include <util/generic/string.h>

#include <library/cpp/html/sanitize/css/config-drv.h>
#include <library/cpp/html/sanitize/css/config-lexer.h>
#include <library/cpp/html/sanitize/css/config_y.h>

using std::cout;
using std::cerr;
using std::cin;
using std::endl;

#define yyterminate() return END

using namespace NCssConfig;

%}

%option case-insensitive
%option noyywrap
%option c++
%option prefix="config"
/*%option stack*/
%option yylineno
/*%option batch*/

/* The following paragraph suffices to track locations accurately. Each time
 * yylex is invoked, the begin position is moved onto the end position. */
%{
//#define YY_USER_ACTION  yylloc->columns(yyleng);
%}

h		[0-9a-f]
unicode		\\{h}{1,6}[ \t\r\n\f]?
escape		{unicode}|\\[ -~\200-\377]
nonascii	[\200-\377]
nmstart_spec    "-"|"_"
nmstart		[a-z]|{nonascii}|{escape}
nmchar		[a-z0-9-]|{nonascii}|{escape}|"_"
ident		{nmstart_spec}?{nmstart}{nmchar}*
num		[0-9]+|[0-9]*"."[0-9]+
s		[ \t\r\n\f]+
w		{s}?
nl		\n|\r\n|\r|\f
string1		\"([\t !#$%&(-~]|\\{nl}|\'|{nonascii}|{escape})*\"
string2		\'([\t !#$%&(-~]|\\{nl}|\"|{nonascii}|{escape})*\'
string		{string1}|{string2}
regexp          \/[^/]*\/[a-zA-Z]*

%%


\/\*[^*]*\*+([^/*][^*]*\*+)*\/{w}              /* ignore c-style comment */


#.*{nl}

{nl}|{s}


"default"   { return DEFAULT; }
"property"  { return PROPERTY; }
"selector"  { return SELECTOR; }
"append"    { return APPEND; }
"pass"      { return PASS; }
"deny"      { return DENY; }
"merge"     { return MERGE; }
"expression" { return EXPRESSION; }
"import"    { return IMPORT; }
"rewrite"   { return REWRITE; }
"and"       { return AND; }
"or"        { return OR; }
"not"       { return NOT; }
"scheme"    { return SCHEME; }
"class"     { return CLASS; }
"form"      { return FORM; }
"iframe"    { return IFRAME; }
">="        { return GREAT_EQUAL; }
"<="        { return LESS_EQUAL; }
"=~"        { return MATCH; }

{regexp}    {
                yylval->text = new TString(yytext);
                return REGEXP;
            }
{num}       {   yylval->fval = atof(yytext);
                return NUMBER;
            }

{ident}     {
                yylval->text = new TString(yytext);
                return IDENT;
            }

{string}    {
                yylval->text = new TString(yytext+1);
                yylval->text->erase(yylval->text->begin() + yylval->text->size()-1);
                return STRING;
            }

.           { return *yytext;}
%%

/*** Additional Code ***/

namespace NCssConfig {

TConfigLexer::TConfigLexer(IInputStream & inps)
    : yyFlexLexer(NULL)
    , Inps(inps)
{
}

TConfigLexer::~TConfigLexer()
{
}

int TConfigLexer::LexerInput(char* buf, int max_size)
{
#ifdef YY_INTERACTIVE
    max_size = Min(1, max_size);
#endif

    try {
        assert(max_size >= 0);

        return (int)Inps.Read(buf, (size_t)max_size);
    } catch (...) {
    }

        return -1;
}

void TConfigLexer::SetDebug(bool b)
{
    yy_flex_debug = b;
}


} //namespace NCssSanit

/* This implementation of ExampleFlexLexer::yylex() is required to fill the
 * vtable of the class ExampleFlexLexer. We define the scanner's main yylex
 * function via YY_DECL to reside in the Scanner class instead. */

#ifdef yylex
#undef yylex
#endif

#if 1
int configFlexLexer::yylex()
{
    Cerr << "in CSSFlexLexer::yylex() !" << Endl;
    return 0;
}

/* When the scanner receives an end-of-file indication from YY_INPUT, it then
 * checks the yywrap() function. If yywrap() returns false (zero), then it is
 * assumed that the function has gone ahead and set up `yyin' to point to
 * another input file, and scanning continues. If it returns true (non-zero),
 * then the scanner terminates, returning 0 to its caller. */

#endif
