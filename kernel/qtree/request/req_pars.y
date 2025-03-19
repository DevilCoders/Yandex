%expect 24         /* expect several shift/reduce conflicts, suppresses yacc's warning message */
%pure-parser
%{
#include <cstdlib>
#include <cstdio>
#include <util/generic/string.h>
#include <util/stream/output.h>
#include <kernel/qtree/request/request.h>
#include <kernel/qtree/request/reqscan.h>

#ifdef _MSC_VER
#pragma warning (disable: 4702)  // unreachable code
#endif

#ifndef NDEBUG
//#define YYERROR_VERBOSE 1
#ifdef  YYDEBUG
#    undef YYDEBUG
#endif
#define YYDEBUG 1
#endif

#define yyparse ParseYandexRequest

// this macro is defined in request.h
#ifdef TRACE_RULE_ORDER
#   define PRINT_RULE_NAME(name)    PrintRuleName(name)
#else
#   define PRINT_RULE_NAME(name)   ((void)0)
#endif

namespace
{
#ifdef TRACE_RULE_ORDER
    inline void PrintRuleName(const TString& name) {
        Cout << "---->" << name << "\n";
    }
#endif
    inline bool IsSingleMark(const TWideToken& tok) {
        const TTokenStructure& subtokens = tok.SubTokens;

        if (subtokens.size() <= 1)
            return false; // it isn't multitoken

        const size_t last = subtokens.size() - 1;
        for (size_t i = 0; i < last; ++i) {
            const TCharSpan& s = subtokens[i];
            if (s.TokenDelim != TOKDELIM_NULL)
                return false;
        }

        return true;
    }
    //! processes the node as it is an expression in brackets ("(..)" or "[..]")
    inline void ProcessExpressionInParentheses(TLangPrefix& prefix, TRequestNode& node, TLangSuffix& suffix) {
        SetPrefix(node, prefix.FormType, prefix.Necessity);
        SetSuffix(node, suffix.Idf);
        //if ((!node.IsLeaf() || node.GetMultitoken().SubTokens.size() > 1) && !node.IsEnclosed()) // title:((a-b)) is broken without checking size of multitoken
        //    node.Parens = true;
        if (!node.IsLeaf() && !node.IsEnclosed() // title:((a-b)) is broken without checking size of multitoken
            && !IsSingleMark(node.GetMultitoken()))
            node.Parens = true;
        Y_ASSERT(0 <= prefix.LParenPos && prefix.LParenPos < suffix.RParenPos);
        node.FullSpan = TCharSpan(prefix.LParenPos, suffix.RParenPos + 1 - prefix.LParenPos);
    }
    inline TRequestNode* ProcessBinaryOperator(TReqTokenizer& tokenizer, TRequestNode* left, const TLangSuffix& suffix, TRequestNode* right) {
        if (!left)
            return right;
        if (!right)
            return left;
        return tokenizer.GetNodeFactory().CreateBinaryOper(left, right, DefaultLangPrefix, suffix, PHRASE_USEROP);
    }
}
%}

%union {
        TRequestNode *pnode;
        TLangEntry   *pentry;
        TLangPrefix   prefix;
        TLangSuffix   suffix;
};

%parse-param { TReqTokenizer* toker }
%lex-param   { TReqTokenizer* toker }

%{

#define THIS               toker->request
#define GC(rn)             (rn) // toker->AddToGC(rn)

#define yyerror(toker, str) toker->Error = str
                           // some errors are recovered.
                           // don't set permanent error code here

inline int yylex(YYSTYPE *lval, TReqTokenizer *toker) {
    Y_ASSERT(toker);
    return toker->GetToken(lval);
}

%}

/* nonterminal symbols */
%type <pnode>  expression subexpression and_doc_expr and_sent_expr factor phrase_expr
%type <pnode>  possible_weak_or_expr weak_or_expr synonymizable_phrase_expr
%type <pnode>  synonymizable_and_expr synonymizable_and_doc_expr
%type <pnode>  synonymizable_factor restricted_factor
%type <pnode>  andnot_doc_expr andnot_sent_expr restr_doc_expr

/*
 * пояснения к токенам для тех, кто не хочет читать код сканера.
 *
 * <prefix>     -- заполняет содержимое структуры TLangPrefix
 *                 последовательность символов +,-,%,! (в том числе и нулевая)
 */

