%{
#include <math.h>
#include <assert.h>

#include <iostream>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>

#include <library/cpp/html/sanitize/css/css2-drv.h>
#include <library/cpp/html/sanitize/css/css2-lexer.h>
#include <library/cpp/html/sanitize/css/css2_y.h>
#include <library/cpp/html/sanitize/css/str_tools.h>

using std::cout;
using std::cerr;
using std::cin;
using std::endl;

#define yyterminate() return END
using namespace NCssSanit;

class TLexerError : public yexception
{
};

#define YY_FATAL_ERROR(msg)\
    throw TLexerError() << msg;

/* cut out comments in JS code */
static TString StripJSComments(const TString & s);
%}

%option case-insensitive
%option noyywrap
%option c++
%option prefix="css2"
%option yylineno
/*%option batch*/


/* The following paragraph suffices to track locations accurately. Each time
 * yylex is invoked, the begin position is moved onto the end position. */
%{
//#define YY_USER_ACTION  yylloc->columns(yyleng);
%}



h		[0-9a-f]
nonascii	[\200-\377]
unicode		\\{h}{1,6}[ \t\r\n\f]?
escape		{unicode}|\\[ -~\200-\377]
nmstart_spec    "-"|"_"
nmstart		[a-z]|{nonascii}|{escape}
nmchar		[a-z0-9-]|{nonascii}|{escape}|"_"
string1		\"([\t !#$%&(-~]|\\{nl}|\'|{nonascii}|{escape})*\"
string2		\'([\t !#$%&(-~]|\\{nl}|\"|{nonascii}|{escape})*\'

