%pure-parser

%{
#include <kernel/remorph/tokenlogic/parser_defs.h>

#include <kernel/remorph/core/core.h>

#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/stream/output.h>
#include <util/string/vector.h>

#include <cstdlib>
#include <cstdio>

using namespace NTokenLogic;
using namespace NTokenLogic::NPrivate;

#define yyparse         ParseRulesImpl

void yyerror(TFileContext* ctx, const char* str) {
    ctx->Ctx.Error = ctx->Lexer.GetContext() + ": " + str;
}

#define PARSE_CTX       ctx->Ctx
#define PARSE_RES       ctx->Res
#define LEXER           ctx->Lexer

//#define TRACE_RULES

#undef TRACE
#ifdef TRACE_RULES
    #define TRACE(args) do { Cerr << args << Endl; } while(false)
#else
    #define TRACE(args) do {} while (false)
#endif

%}

%parse-param { NTokenLogic::NPrivate::TFileContext* ctx }
%lex-param { NTokenLogic::NPrivate::TFileContext* ctx }
%code requires {
#include <kernel/remorph/tokenlogic/parser_defs.h>
}

%union
{
    NTokenLogic::NPrivate::TLexerToken* Token;
    NTokenLogic::NPrivate::TExpPart*    Exp;
};

%{

inline int yylex(YYSTYPE* lval, TFileContext* ctx) {
    Y_ASSERT(ctx);
    lval->Token = ctx->Lexer.NextToken();
    TRACE("lexer: Id=" << lval->Token->Type << ", Text=" << lval->Token->Text);
    return lval->Token->Type;
}

%}


/* precedence */
%left TOK_OR
%left TOK_XOR
%left TOK_AND
%nonassoc '!'
%nonassoc TOK_EQ TOK_NEQ TOK_LTE TOK_GTE '<' '>' '?'
%left ':'

%token TOK_TRUE
%token TOK_FALSE

%token <Token> TOK_RULE
%token <Token> TOK_TOKEN
%token <Token> TOK_DEFAULT
%token <Token> TOK_USEGZT
%token <Token> TOK_INCLUDE
%token <Token> TOK_REF
%token <Token> TOK_LITERAL
%token <Token> TOK_ID
%token <Token> TOK_QSTR
%token <Token> TOK_WORD
%token <Token> TOK_INT
%token <Token> TOK_NUMBER
%token <Token> TOK_SUFFIX
%token <Token> '.'

%type <Token> literal agrliteral weight
%type <Exp> compare expression

%start rules

%%

rules :
    /* empty */
    | rules command
    ;

command :
    cmd_rule
    | cmd_token
    | cmd_default
    | cmd_use_gzt
    | cmd_include
    ;

cmd_rule :
    TOK_RULE TOK_ID weight '=' expression ';'
    {
        TRACE("rule <- name weight expression: " << $2->Text);
        double weight = 1.0;
        if ($3) {
            try {
                weight = FromString<double>($3->Text);
            } catch (const yexception& e) {
                PARSE_CTX.Error = $3->ToString() + ": " + e.what();
                YYABORT;
            }
        }

        PARSE_RES.Rules.push_back(TRule($2->Text, $5->Part, weight, PARSE_CTX.HeavyFlag));
        LogicalJoin(PARSE_RES.Rules.back().Expression, ctx->DefaultExp, TTokenExpLogical::And);
        PARSE_CTX.ClearExpPartCache();
        LEXER.ClearTokenCache();
        PARSE_CTX.HeavyFlag = false;
    }
    ;

weight :
    '(' TOK_INT ')'
    {
        TRACE("weight <- INT: " << $2->Text);
        $$ = $2;
    }
    ;

weight :
    '(' TOK_NUMBER ')'
    {
        TRACE("weight <- NUM: " << $2->Text);
        $$ = $2;
    }
    ;

weight :
    /* empty */
    {
        TRACE("weight default");
        $$ = 0;
    }
    ;