/* terminal symbols */
%token <pnode>   ATTR_VALUE ZONE_L_PAREN ZONE_NAME
%token <suffix>  OR AND1 AND2 ANDNOT1 ANDNOT2 REFINE RESTR_DOC WORD_VARIANT_OP
%token <pentry>  IDENT QUOTED_STRING
%token <prefix>  L_PAREN
%token <suffix>  R_PAREN RESTRBYPOS

%start request

%%

request :
     /* no tokens */
   | expression {
        PRINT_RULE_NAME("expression");
        toker->SetResult($1);
     }
   ;

// - the sentence operators (& ~) located below the document operators (' ' && ~~ << | ^ <-)
//   in the tree if request has no parentheses
// - ATTENTION: now the ' ' operator has level 2 by default and has definite priority: it is located
//   between && and ~ (see 'phrase_expr')
//
// - changed behavior of the 'inpos' operator: it influences the left word or expression only
//   [a b inpos:1..2] - 'inpos' influences only 'b', [a (b c) inpos:1..2] - 'inpos' influences '(b c)'

expression :
     subexpression
   | expression  REFINE  subexpression {
         PRINT_RULE_NAME("expression  REFINE  subexpression");
         if (!$3) {
             $$ = GC($1);
         } else if (!$1) {
             $$ = GC(0);
         } else {
             toker->SaveRefine($3);
             $$ = GC($1);
         }
     }
   | expression  REFINE error {
         PRINT_RULE_NAME("expression  REFINE error");
         $$ = GC($1);
         yyerrok;
     }
   | REFINE  subexpression {
         PRINT_RULE_NAME("REFINE  subexpression");
         $$ = GC($2);
     }
   ;

subexpression :
     restr_doc_expr
   | subexpression OR restr_doc_expr {
         PRINT_RULE_NAME("subexpression OR restr_doc_expr");
         $$ = GC(ProcessBinaryOperator(*toker, $1, $2, $3));
     }
   | subexpression OR error {
         PRINT_RULE_NAME("subexpression OR error");
         $$ = GC($1);
         yyerrok;
     }
   | OR restr_doc_expr {
         PRINT_RULE_NAME("OR restr_doc_expr");
         $$ = GC($2);
     }
   ;

restr_doc_expr :
     andnot_doc_expr
   | restr_doc_expr RESTR_DOC andnot_doc_expr {
         PRINT_RULE_NAME("restr_doc_expr RESTR_DOC andnot_doc_expr");
         $$ = GC(ProcessBinaryOperator(*toker, $1, $2, $3));
     }
   | restr_doc_expr RESTR_DOC error {
         PRINT_RULE_NAME("restr_doc_expr RESTR_DOC error");
         $$ = GC($1);
         yyerrok;
     }
   | RESTR_DOC andnot_doc_expr {
         PRINT_RULE_NAME("RESTR_DOC andnot_doc_expr");
         $$ = GC($2);
     }
   ;

andnot_doc_expr :
     possible_weak_or_expr
   | andnot_doc_expr ANDNOT2 possible_weak_or_expr {
         PRINT_RULE_NAME("andnot_doc_expr ANDNOT2 and_doc_expr");
         $$ = GC(ProcessBinaryOperator(*toker, $1, $2, $3));
     }
   | andnot_doc_expr ANDNOT2 error {
         PRINT_RULE_NAME("andnot_doc_expr ANDNOT2 error");
         $$ = GC($1);
         yyerrok;
     }
   | ANDNOT2 possible_weak_or_expr {
         PRINT_RULE_NAME("ANDNOT2 and_doc_expr");
         $$ = GC($2);
     }
   ;

possible_weak_or_expr :
     weak_or_expr
   | and_doc_expr
   ;

weak_or_expr:
     synonymizable_and_doc_expr
   | weak_or_expr WORD_VARIANT_OP synonymizable_and_doc_expr {
         PRINT_RULE_NAME("weak_or_expr WORD_VARIANT_OP synonymizable_and_doc_expr");
         $$ = GC(ProcessBinaryOperator(*toker, $1, $2, $3));
     }
   | weak_or_expr WORD_VARIANT_OP error {
         PRINT_RULE_NAME("weak_or_expr WORD_VARIANT_OP error");
         $$ = GC($1);
         yyerrok;
     }
   | WORD_VARIANT_OP synonymizable_and_doc_expr {
         PRINT_RULE_NAME("WORD_VARIANT_OP synonymizable_and_doc_expr");
         $$ = GC($2);
     }
    ;