ident		{nmstart_spec}?{nmstart}{nmchar}*
name		{nmchar}+
num		[0-9]+|[0-9]*"."[0-9]+
string		{string1}|{string2}
url		([!#$%&*-~]|{nonascii}|{escape})*
s		[ \t\r\n\f]+
w		{s}?
nl		\n|\r\n|\r|\f

range		\?{1,6}|{h}(\?{0,5}|{h}(\?{0,4}|{h}(\?{0,3}|{h}(\?{0,2}|{h}(\??|{h})))))


A		a|\\0{0,4}(41|61)(\r\n|[ \t\r\n\f])?
C		c|\\0{0,4}(43|63)(\r\n|[ \t\r\n\f])?
D		d|\\0{0,4}(44|64)(\r\n|[ \t\r\n\f])?
E		e|\\0{0,4}(45|65)(\r\n|[ \t\r\n\f])?|\\e
G		g|\\0{0,4}(47|67)(\r\n|[ \t\r\n\f])?|\\g
H		h|\\0{0,4}(48|68)(\r\n|[ \t\r\n\f])?|\\h
I		i|\\0{0,4}(49|69)(\r\n|[ \t\r\n\f])?|\\i
K		k|\\0{0,4}(4b|6b)(\r\n|[ \t\r\n\f])?|\\k
M		m|\\0{0,4}(4d|6d)(\r\n|[ \t\r\n\f])?|\\m
N		n|\\0{0,4}(4e|6e)(\r\n|[ \t\r\n\f])?|\\n
O		o|\\0{0,4}(51|71)(\r\n|[ \t\r\n\f])?|\\o
P		p|\\0{0,4}(50|70)(\r\n|[ \t\r\n\f])?|\\p
R		r|\\0{0,4}(52|72)(\r\n|[ \t\r\n\f])?|\\r
S		s|\\0{0,4}(53|73)(\r\n|[ \t\r\n\f])?|\\s
T		t|\\0{0,4}(54|74)(\r\n|[ \t\r\n\f])?|\\t
X		x|\\0{0,4}(58|78)(\r\n|[ \t\r\n\f])?|\\x
Z		z|\\0{0,4}(5a|7a)(\r\n|[ \t\r\n\f])?|\\z

expression      {E}{X}{P}{R}{E}{S}{S}{I}{O}{N}
%%

%{
    if (StartToken) {
        int t = StartToken;
        StartToken = 0;
        return t;
    }
%}

\/\*[^*]*\*+([^/*][^*]*\*+)*\/{w}              /* ignore c-style comment */
\/\/[^\n]*{w}
\<\!(--([^\-]|-[^\-])*--[\_\r\n\t]*)*>{w}      /* ignore html comments */


{expression}{w}"("{w} {
                        /* special case for IE's 'expression'.
                            Becouse there may JavaScript code between '(' and ')', we should skip all that code.
                            if JS code is syntax incorrect we will check for '}' symbol which is stop us.
                        */
                        yylval->text = Gc.New<TString>();
                        int c;
                        int level = 1;
                        int brace_level = 0;

                        for (;;) {
                            c = yyinput();
                            switch(c) {
                                case EOF:
                                    yylval->text->Value = ")";
                                    return (EOF);

                                case '(':
                                    level++;
                                    yylval->text->Value += '(';
                                    break;

                                case ')':
                                    yylval->text->Value += ')';
                                    if (0 == --level) {
                                        yylval->text->Value = StripJSComments(yylval->text->Value);
                                        return EXPRESSION;
                                    }
                                    break;

                                case '\"':
                                    yylval->text->Value += SkipQuotedString('\"');
                                    break;

                                case '\'':
                                    yylval->text->Value += SkipQuotedString('\'');
                                    break;

                                case '{':
                                    yylval->text->Value += '{';
                                    brace_level++;
                                    break;

                                case '}':
                                    if (--brace_level < 0) {
                                        unput('}');
                                        yylval->text->Value = ")"; // JS code is wrong. So simply discard it
                                        return EXPRESSION;
                                    } else {
                                        yylval->text->Value += '}';
                                        --brace_level;
                                    }
                                    break;

                                default:
                                    yylval->text->Value += char(c);
                                    break;

                            }
                        }
                }

{nl}|{s}    {
               return S;
            }


{w}"{"{w}      {
                return LBRACE;
            }


{w}"}"{w}      {
                return RBRACE;
            }

{w}":"{w}   { return ':'; }

{w}";"{w}   { return ';'; }

{w}"="{w}   { return '='; }

"~="        {return INCLUDES;}
"|="        {return DASHMATCH;}

{w}","{w}   {return COMMA_T;}
{w}">"{w}   {return GREATER;}
{w}"+"{w}   {return PLUS;}


{string}    	 {
                    yylval->text = Gc.New<TString>();
                    yylval->text->Value = yytext+1;
                    yylval->text->Value.erase(yylval->text->Value.begin() + yylval->text->Value.size()-1);

                    return STRING;
                 }

"#"{name}	{
		    yylval->text = Gc.New<TString>(); /*skip hash?*/
		    yylval->text->Value = yytext; /*skip hash?*/
                    return HASH;
                }

"@import"{w}		{return IMPORT_SYM;}
"@page"			{return PAGE_SYM;}
"@media"		{return MEDIA_SYM;}
"@font-face"	{return FONT_FACE_SYM;}
"@charset"		{return CHARSET_SYM;}
"@"{ident}		{return ATKEYWORD;}

"!"{w}"important"	{ return IMPORTANT_SYM; }
"!important"	{ return IMPORTANT_SYM; }


{num}em			{yylval->fval=atof(yytext);
                 return EMS;}

{num}ex			{yylval->fval=atof(yytext);
                 return EXS;}

{num}px			{yylval->fval=atof(yytext);
                 return LENGTH_PX;}

{num}cm			{yylval->fval=atof(yytext);
                 return LENGTH_CM;}

{num}mm			{yylval->fval=atof(yytext);
                 return LENGTH_MM;}

{num}in			{yylval->fval=atof(yytext);
                 return LENGTH_IN;}

{num}pt			{yylval->fval=atof(yytext);
                 return LENGTH_PT;}

{num}pc			{yylval->fval=atof(yytext);
                 return LENGTH_PC;}

{num}deg		{yylval->fval=atof(yytext);
                 return ANGLE_DEG;}

{num}rad		{yylval->fval=atof(yytext);
                 return ANGLE_RAD;}

{num}grad		{yylval->fval=atof(yytext);
                 return ANGLE_GRAD;}

{num}ms			{yylval->fval=atof(yytext);
                 return TIME_MS;}

{num}s			{yylval->fval=atof(yytext);
                 return TIME_S;}

{num}Hz			{yylval->fval=atof(yytext);
                 return FREQ_HZ;}

{num}kHz		{yylval->fval=atof(yytext);
                 return FREQ_KHZ;}

{num}{ident}	{
                        yylval->text = Gc.New<TString>();
                        yylval->text->Value = yytext;
                 return DIMEN;}

{num}%			{yylval->fval=atof(yytext);
                 return PERCENTAGE;}

{num}			{yylval->fval=atof(yytext);
                 return NUMBER;}


{ident}     {
                 yylval->text = Gc.New<TString>();
                 yylval->text->Value = yytext;
                 return IDENT;
            }


"url"{w}"("{w}          {
                            yylval->text = Gc.New<TString>();
                            int c;
                            int level = 1;
                            int brace_level = 0;

                            for (;;) {
                                c = yyinput();
                                switch(c) {
                                    case EOF:
                                        return (EOF);

                                    case '(':
                                        level++;
                                        yylval->text->Value += '(';
                                        break;

                                    case ')':
                                        if (0 == --level) {
                                            StripQuotes(yylval->text->Value);
                                            yylval->text->Value = EscapeForbiddenUrlSymbols(yylval->text->Value);
                                            return URI;
                                        }
                                        yylval->text->Value += ')';
                                        break;

                                    case '\"':
                                        yylval->text->Value += SkipQuotedString('\"');
                                        break;

                                    case '\'':
                                        yylval->text->Value += SkipQuotedString('\'');
                                        break;

                                    case '{':
                                        yylval->text->Value += '{';
                                        brace_level++;
                                        break;

                                    case '}':
                                        if (--brace_level < 0) {
                                            unput('}');
                                            StripQuotes(yylval->text->Value);
                                            yylval->text->Value = EscapeForbiddenUrlSymbols(yylval->text->Value);
                                            return URI;
                                        } else {
                                            yylval->text->Value += '}';
                                            --brace_level;
                                        }
                                        break;

                                    default:
                                        yylval->text->Value += char(c);
                                        break;

                                }
                            }
                        }

{ident}{w}"("{w}	{  /* this rule checks both for "function(" and "expression(" */

                          /*  if (strcasecmp(yytext, "expression( " ) == 0) // !!!
                            {
                                yylval->text = new TString;
                                return ReadExpression(*yylval->text);
                            }
                            else*/
                            {
                                yylval->text = Gc.New<TString>();
                                for (const char* p = yytext; *p != '(' && !isspace(*p); p++)
                                    yylval->text->Value += *p;

                                return FUNCTION;
                            }
                        }



U\+{range}		{return UNICODERANGE;}
U\+{h}{1,6}-{h}{1,6}	{return UNICODERANGE;}

.			{ return yytext[0];}

%%

/*** Additional Code ***/

namespace NCssSanit {

TCSS2Lexer::TCSS2Lexer(IInputStream& inps, bool inline_css, TGc& gc)
    : yyFlexLexer(NULL)
    , Inps(inps)
    , StartToken(START_FULL)
    , Gc(gc)
{
    if (inline_css)
        StartToken = START_INLINE;
}

TCSS2Lexer::~TCSS2Lexer() {
}

int TCSS2Lexer::LexerInput(char* buf, int max_size)
{
#ifdef YY_INTERACTIVE
    max_size = Min(1, max_size);
#endif

    try {
        assert(max_size >= 0);

        int res = (int)Inps.Read(buf, (size_t)max_size);
        //Cerr << buf << "\n";
        return res;
    } catch (...) {
    }

    return -1;
}

void TCSS2Lexer::SetDebug(bool b)
{
    yy_flex_debug = b;
}

TString TCSS2Lexer::SkipQuotedString(char quote)
{
    int c;
    TString res;
    res += quote;

    for (;;) {
        c = yyinput();

        switch (c) {
            case '\'':
            case '"':
                if (c == quote) {
                    res += quote;
                    goto END_LOOP;
                }
                break;

            case '\\':
                c = yyinput();
                if (c == EOF)
                    goto END_LOOP;

                res += '\\';
                break;

            case  EOF:
                goto END_LOOP;
        }
        res += c;
    }
END_LOOP:

    return res;

}

void TCSS2Lexer::StripQuotes(TString & str)
{
    if (str.empty())
        return;

    while (isspace(str.back()))
        str.pop_back();

    if (str[0] == '"' || str[0] == '\'')
        str.erase(0,1);

    if (str.back() == '"' || str.back() == '\'')
        str.erase(str.size()-1);

}

/* Special case for IE's 'expression'.
   Becouse there may JavaScript code between '(' and ')', we should skip all that code.
   if JavaScript code has errors, we will check for '}' symbol which is stop us from further reading.
*/
int TCSS2Lexer::ReadExpression (TString & result)
{
    int c;
    int level = 1;
    int brace_level = 0;

    for (;;) {
        c = yyinput();
        switch(c) {
            case EOF:
                return (EOF);

            case '(':
                level++;
                result += '(';
                break;

            case ')':
                result += ')';
                if (0 == --level)
                    return EXPRESSION;
                break;

            case '\"':
                result += SkipQuotedString('\"');
                break;

            case '\'':
                result += SkipQuotedString('\'');
                break;

            case '{':
                result += '{';
                brace_level++;
                break;

            case '}':
                if (--brace_level < 0) {
                    unput('}');
                    return EXPRESSION;
                } else {
                    result += '}';
                    --brace_level;
                }
                break;

            default:
                result += char(c);
                break;

        }
    }
    assert(0);
    return EOF;
}

} //namespace NCcssSanit

/* This implementation of ExampleFlexLexer::yylex() is required to fill the
 * vtable of the class ExampleFlexLexer. We define the scanner's main yylex
 * function via YY_DECL to reside in the Scanner class instead. */

#ifdef yylex
#undef yylex
#endif

#if 1
int css2FlexLexer::yylex()
{
    return 0;
}

/* When the scanner receives an end-of-file indication from YY_INPUT, it then
 * checks the yywrap() function. If yywrap() returns false (zero), then it is
 * assumed that the function has gone ahead and set up `yyin' to point to
 * another input file, and scanning continues. If it returns true (non-zero),
 * then the scanner terminates, returning 0 to its caller. */

//int css2FlexLexer::yywrap()
//{
//    return 1;
//}
#endif

TString::const_iterator EndOfMultiLineComment(TString::const_iterator it, TString::const_iterator it_end)
{
    while(it != it_end){
        switch(*it){
            case '*':
                it++;
                if (it == it_end)
                    return it;
                if (*it == '/') {
                    it++;
                    return it;
                }
                break;
            default:
                it++;
        }
    }
    return it;
}

TString::const_iterator EndOfSingleLineComment(TString::const_iterator it, TString::const_iterator it_end)
{
    while(it != it_end){
        switch(*it){
            case '\n':
                it++;
                return it;

            default:
                it++;
        }
    }
    return it;
}

TString StripJSComments(const TString & s)
{
    TString res;

    TString::const_iterator it  = s.begin();
    TString::const_iterator it_end = s.end();

    while(it != it_end){
        switch(*it) {
            case '/':
                it++;

                if (it == it_end) {
                    res += '/';
                    goto END_LOOP;
                }

                if (*it == '*')
                    it = EndOfMultiLineComment(it, it_end);
                else if (*it == '/')
                    it = EndOfSingleLineComment(it, it_end);
                else {
                    res += '/';
                    res += *it;
                }
                break;

            default:
                res += *it;
                it++;
        }
    }
END_LOOP:

    return res;
}