expression :
    compare
    {
        TRACE("expression <- compare: Lit=" << static_cast<const TTokenExpCmp*>($1->Part.front().Get())->Lit.GetId());
        $$ = $1;
    }
    ;

expression :
    expression TOK_OR expression
    {
        TRACE("expression <- expression OR expression");
        $$ = PARSE_CTX.AllocExpPart();
        $$->Part = $1->Part;
        LogicalJoin($$->Part, $3->Part, TTokenExpLogical::Or);
    }
    ;

expression :
    expression TOK_XOR expression
    {
        TRACE("expression <- expression XOR expression");
        $$ = PARSE_CTX.AllocExpPart();
        $$->Part = $1->Part;
        LogicalJoin($$->Part, $3->Part, TTokenExpLogical::Xor);
    }
    ;

expression :
    expression TOK_AND expression
    {
        TRACE("expression <- expression AND expression");
        $$ = PARSE_CTX.AllocExpPart();
        $$->Part = $1->Part;
        LogicalJoin($$->Part, $3->Part, TTokenExpLogical::And);
    }
    ;

expression :
    '!' expression
    {
        TRACE("expression <- !expression");
        $$ = PARSE_CTX.AllocExpPart();
        $$->Part = $2->Part;
        $$->Part.push_back(new TTokenExpLogical(TTokenExpLogical::Not));
    }
    ;

expression :
    '(' expression ')'
    {
        TRACE("expression <- (expression)");
        $$ = PARSE_CTX.AllocExpPart();
        $$->Part = $2->Part;
    }
    ;

compare :
    TOK_TRUE
    {
        TRACE("compare <- TRUE");
        $$ = PARSE_CTX.AllocExpPart();
        $$->Part.push_back(new TTokenExpConst(true));
    }
    ;

compare :
    TOK_FALSE
    {
        TRACE("compare <- FALSE");
        $$ = PARSE_CTX.AllocExpPart();
        $$->Part.push_back(new TTokenExpConst(false));
    }
    ;

compare :
    nmliteral
    {
        TRACE("compare <- nmliteral: Lit=" << PARSE_CTX.CurrentNamedLit.first.GetId() << ", Labels=" << JoinStrings(PARSE_CTX.CurrentNamedLit.second, ","));
        Y_ASSERT(!PARSE_CTX.CurrentLiteral.IsNone());
        $$ = PARSE_CTX.AllocExpPart();
        $$->Part.push_back(new TTokenExpCmp(
            PARSE_CTX.CurrentLiteral,
            PARSE_CTX.CurrentLabels,
            TTokenExpCmp::Gt,
            0
        ));
    }
    ;

compare :
    nmliteral TOK_EQ TOK_INT
    {
        TRACE("compare <- nmliteral == INT: Lit=" << PARSE_CTX.CurrentNamedLit.first.GetId() << ", Labels=" << JoinStrings(PARSE_CTX.CurrentNamedLit.second, ","));
        Y_ASSERT(!PARSE_CTX.CurrentLiteral.IsNone());
        $$ = PARSE_CTX.AllocExpPart();
        $$->Part.push_back(new TTokenExpCmp(
            PARSE_CTX.CurrentLiteral,
            PARSE_CTX.CurrentLabels,
            TTokenExpCmp::Eq,
            FromString<ui32>($3->Text)
        ));
    }
    ;

compare :
    nmliteral TOK_NEQ TOK_INT
    {
        TRACE("compare <- nmliteral != INT: Lit=" << PARSE_CTX.CurrentNamedLit.first.GetId() << ", Labels=" << JoinStrings(PARSE_CTX.CurrentNamedLit.second, ","));
        Y_ASSERT(!PARSE_CTX.CurrentLiteral.IsNone());
        $$ = PARSE_CTX.AllocExpPart();
        $$->Part.push_back(new TTokenExpCmp(
            PARSE_CTX.CurrentLiteral,
            PARSE_CTX.CurrentLabels,
            TTokenExpCmp::Neq,
            FromString<ui32>($3->Text)
        ));
    }
    ;

