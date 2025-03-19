%option batch
%option nodefault
%option noyywrap
%option noyymore
%option noreject
%option 8bit
%option c++
%option yylineno
%option stack
%option yyclass="TRuleLexer"
%option prefix="rl"

%s SC_ID SC_RE SC_PATH
%x SC_STRING_BEGIN SC_STRING_REST SC_LIST_ITEM SC_LIST_ITEM_REST SC_LITERAL SC_LITERAL_QUOTED SC_STRING SC_EXPECT_AGREE SC_AGREE SC_LITERAL_RE

%{
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

#include <cstdio>
#include <util/generic/yexception.h>
#include <util/generic/string.h>
#include <kernel/remorph/matcher/rule_lexer.h>

#undef YY_DECL
#define YY_DECL TRuleLexerToken TRuleLexer::GetToken(TString& s)
#define yyterminate() return RLT_EOF
#define YY_USER_ACTION CharPos += YYLeng();
#define UPDATE_SOURCE_POS do { SourcePos.BegLine = LinePrev; SourcePos.EndLine = yylineno; SourcePos.BegCol = CharPosPrev; SourcePos.EndCol = CharPos - 1; } while (false)
#define UNREQ_SEQ do { UPDATE_SOURCE_POS; throw TLexerError() << "unrecognized character sequence"; } while (false)
#define UNMATCHED do { UPDATE_SOURCE_POS; throw TLexerError() << "unmatched \" or / character"; } while (false)
#define RETURN_TOKEN(T) do { UPDATE_SOURCE_POS; LinePrev = yylineno; CharPosPrev = CharPos; return T; } while (false)

using namespace NReMorph::NPrivate;
using namespace std;
%}

ID [[:alpha:]][[:alnum:]_]*
ID_LOOSE [[:alpha:]][[:alnum:]_\-]*
FLOAT_WITH_WHOLE [-+]?[[:digit:]]+(\.[[:digit:]]*)?
FLOAT_FRACTION [-+]?\.[[:digit:]]+
BLANK [\t ]
BLANKS {BLANK}+
WORD [[:alnum:]\x80-\xFF_\-]+
PATH \"[^\n\t"]+\"

%%

{BLANKS}            CharPosPrev += YYLeng();


<INITIAL>{
^#.*\n              CharPos = 0; CharPosPrev = 0;
^\n                 CharPos = 0; CharPosPrev = 0;
def/{BLANK}         RETURN_TOKEN(RLT_DEF);
logic/{BLANK}       RETURN_TOKEN(RLT_LOGIC);
rule/{BLANK}        RETURN_TOKEN(RLT_RULE);
include/{BLANK}     RETURN_TOKEN(RLT_INCLUDE);
string/{BLANK}      RETURN_TOKEN(RLT_STRING);
use-gzt/{BLANK}     RETURN_TOKEN(RLT_USE_GZT);
}

\n                  CharPos = 0; CharPosPrev = 0;
;                   RETURN_TOKEN(RLT_EOL);

<SC_ID>{
=                   RETURN_TOKEN(RLT_EOID);
{ID}                s = TString(YYText(), YYLeng()); RETURN_TOKEN(RLT_ID);
\${ID}              s = TString(YYText() + 1, YYLeng() - 1); RETURN_TOKEN(RLT_REFERENCE);
\(                  RETURN_TOKEN(RLT_LPAREN);
\)                  RETURN_TOKEN(RLT_RPAREN);
{FLOAT_WITH_WHOLE}  s = TString(YYText(), YYLeng()); RETURN_TOKEN(RLT_FLOAT);
{FLOAT_FRACTION}    s = TString(YYText(), YYLeng()); RETURN_TOKEN(RLT_FLOAT);
}

<SC_LIST_ITEM>{
,                   RETURN_TOKEN(RLT_COMMA);
;                   RETURN_TOKEN(RLT_EOL);
{BLANKS}            CharPosPrev += YYLeng();
\"                  yy_push_state(SC_LIST_ITEM_REST); StringValue.clear();
{WORD}              s = TString(YYText(), YYLeng()); RETURN_TOKEN(RLT_WORD);
.                   UNREQ_SEQ;
}

<SC_LIST_ITEM_REST>{
\\\"                StringValue.append('"');
\"                  s = StringValue; yy_pop_state(); RETURN_TOKEN(RLT_WORD);
.                   StringValue.append(*YYText());
\n                  UNMATCHED;
<<EOF>>             UNMATCHED;
}