synonymizable_and_doc_expr :
     synonymizable_phrase_expr
   | synonymizable_and_doc_expr AND2 synonymizable_phrase_expr {
         PRINT_RULE_NAME("synonymizable_and_doc_expr AND2 synonymizable_phrase_expr");
         $$ = GC(toker->CreatePhrase($1, $2, $3, PHRASE_USEROP));
     }
   | synonymizable_and_doc_expr AND2 error {
         PRINT_RULE_NAME("synonymizable_and_doc_expr AND2 error");
         $$ = GC($1);
         yyerrok;
     }
   | AND2 synonymizable_phrase_expr {
         PRINT_RULE_NAME("AND2 synonymizable_phrase_expr");
         $$ = GC($2);
     }
   ;

synonymizable_phrase_expr :
     synonymizable_and_expr
   | synonymizable_phrase_expr synonymizable_and_expr {
         PRINT_RULE_NAME("synonymizable_phrase_expr synonymizable_and_expr");
         /*здесь пробел трактуется как фраза
           !я иду
           я (иду|бегу)
         */
         TLangSuffix ls = DefaultLangSuffix;
         ls.OpInfo = THIS.PhraseOperInfo;
         $$ = GC(toker->CreatePhrase($1, ls, $2, PHRASE_PHRASE));
     }
   ;

synonymizable_and_expr :
     synonymizable_factor
   | synonymizable_and_expr AND1 synonymizable_factor {
         PRINT_RULE_NAME("synonymizable_and_expr AND1 synonymizable_factor");
         $$ = GC(toker->CreatePhrase($1, $2, $3, PHRASE_USEROP));
     }
   | synonymizable_and_expr AND1 error {
         PRINT_RULE_NAME("synonymizable_and_expr AND1 error");
         $$ = GC($1);
         yyerrok;
     }
   | AND1 synonymizable_factor {
         PRINT_RULE_NAME("AND1 synonymizable_factor");
         $$ = GC($2);
     }
   ;

/* Has ANDONOT1 or RESTRICTBYPOS inside,
otherwise it is synonymizable_and_doc_expr. */
and_doc_expr :
     phrase_expr
   | and_doc_expr AND2 phrase_expr {
         PRINT_RULE_NAME("and_doc_expr AND2 phrase_expr");
         $$ = GC(toker->CreatePhrase($1, $2, $3, PHRASE_USEROP));
     }
   | and_doc_expr AND2 synonymizable_phrase_expr {
         PRINT_RULE_NAME("and_doc_expr AND2 synonymizable_phrase_expr");
         $$ = GC(toker->CreatePhrase($1, $2, $3, PHRASE_USEROP));
     }
   | synonymizable_and_doc_expr AND2 phrase_expr {
         PRINT_RULE_NAME("synonymizable_and_doc_expr AND2 phrase_expr");
         $$ = GC(toker->CreatePhrase($1, $2, $3, PHRASE_USEROP));
     }
   | and_doc_expr AND2 error {
         PRINT_RULE_NAME("and_doc_expr AND2 error");
         $$ = GC($1);
         yyerrok;
     }
   | AND2 phrase_expr {
         PRINT_RULE_NAME("AND2 phrase_expr");
         $$ = GC($2);
     }
   ;

// actually 'phrase_expr' could be joined with 'and_doc_expr' because '&&' and ' ' are equal operators
// 'phrase_expr' created to provide more compatibility with the old phrases, it produces smaller diff in
// the printwzrd test
phrase_expr :
     andnot_sent_expr
   | phrase_expr andnot_sent_expr {
         PRINT_RULE_NAME("phrase_expr andnot_sent_expr");
         /*здесь пробел трактуется как фраза
           !я иду
           я (иду|бегу)
         */
         TLangSuffix ls = DefaultLangSuffix;
         ls.OpInfo = THIS.PhraseOperInfo;
         $$ = GC(toker->CreatePhrase($1, ls, $2, PHRASE_PHRASE));
     }
   | phrase_expr synonymizable_and_expr {
         PRINT_RULE_NAME("phrase_expr synonymizable_and_expr");
         TLangSuffix ls = DefaultLangSuffix;
         ls.OpInfo = THIS.PhraseOperInfo;
         $$ = GC(toker->CreatePhrase($1, ls, $2, PHRASE_PHRASE));
     }
   | synonymizable_phrase_expr andnot_sent_expr {
         PRINT_RULE_NAME("synonymizable_phrase_expr andnot_sent_expr");
         TLangSuffix ls = DefaultLangSuffix;
         ls.OpInfo = THIS.PhraseOperInfo;
         $$ = GC(toker->CreatePhrase($1, ls, $2, PHRASE_PHRASE));
     }
   ;