compare :
    nmliteral TOK_LTE TOK_INT
    {
        TRACE("compare <- nmliteral <= INT: Lit=" << PARSE_CTX.CurrentNamedLit.first.GetId() << ", Labels=" << JoinStrings(PARSE_CTX.CurrentNamedLit.second, ","));
        Y_ASSERT(!PARSE_CTX.CurrentLiteral.IsNone());
        $$ = PARSE_CTX.AllocExpPart();
        $$->Part.push_back(new TTokenExpCmp(
            PARSE_CTX.CurrentLiteral,
            PARSE_CTX.CurrentLabels,
            TTokenExpCmp::Lte,
            FromString<ui32>($3->Text)
        ));
    }
    ;

compare :
    nmliteral TOK_GTE TOK_INT
    {
        TRACE("compare <- nmliteral >= INT: Lit=" << PARSE_CTX.CurrentNamedLit.first.GetId() << ", Labels=" << JoinStrings(PARSE_CTX.CurrentNamedLit.second, ","));
        Y_ASSERT(!PARSE_CTX.CurrentLiteral.IsNone());
        $$ = PARSE_CTX.AllocExpPart();
        $$->Part.push_back(new TTokenExpCmp(
            PARSE_CTX.CurrentLiteral,
            PARSE_CTX.CurrentLabels,
            TTokenExpCmp::Gte,
            FromString<ui32>($3->Text)
        ));
    }
    ;

compare :
    nmliteral '<' TOK_INT
    {
        TRACE("compare <- nmliteral < INT: Lit=" << PARSE_CTX.CurrentNamedLit.first.GetId() << ", Labels=" << JoinStrings(PARSE_CTX.CurrentNamedLit.second, ","));
        Y_ASSERT(!PARSE_CTX.CurrentLiteral.IsNone());
        $$ = PARSE_CTX.AllocExpPart();
        $$->Part.push_back(new TTokenExpCmp(
            PARSE_CTX.CurrentLiteral,
            PARSE_CTX.CurrentLabels,
            TTokenExpCmp::Lt,
            FromString<ui32>($3->Text)
        ));
    }
    ;

compare :
    nmliteral '>' TOK_INT
    {
        TRACE("compare <- nmliteral > INT: Lit=" << PARSE_CTX.CurrentNamedLit.first.GetId() << ", Labels=" << JoinStrings(PARSE_CTX.CurrentNamedLit.second, ","));
        Y_ASSERT(!PARSE_CTX.CurrentLiteral.IsNone());
        $$ = PARSE_CTX.AllocExpPart();
        $$->Part.push_back(new TTokenExpCmp(
            PARSE_CTX.CurrentLiteral,
            PARSE_CTX.CurrentLabels,
            TTokenExpCmp::Gt,
            FromString<ui32>($3->Text)
        ));
    }
    ;

compare :
    nmliteral '?'
    {
        TRACE("compare <- nmliteral?: Lit=" << PARSE_CTX.CurrentNamedLit.first.GetId() << ", Labels=" << JoinStrings(PARSE_CTX.CurrentNamedLit.second, ","));
        Y_ASSERT(!PARSE_CTX.CurrentLiteral.IsNone());
        $$ = PARSE_CTX.AllocExpPart();
        $$->Part.push_back(new TTokenExpCmp(
            PARSE_CTX.CurrentLiteral,
            PARSE_CTX.CurrentLabels,
            TTokenExpCmp::Gte,
            0
        ));
    }
    ;

nmliteral :
    nmliteral ':' TOK_ID
    {
        TRACE("nmliteral <- nmliteral:label: Lit=" << PARSE_CTX.CurrentLiteral.GetId() << ", Label=" << $3->Text);
        PARSE_CTX.CurrentLabels.insert_unique($3->Text);
    }
    ;

