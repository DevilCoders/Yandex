%option batch
%option nodefault
%option noyywrap
%option noyymore
%option noreject
%option 8bit
%option c++
%option yylineno
%option stack
%option yyclass="TLiteralLexer"
%option prefix="lit"

%s SC_LOGIC
%x SC_STRING SC_LOGIC_VALUE SC_LOGIC_VALUE_STRING SC_LOGIC_VALUE_REGEXP

%{
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

#include <cstdio>
#include <util/generic/yexception.h>
#include <util/generic/string.h>
#include <kernel/remorph/literal/lexer.h>
#include <kernel/remorph/literal/parser.h>

#undef YY_DECL
#define YY_DECL TLiteralLexerToken TLiteralLexer::GetToken(TString& s)
#define yyterminate() return RLT_EOF
#define YY_USER_ACTION CharPos += YYLeng();
#define UPDATE_SOURCE_POS do { SourcePos.BegLine = SourcePos.EndLine = yylineno; SourcePos.BegCol = CharPosPrev; SourcePos.EndCol = CharPos; } while (false)
#define UNREQ_SEQ do { UPDATE_SOURCE_POS; throw TLiteralParseError(SourcePos) << "unrecognized character sequence"; } while (false)
#define UNMATCHED do { UPDATE_SOURCE_POS; throw TLiteralParseError(SourcePos) << "unmatched \" or / character"; } while (false)
#define RETURN_TOKEN(T) do { UPDATE_SOURCE_POS; CharPosPrev = CharPos; return T; } while (false)

using namespace NLiteral;
using namespace std;
%}

ID [[:alpha:]][[:alnum:]_]*
ID_WITH_DOT {ID}(\.{ID})*
BLANK [\t ]
BLANKS {BLANK}+
WORD [[:alnum:]\x80-\xFF_\-]+
WORD_COMMA [[:alnum:]\x80-\xFF_\-][[:alnum:]\x80-\xFF_\-,]*
PATH \"[^\n\t"]+\"
RE_MODS [[:alpha:]]*

%%

<INITIAL>{
{BLANKS}            CharPosPrev += YYLeng();
\n                  CharPos = 0; CharPosPrev = 0;
"?"                 RETURN_TOKEN(RLT_TYPE_LOGIC);
"*"                 RETURN_TOKEN(RLT_TYPE_LEMMA);
"%"{PATH}           s = TString(YYText() + 2, YYLeng() - 3); RETURN_TOKEN(RLT_TYPE_FILE);
"*%"{PATH}          s = TString(YYText() + 3, YYLeng() - 4); RETURN_TOKEN(RLT_TYPE_LEMMA_FILE);
{WORD}              s = TString(YYText(), YYLeng()); RETURN_TOKEN(RLT_WORD);
\"                  yy_push_state(SC_STRING); s.clear();
.                   UNREQ_SEQ;
}

<SC_STRING>{
\\\\                s.append('\\');
\\\"                s.append('"');
\"                  yy_pop_state(); RETURN_TOKEN(RLT_WORD);
.                   s.append(*YYText());
}

<SC_LOGIC>{
{BLANKS}            CharPosPrev += YYLeng();
\n                  CharPos = 0; CharPosPrev = 0;
=                   yy_push_state(SC_LOGIC_VALUE); RETURN_TOKEN(RLT_EQUAL);
!=                  yy_push_state(SC_LOGIC_VALUE); RETURN_TOKEN(RLT_NOT_EQUAL);
\<                  yy_push_state(SC_LOGIC_VALUE); RETURN_TOKEN(RLT_LESS);
\>                  yy_push_state(SC_LOGIC_VALUE); RETURN_TOKEN(RLT_GREATER);
&                   RETURN_TOKEN(RLT_AND);
\|                  RETURN_TOKEN(RLT_OR);
!                   RETURN_TOKEN(RLT_NOT);
\(                  RETURN_TOKEN(RLT_LPAREN);
\)                  RETURN_TOKEN(RLT_RPAREN);
{ID_WITH_DOT}       s = TString(YYText(), YYLeng()); RETURN_TOKEN(RLT_ID);
\${ID}              s = TString(YYText() + 1, YYLeng() - 1); RETURN_TOKEN(RLT_REFERENCE);
.                   UNREQ_SEQ;
}

<SC_LOGIC_VALUE>{
{BLANKS}            CharPosPrev += YYLeng();
\n                  CharPos = 0; CharPosPrev = 0;
\"                  yy_push_state(SC_LOGIC_VALUE_STRING); s.clear();
\/                  yy_push_state(SC_LOGIC_VALUE_REGEXP); s.clear();
{WORD_COMMA}        s = TString(YYText(), YYLeng()); yy_pop_state(); RETURN_TOKEN(RLT_WORD);
.                   UNREQ_SEQ;
}

<SC_LOGIC_VALUE_STRING>{
\\\\                s.append('\\');
\\\"                s.append('"');
\"                  yy_pop_state(); yy_pop_state(); RETURN_TOKEN(RLT_WORD);
.                   s.append(*YYText());
\n                  UNMATCHED;
<<EOF>>             UNMATCHED;
}

<SC_LOGIC_VALUE_REGEXP>{
\\\\                s.append('\\');
\\\/                s.append('/');
\/{RE_MODS}         Suffix = TString(YYText() + 1, YYLeng() - 1); yy_pop_state(); yy_pop_state(); RETURN_TOKEN(RLT_RE);
.                   s.append(*YYText());
\n                  UNMATCHED;
<<EOF>>             UNMATCHED;
}

.                   {
                    UNREQ_SEQ;
                    }

%%

TLiteralLexer::~TLiteralLexer() {
    yy_flex_free(yy_start_stack);
}

void TLiteralLexer::SetStartConditionINITIAL() {
    BEGIN(INITIAL);
}

void TLiteralLexer::SetStartConditionLOGIC() {
    BEGIN(SC_LOGIC);
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