<SC_RE>{
\|                  RETURN_TOKEN(RLT_PIPE);
\*                  RETURN_TOKEN(RLT_ASTERISK);
\?                  RETURN_TOKEN(RLT_QUESTION);
\.                  RETURN_TOKEN(RLT_DOT);
\+                  RETURN_TOKEN(RLT_PLUS);
\(                  RETURN_TOKEN(RLT_LPAREN_NC);
\(<>                RETURN_TOKEN(RLT_LPAREN);
"(<"{ID}>           s = TString(YYText() + 2, YYLeng() - 3); RETURN_TOKEN(RLT_LPAREN_NAMED);
\)                  RETURN_TOKEN(RLT_RPAREN);
\^                  RETURN_TOKEN(RLT_CARET);
\$                  RETURN_TOKEN(RLT_DOLLAR);
\/                  RETURN_TOKEN(RLT_MARK);
\${ID}              s = TString(YYText() + 1, YYLeng() - 1); RETURN_TOKEN(RLT_REFERENCE);
\[                  RETURN_TOKEN(RLT_LBRACKET);
"#"{ID_LOOSE}       s = TString(YYText() + 1, YYLeng() - 1); yy_push_state(SC_EXPECT_AGREE); RETURN_TOKEN(RLT_SHARP);
\"                  yy_push_state(SC_STRING); s.clear();
{WORD}              s = TString(YYText(), YYLeng()); RETURN_TOKEN(RLT_WORD);
}

<SC_EXPECT_AGREE>{
\(                  yy_push_state(SC_AGREE); RETURN_TOKEN(RLT_LPAREN_NC);
.                   yyless(0); yy_pop_state();
}

<SC_AGREE>{
{WORD}              s = TString(YYText(), YYLeng()); RETURN_TOKEN(RLT_WORD);
\)                  yy_pop_state(); yy_pop_state(); RETURN_TOKEN(RLT_RPAREN);
.                   UNREQ_SEQ;
}

<SC_PATH>{
{PATH}              s = TString(YYText() + 1, YYLeng() - 2); RETURN_TOKEN(RLT_PATH);
}

<SC_STRING_BEGIN>{
{BLANKS}            CharPosPrev += YYLeng();
\"                  yy_push_state(SC_STRING_REST); StringValue.clear();
.                   UNREQ_SEQ;
}

<SC_STRING_REST>{
\\\\                StringValue.append('\\');
\\\"                StringValue.append('"');
\"                  s = StringValue; yy_pop_state(); yy_pop_state(); RETURN_TOKEN(RLT_STRING_VALUE);
.                   StringValue.append(*YYText());
\n                  UNMATCHED;
<<EOF>>             UNMATCHED;
}

<SC_LITERAL>{
\"                  StringValue.append(*YYText()); yy_push_state(SC_LITERAL_QUOTED);
\/                  StringValue.append(*YYText()); yy_push_state(SC_LITERAL_RE);
\]                  s = StringValue; RETURN_TOKEN(RLT_RBRACKET);
;                   s = StringValue; RETURN_TOKEN(RLT_EOL);
\n                  StringValue.append(*YYText()); CharPos = 0;
.                   StringValue.append(*YYText());
}

<SC_LITERAL_QUOTED>{
\\\\                StringValue.append("\\\\");
\\\"                StringValue.append("\\\"");
\"                  StringValue.append(*YYText()); yy_pop_state();
.                   StringValue.append(*YYText());
\n                  UNMATCHED;
<<EOF>>             UNMATCHED;
}

<SC_LITERAL_RE>{
\\\\                StringValue.append("\\\\");
\\\/                StringValue.append("\\/");
\/                  StringValue.append(*YYText()); yy_pop_state();
.                   StringValue.append(*YYText());
\n                  UNMATCHED;
<<EOF>>             UNMATCHED;
}

<SC_STRING>{
\\\\                s.append('\\');
\\\"                s.append('"');
\"                  yy_pop_state(); RETURN_TOKEN(RLT_STRING_VALUE);
.                   s.append(*YYText());
\n                  UNMATCHED;
<<EOF>>             UNMATCHED;
}

.                   {
                    UNREQ_SEQ;
                    }

%%

TRuleLexer::~TRuleLexer() {
    yy_flex_free(yy_start_stack);
}

void TRuleLexer::SetStartConditionRE() {
    BEGIN(SC_RE);
}

void TRuleLexer::SetStartConditionINITIAL() {
    BEGIN(INITIAL);
}

void TRuleLexer::SetStartConditionID() {
    BEGIN(SC_ID);
}

void TRuleLexer::SetStartConditionPATH() {
    BEGIN(SC_PATH);
}

void TRuleLexer::SetStartConditionSTRING() {
    yy_push_state(SC_STRING_BEGIN);
}

void TRuleLexer::SetStartConditionLITERAL() {
    StringValue.clear();
    BEGIN(SC_LITERAL);
}

void TRuleLexer::SetStartConditionLIST() {
    BEGIN(SC_LIST_ITEM);
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