nmliteral :
    agrliteral
    {
        TRACE("nmliteral <- agrliteral, Lit =" << PARSE_CTX.CurrentLiteral.GetType());
        if (!PARSE_CTX.CurrentAgreements.empty()) {
            if (PARSE_CTX.CurrentLiteral.IsNone()) {
                Y_ASSERT(PARSE_CTX.CurrentLTElement);
                PARSE_CTX.CurrentLTElement = PARSE_CTX.CurrentLTElement->Clone("#" + ::ToString(++PARSE_CTX.NextClassId));
                PARSE_CTX.CurrentLiteral = PARSE_RES.LiteralTable->Add(PARSE_CTX.CurrentLTElement.Get());
            } else {
                NRemorph::TLiteral oldLit = PARSE_CTX.CurrentLiteral;
                PARSE_CTX.CurrentLTElement = PARSE_RES.LiteralTable->Get(oldLit)->Clone("#" + ::ToString(++PARSE_CTX.NextClassId));
                PARSE_CTX.CurrentLiteral = PARSE_RES.LiteralTable->Add(PARSE_CTX.CurrentLTElement.Get());
                PARSE_RES.LiteralTable->CopyAgreements(oldLit, PARSE_CTX.CurrentLiteral);
            }
            PARSE_RES.LiteralTable->GetAgreementGroup(PARSE_CTX.CurrentLiteral).AddAgreements(PARSE_CTX.CurrentAgreements);
            PARSE_CTX.CurrentAgreements.clear();
        } else if (PARSE_CTX.CurrentLiteral.IsNone()) {
            Y_ASSERT(PARSE_CTX.CurrentLTElement);
            PARSE_CTX.CurrentLiteral = PARSE_RES.LiteralTable->Add(PARSE_CTX.CurrentLTElement.Get());
        }
    }
    ;

agree :
    TOK_SUFFIX '(' TOK_WORD ')'
    {
        TRACE("agree <- #agree(word_ctx): " << $1->Text << ", ctx= " << $3->Text);
        try {
            PARSE_CTX.CurrentAgreements.push_back(NLiteral::CreateAgreement($1->Text));
            PARSE_CTX.CurrentAgreements.back()->SetContext($1->Text + "#" + $3->Text);
        } catch (const yexception& e) {
            PARSE_CTX.Error = $1->ToString() + ": " + e.what();
            YYABORT;
        }
    }
    ;

agree :
    TOK_SUFFIX '(' TOK_ID ')'
    {
        TRACE("agree <- #agree(id_ctx): " << $1->Text << ", ctx= " << $3->Text);
        try {
            PARSE_CTX.CurrentAgreements.push_back(NLiteral::CreateAgreement($1->Text));
            PARSE_CTX.CurrentAgreements.back()->SetContext($1->Text + "#" + $3->Text);
        } catch (const yexception& e) {
            PARSE_CTX.Error = $1->ToString() + ": " + e.what();
            YYABORT;
        }
    }
    ;

agrliteral :
    agrliteral agree
    {
        TRACE("agrliteral <- agrliteral agree");
        if (!PARSE_CTX.CurrentLiteral.IsNone() && !PARSE_CTX.CurrentLiteral.IsOrdinal()) {
            PARSE_CTX.Error = $1->ToString() + ": agreement cannot be specified for 'any' token";
            YYABORT;
        }
        if (PARSE_CTX.CurrentAgreements.back()->GetType() == NLiteral::TAgreement::Distance) {
            PARSE_CTX.HeavyFlag = true;
        }

        $$ = $1;
    }
    ;

agrliteral :
    TOK_REF
    {
        TRACE("agrliteral <- REF: " << $1->Text);
        TTokenMap::const_iterator i = PARSE_CTX.TokenMap.find($1->Text);
        if (i == PARSE_CTX.TokenMap.end()) {
            PARSE_CTX.Error = $1->ToString() + ": token \"" + $1->Text + "\" is not defined";
            YYABORT;
        } else {
            PARSE_CTX.CurrentLTElement.Drop();
            PARSE_CTX.CurrentLiteral = i->second.first;
            PARSE_CTX.CurrentLabels = i->second.second;
            PARSE_CTX.CurrentAgreements.clear();
        }
        $$ = $1;
    }
    ;

