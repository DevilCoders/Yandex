%option batch
%option nodefault
%option noyywrap
%option noyymore
%option noreject
%option 8bit
%option c++
%option yylineno
%option stack
%option yyclass="TLexer"
%option prefix="tl"

%x SC_STRING SC_LITERAL SC_LITERAL_QUOTED SC_LITERAL_RE

%{
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

#include <kernel/remorph/tokenlogic/lexer.h>
#include <kernel/remorph/tokenlogic/parser_defs.h>
#include <kernel/remorph/tokenlogic/parser.h>

#include <util/generic/yexception.h>
#include <util/generic/string.h>

#include <cstdio>

#undef YY_DECL
#define YY_DECL int TLexer::GetToken(TString& s)
#define yyterminate() return -1
#define YY_USER_ACTION CharPos += YYLeng();
#define UPDATE_SOURCE_POS do { SourcePos.BegLine = LinePrev; SourcePos.EndLine = yylineno; SourcePos.BegCol = CharPosPrev; SourcePos.EndCol = CharPos - 1; } while (false)
#define UNREQ_SEQ do { UPDATE_SOURCE_POS; throw TLexerError() << "unrecognized character sequence"; } while (false)
#define UNMATCHED do { UPDATE_SOURCE_POS; throw TLexerError() << "unmatched \" or / character"; } while (false)
#define RETURN_TOKEN(T) do { UPDATE_SOURCE_POS; LinePrev = yylineno; CharPosPrev = CharPos; return T; } while (false)

using namespace NTokenLogic::NPrivate;
using namespace std;
%}

BLANK [\t ]
BLANKS {BLANK}+
ID [[:alpha:]][[:alnum:]_]*
ID_LOOSE [[:alpha:]][[:alnum:]_\-]*
INT [[:digit:]]+
INT_SIGNED [-+][[:digit:]]+
FLOAT [-+]?[[:digit:]]+\.{INT}?
FLOAT_FRACTION [-+]?\.{INT}
WORD [[:alnum:]\x80-\xFF_\-]+
NON_ALPHA [^[:alpha:]]

%%

<INITIAL>{
{BLANKS}            CharPosPrev += YYLeng();
\n                  CharPos = 0; CharPosPrev = 0;
^#.*\n              CharPos = 0; CharPosPrev = 0;
^\n                 CharPos = 0; CharPosPrev = 0;
include/{BLANK}     s = "include"; RETURN_TOKEN(TOK_INCLUDE);
rule/{BLANK}        s = "rule"; RETURN_TOKEN(TOK_RULE);
use-gzt/{BLANK}     s = "use-gzt"; RETURN_TOKEN(TOK_USEGZT);
default/{BLANK}     s = "default"; RETURN_TOKEN(TOK_DEFAULT);
token/{BLANK}       s = "token"; RETURN_TOKEN(TOK_TOKEN);
true/{NON_ALPHA}    s = "true"; RETURN_TOKEN(TOK_TRUE);
false/{NON_ALPHA}   s = "false"; RETURN_TOKEN(TOK_FALSE);
"=="                s = "=="; RETURN_TOKEN(TOK_EQ);
"!="                s = "!="; RETURN_TOKEN(TOK_NEQ);
"<="                s = "<="; RETURN_TOKEN(TOK_LTE);
">="                s = ">="; RETURN_TOKEN(TOK_GTE);
"&&"                s = "&&"; RETURN_TOKEN(TOK_AND);
"^^"                s = "^^"; RETURN_TOKEN(TOK_XOR);
"||"                s = "||"; RETURN_TOKEN(TOK_OR);
\[                  yy_push_state(SC_LITERAL); s.clear();
\"                  yy_push_state(SC_STRING); s.clear();
{INT}               s = TString(YYText(), YYLeng()); RETURN_TOKEN(TOK_INT);
{INT_SIGNED}        s = TString(YYText(), YYLeng()); RETURN_TOKEN(TOK_NUMBER);
{FLOAT}             s = TString(YYText(), YYLeng()); RETURN_TOKEN(TOK_NUMBER);
{FLOAT_FRACTION}    s = TString(YYText(), YYLeng()); RETURN_TOKEN(TOK_NUMBER);
{ID}                s = TString(YYText(), YYLeng()); RETURN_TOKEN(TOK_ID);
\${ID}              s = TString(YYText() + 1, YYLeng() - 1); RETURN_TOKEN(TOK_REF);
{WORD}              s = TString(YYText(), YYLeng()); RETURN_TOKEN(TOK_WORD);
#{ID_LOOSE}         s = TString(YYText() + 1, YYLeng() - 1); RETURN_TOKEN(TOK_SUFFIX);
.                   s = TString(YYText(), YYLeng()); RETURN_TOKEN(*YYText());
}

<SC_STRING>{
\\\\                s.append('\\');
\\\"                s.append('"');
\"                  yy_pop_state(); RETURN_TOKEN(TOK_QSTR);
.                   s.append(*YYText());
\n                  UNMATCHED;
<<EOF>>             UNMATCHED;
}

<SC_LITERAL>{
\"                  s.append(*YYText()); yy_push_state(SC_LITERAL_QUOTED);
\/                  s.append(*YYText()); yy_push_state(SC_LITERAL_RE);
\]                  yy_pop_state(); RETURN_TOKEN(TOK_LITERAL);
\n                  s.append(*YYText()); CharPos = 0;
.                   s.append(*YYText());
}

<SC_LITERAL_QUOTED>{
\\\\                s.append("\\\\");
\\\"                s.append("\\\"");
\"                  s.append(*YYText()); yy_pop_state();
.                   s.append(*YYText());
\n                  UNMATCHED;
<<EOF>>             UNMATCHED;
}

<SC_LITERAL_RE>{
\\\\                s.append("\\\\");
\\\/                s.append("\\/");
\/                  s.append(*YYText()); yy_pop_state();
.                   s.append(*YYText());
\n                  UNMATCHED;
<<EOF>>             UNMATCHED;
}

.                   {
                    UNREQ_SEQ;
                    }

%%

TLexer::~TLexer() {
    yy_flex_free(yy_start_stack);
}

// stubs to make compiler happy, never used
static void yy_push_state(int) {
    Y_ASSERT(false);
}

static void yy_pop_state(void) {
    Y_ASSERT(false);
}

static int yy_top_state( void ) {
    Y_ASSERT(false);
    yy_start_stack_ptr = 0;
    yy_start_stack_depth = 0;
    yy_start_stack = 0;
    return 0;
}