/* An expression of factors, AND1 and ANDNOT1
with at least one non-synonymizable factor or ANDNOT1,
i.e. not a synonymizable_and_expr. */
 andnot_sent_expr :
     and_sent_expr
   | andnot_sent_expr ANDNOT1 and_sent_expr {
         PRINT_RULE_NAME("andnot_sent_expr ANDNOT1 and_sent_expr");
         $$ = GC(ProcessBinaryOperator(*toker, $1, $2, $3));
     }
   | andnot_sent_expr ANDNOT1 synonymizable_and_expr {
         PRINT_RULE_NAME("andnot_sent_expr ANDNOT1 synonymizable_and_expr");
         $$ = GC(ProcessBinaryOperator(*toker, $1, $2, $3));
     }
   | synonymizable_and_expr ANDNOT1 synonymizable_and_expr {
         PRINT_RULE_NAME("synonymizable_and_expr ANDNOT1 synonymizable_and_expr");
         $$ = GC(ProcessBinaryOperator(*toker, $1, $2, $3));
     }
   | synonymizable_and_expr ANDNOT1 and_sent_expr {
         PRINT_RULE_NAME("synonymizable_and_expr ANDNOT1 and_sent_expr");
         $$ = GC(ProcessBinaryOperator(*toker, $1, $2, $3));
     }
   | andnot_sent_expr ANDNOT1 error {
         PRINT_RULE_NAME("andnot_sent_expr ANDNOT_1 error");
         $$ = GC($1);
         yyerrok;
     }
   | synonymizable_and_expr ANDNOT1 error {
         PRINT_RULE_NAME("synonymizable_and_expr ANDNOT_1 error");
         $$ = GC($1);
         yyerrok;
     }
   | ANDNOT1 and_sent_expr {
         PRINT_RULE_NAME("ANDNOT1 and_sent_expr");
         $$ = GC($2);
     }
   | ANDNOT1 synonymizable_and_expr {
         PRINT_RULE_NAME("ANDNOT1 synonymizable_and_expr");
         $$ = GC($2);
     }
   ;

/* Factors connected by AND1,
at least one of the factors is not synonymizable. */
and_sent_expr :
     restricted_factor
   | and_sent_expr AND1 restricted_factor {
         PRINT_RULE_NAME("and_sent_expr AND1 restricted_factor");
         $$ = GC(toker->CreatePhrase($1, $2, $3, PHRASE_USEROP));
     }
   | and_sent_expr AND1 synonymizable_factor {
         PRINT_RULE_NAME("and_sent_expr AND1 synonymizable_factor");
         $$ = GC(toker->CreatePhrase($1, $2, $3, PHRASE_USEROP));
     }
   | synonymizable_and_expr AND1 restricted_factor {
         PRINT_RULE_NAME("synonymizable_and_expr AND1 restricted_factor");
         $$ = GC(toker->CreatePhrase($1, $2, $3, PHRASE_USEROP));
     }
   | and_sent_expr AND1 error {
         PRINT_RULE_NAME("and_sent_expr AND1 error");
         $$ = GC($1);
         yyerrok;
     }
   | AND1 restricted_factor {
         PRINT_RULE_NAME("AND1 restricted_factor");
         $$ = GC($2);
     }
   ;

restricted_factor :
     factor
   | factor RESTRBYPOS {
         PRINT_RULE_NAME("factor RESTRBYPOS");
         $$ = GC(toker->GetNodeFactory().CreateBinaryOper($1, 0, DefaultLangPrefix, $2, PHRASE_USEROP));
     }
   | synonymizable_factor RESTRBYPOS {
         PRINT_RULE_NAME("synonymizable_factor RESTRBYPOS");
         $$ = GC(toker->GetNodeFactory().CreateBinaryOper($1, 0, DefaultLangPrefix, $2, PHRASE_USEROP));
     }
   | RESTRBYPOS { // this rule adds one shift/reduce conflict, but removes all syntax errors that could be caused by the inpos attribute
         PRINT_RULE_NAME("RESTRBYPOS");
         $$ = NULL;
     }
   ;