agrliteral :
    literal
    {
        TRACE("agrliteral <- literal: Lit=" << PARSE_CTX.CurrentNamedLit.first.GetId());
        PARSE_CTX.CurrentLabels.clear();
        PARSE_CTX.CurrentAgreements.clear();
        $$ = $1;
    }
    ;

literal :
    TOK_LITERAL
    {
        TRACE("literal <- LITERAL: " << $1->Text);
        PARSE_CTX.CurrentLTElement = ctx->ParseLiteral($1->Text);
        PARSE_CTX.CurrentLiteral = NRemorph::TLiteral();
        $$ = $1;
    }
    ;

literal :
    TOK_QSTR
    {
        TRACE("literal <- QSTR: " << $1->Text);
        PARSE_CTX.CurrentLTElement = ctx->ParseSingleWord($1->Text);
        PARSE_CTX.CurrentLiteral = NRemorph::TLiteral();
        $$ = $1;
    }
    ;

literal :
    TOK_WORD
    {
        TRACE("literal <- WORD: " << $1->Text);
        PARSE_CTX.CurrentLTElement = ctx->ParseSingleWordUnquoted($1->Text);
        PARSE_CTX.CurrentLiteral = NRemorph::TLiteral();
        $$ = $1;
    }
    ;

literal :
    TOK_ID
    {
        TRACE("literal <- ID: " << $1->Text);
        PARSE_CTX.CurrentLTElement = ctx->ParseSingleWordUnquoted($1->Text);
        PARSE_CTX.CurrentLiteral = NRemorph::TLiteral();
        $$ = $1;
    }
    ;

literal :
    '.'
    {
        TRACE("literal <- ANY");
        PARSE_CTX.CurrentLTElement.Drop();
        PARSE_CTX.CurrentLiteral = NRemorph::TLiteral(0, NRemorph::TLiteral::Any);
        $$ = $1;
    }
    ;


cmd_token :
    TOK_TOKEN TOK_ID '=' nmliteral ';'
    {
        TTokenMap::const_iterator i = PARSE_CTX.TokenMap.find($2->Text);
        if (i != PARSE_CTX.TokenMap.end()) {
            PARSE_CTX.Error = $2->ToString() + ": token \"" + $2->Text + "\" already defined";
            YYABORT;
        } else {
            Y_ASSERT(!PARSE_CTX.CurrentLiteral.IsNone());
            PARSE_CTX.TokenMap[$2->Text] = std::make_pair(PARSE_CTX.CurrentLiteral, PARSE_CTX.CurrentLabels);
        }
        LEXER.ClearTokenCache();
    }
    ;

cmd_default :
    TOK_DEFAULT expression ';'
    {
        LogicalJoin(ctx->DefaultExp, $2->Part, TTokenExpLogical::And);
        PARSE_CTX.ClearExpPartCache();
        LEXER.ClearTokenCache();
    }
    ;

cmd_use_gzt :
    TOK_USEGZT gzt_list ';'
    {
        LEXER.ClearTokenCache();
    }
    ;

gzt_list : gzt_item;
gzt_list : gzt_list ',' gzt_item;

gzt_item :
    TOK_QSTR   { PARSE_RES.UseGzt.push_back($1->Text); }
    | TOK_ID   { PARSE_RES.UseGzt.push_back($1->Text); }
    | TOK_WORD { PARSE_RES.UseGzt.push_back($1->Text); }
    ;

cmd_include :
    TOK_INCLUDE TOK_QSTR ';'
    {
        if (!ctx->CheckAvailable($2->Text)) {
            PARSE_CTX.Error = $2->ToString() + ": cannot open file " + $2->Text;
            YYABORT;
        }
        if (!ctx->ParseFile($2->Text)) { // May be syntax error inside
            YYABORT;
        }
        LEXER.ClearTokenCache();
    }
    ;