/*
 я иду        -- фраза -- level и расстояние задаётся отдельно
 я & иду      -- в одном абзаце (level = 1,  расстояние - нулевое)
 я /2 иду     -- расстояние в словах (level = 0, расстояние - /2 в словах)
 я && иду     -- в одном документе (level = 2, расстояние - нулевое)
 я &/2 иду    -- расстояние в словах (level = 0, расстояние - /2 в словах)
 я &&/2 иду   -- расстояние в абзацах (level = 1, расстояние - /2 в абзацах)

 я /0 иду     -- расстояние в словах (level = 0, расстояние - /0 в словах)
                 позиции должны совпадать
 я &/0 иду    -- расстояние в словах (level = 0, расстояние - /0 в словах)
                 позиции должны совпадать
 я &&/0 иду   -- расстояние в абзацах (level = 1, расстояние - /0 в абзацах)
                 то же что и "я & иду"

 если расстояние - нулевое, то поиск - внутри структ. единицы
 задание расстояния (в том числе и нулевого!) понижает level на 1
*/

synonymizable_factor :
     L_PAREN synonymizable_and_doc_expr R_PAREN {
        PRINT_RULE_NAME("L_PAREN synonymizable_and_doc_expr R_PAREN");
        $$ = GC($2);
        if ($$)
           ProcessExpressionInParentheses($1, *$$, $3);
     }
   | IDENT {
        PRINT_RULE_NAME("IDENT");
        $$ = GC(toker->GetNodeFactory().CreateWordNode(*$1, toker->GetMultitoken(), toker->GetTokenTypes()));
        toker->ClearSubtokens();
        toker->DropTokenTypes();
     }
   | QUOTED_STRING {
        PRINT_RULE_NAME("QUOTED_STRING as a synonymizable_factor");
        $$ = GC(ParseQuotedString(*toker, *$1));
     }


factor :
     L_PAREN  expression  R_PAREN {
        PRINT_RULE_NAME("L_PAREN  expression  R_PAREN");
        $$ = GC($2);
        if ($$)
           ProcessExpressionInParentheses($1, *$$, $3);
     }
   | ATTR_VALUE {
        PRINT_RULE_NAME("ATTR_VALUE");
        $$ = GC($1);
     }
   | L_PAREN R_PAREN {
        PRINT_RULE_NAME("L_PAREN R_PAREN");
        $$ = 0;
     }
   | ZONE_L_PAREN subexpression R_PAREN {
        PRINT_RULE_NAME("ZONE_L_PAREN expression R_PAREN");
        $$ = GC(ConvertAZNode($1, $2));
     }
   | ZONE_L_PAREN subexpression error {
        PRINT_RULE_NAME("ZONE_L_PAREN expression error");
        $$ = GC(ConvertZoneExprToPhrase($1, $2, *toker));
        yyerrok;
     }
   | ZONE_NAME QUOTED_STRING {
        PRINT_RULE_NAME("ZONE_NAME QUOTED_STRING");
        TRequestNode* node = ParseQuotedString(*toker, *$2);
        $$ = GC(ConvertAZNode($1, node));
     }
   ;

%%

//------------------- constants for request printing  ------------------

const char * LEFT_PAREN_STR = "(";
const char * RIGHT_PAREN_STR  = ")";

const char * AND_STR = "&";
const char * AND_NOT_STR = "~";
const char * OR_STR = "|";
const char * WEAK_OR_STR = "^";
const char * REFINE_STR = "<-";
const char * RESTR_DOC_STR = "<<";

const char * NEAR_PREFFIX_STR = "/";
const char * REVERSE_FREQ_PREFFIX_STR = "::";

const char * CMP_LE_STR = "<=";
const char * CMP_GE_STR = ">=";

const char * NEW_CMP_LT_STR = ":<";
const char * NEW_CMP_LE_STR = ":<=";
const char * NEW_CMP_EQ_STR = ":";
const char * NEW_CMP_GE_STR = ":>=";
const char * NEW_CMP_GT_STR = ":>";

const char * EXACT_WORD_PREFFIX_STR = "!";
const char * EXACT_LEMMA_PREFFIX_STR = "!!";
const char * BEG_ATTRVALUE_STR = "\"";
const char * NEAR_DELIM_STR = "\"";

const char * BEG_RESTRBYPOS_STR = "[[";
const char * END_RESTRBYPOS_STR = "]]";
const char * COMMA_STR = ",,";


//new params for new syntax
const char * CMP_RIGHT_ARROW = "->";
const char * NEW_DOUBLE_DOT = "..";
//


//----------------------------------------------------------------------
