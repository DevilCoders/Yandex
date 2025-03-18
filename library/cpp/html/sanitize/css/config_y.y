%{
#include <cstdio>
#include <util/system/compat.h>

#include <library/cpp/html/sanitize/css/config-drv.h>
#include <library/cpp/html/sanitize/css/config-lexer.h>

#ifdef YYPREFIX
#   undef YYPREFIX
#endif
#define YYPREFIX YYCONFIG

#ifdef WITH_DEBUG_OUT
//    #define YYDEBUG 1
#endif

//#define YYLEX_PARAM void

#include <library/cpp/html/sanitize/css/config-drv.h>

using namespace NCssConfig;

%}

%pure-parser

%union
{
    int ival;
    double fval;
    TString * text;
    EActionType action;
    TEssence * essence;
    TEssenceSet * essence_set;
    TExprVal * expr_value;
    TBoolExpr  * expr_bool;
    TImportAction * import_action;
    TStrokaList * stroka_list;
}

%token        END  0  "end of file"
%token <ival> PROPERTY
%token <ival> SELECTOR
%token <ival> APPEND
%token <ival> EXPRESSION
%token <ival> IMPORT
%token <ival> DEFAULT
%token <ival> REWRITE
%token <ival> SCHEME
%token <action> PASS DENY MERGE
%token <text> IDENT
%token <text> STRING
%token <text> REGEXP
%token <ival> AND
%token <ival> OR
%token <ival> NOT
%token <ival> LESS_EQUAL
%token <ival> GREAT_EQUAL
%token <ival> MATCH
%token <fval> NUMBER
%token <ival> CLASS
%token <ival> FORM
%token <ival> IFRAME

%type <action>          pass_deny
%type <expr_bool>       opt_value_check
%type <expr_bool>       value_check
%type <expr_bool>       check_expr
%type <expr_bool>       check_atom
%type <expr_value>      check_what
%type <essence_set>     prop_list
%type <essence>         ident_or_regexp
%type <essence>         prop_descr
%type <text>            import_option_proxy
%type <import_action>   import_option
%type <stroka_list>     string_list

/*%destructor { delete $$; } IDENT STRING REGEXP*/

%left OR
%left AND
%left NOT
%left NEG

%parse-param  { NCssConfig::TConfigDriver& driver }
%lex-param    { NCssConfig::TConfigDriver& driver }

%{

static int yylex(YYSTYPE *locp, NCssConfig::TConfigDriver& driver)
{
    return driver.Lexer->Lex(locp);
}

static void yyerror(NCssConfig::TConfigDriver& driver, const char* str)
{
    driver.error(str);
}


#include <library/cpp/html/sanitize/css/config-drv.h>

bool ident_is_regexp = false;


#define yyparse yyparse_config

%}

%%


config
    : stmt_list
    ;

stmt_list
    :
    | stmt_list stmt
    ;

stmt
    : default_stmt
    | property_stmt
    | selector_stmt
    | expression_stmt
    | class_stmt
    | import_stmt
    | scheme_stmt
    | selector_append_stmt
    ;

default_stmt
    : DEFAULT pass_deny
    {
        driver.Config.DefaultPass = ($2 == AT_PASS ? true : false);
    }
    ;

property_stmt
    : PROPERTY pass_deny  '(' prop_list ')'
    {
        switch($2) {
            case AT_DENY:
                driver.Config.PropertyDeny().AddEssenceSet(*$4);
                break;
            case AT_PASS:
                driver.Config.PropertyPass().AddEssenceSet(*$4);
                break;
            default:
                break;
        }
        delete $4;
    }
    ;

prop_list
    :
    {
        $$ = new NCssConfig::TEssenceSet;
    }
    | prop_list prop_descr
    {
        $$ = $1;
        $$->Add (*$2);
        delete $2;
    }
    ;

prop_descr
    : ident_or_regexp opt_value_check
    {
        $$ = $1;
        if ($2) {
            $$->AssignExpr ($2);
            delete $2;
        }
    }
    ;

ident_or_regexp
    : IDENT
    {
        $$ = new TEssence(*$1, false);
        delete $1;
    }
    | REGEXP
    {
        $$ = new TEssence(*$1, true);
        delete $1;
    }
    ;

opt_value_check
    : { $$ = 0; }
    | value_check { $$ = $1; }
    ;

value_check
    : '{' check_expr '}' { $$ = $2; }
    ;

check_expr
    : check_atom
    {
        $$ = $1;
    }
    | '(' check_expr ')'
    {
        $$ = $2;
    }
    | check_expr AND check_expr
    {
        $$ = new TBoolExprAnd (*$1, *$3);
        delete $1;
        delete $3;
    }
    | check_expr OR check_expr
    {
        $$ = new TBoolExprOr (*$1, *$3);
        delete $1;
        delete $3;
    }
    | NOT check_expr
    {
        $$ = new TBoolExprNot (*$2);
        delete $2;
    }
    ;

check_atom
    : check_what '>' check_what
    {
        $$ = new TBoolExprCmpGreat (*$1, *$3);
        delete $1;
        delete $3;
    }
    | check_what '<' check_what
    {
        $$ = new TBoolExprCmpLess (*$1, *$3);
        delete $1;
        delete $3;
    }
    | check_what '=' check_what
    {
        $$ = new TBoolExprCmpEqual (*$1, *$3);
        delete $1;
        delete $3;
    }
    | check_what LESS_EQUAL check_what
    {
        $$ = new TBoolExprCmpLessEqual (*$1, *$3);
        delete $1;
        delete $3;
    }
    | check_what GREAT_EQUAL check_what
    {
        $$ = new TBoolExprCmpGreatEqual (*$1, *$3);
        delete $1;
        delete $3;
    }
    | check_what MATCH check_what
    {
        $$ = new TBoolExprCmpMatch (*$1, *$3);
        delete $1;
        delete $3;
    }
    ;

check_what
    : IDENT
    {
        if (stricmp($1->c_str(), "value") == 0)
            $$ = new TExprValValue();
        else if (stricmp($1->c_str(), "unit") == 0)
            $$ = new TExprValUnit();
        else
            $$ = new TExprVal(*$1, false);
        delete $1;
    }
    | NUMBER { $$ = new TExprVal($1); }
    | '-' NUMBER %prec NEG { $$ = new TExprVal(-$2); }
    | STRING { $$ = new TExprVal(*$1, false); delete $1; }
    | REGEXP { $$ = new TExprVal(*$1, true); delete $1; }
/*    | '(' check_what ')'
    {
        $$ = $2;
    }*/
    ;


pass_deny
    : PASS { $$ = AT_PASS; }
    | DENY { $$ = AT_DENY; }
    ;

selector_stmt
    : SELECTOR pass_deny  '(' prop_list ')'
    {
        switch($2) {
            case AT_DENY:
                driver.Config.SelectorDeny().AddEssenceSet(*$4);
                break;
            case AT_PASS:
                driver.Config.SelectorPass().AddEssenceSet(*$4);
                break;
            default:
                break;
        }
        delete $4;
    }
    ;

expression_stmt
    : EXPRESSION pass_deny
    {
        driver.Config.ExpressionPass = ($2 == AT_PASS ? true : false);
    }

    ;

class_synonyms
    : CLASS
    | FORM
    | IFRAME
    ;

class_stmt
    : class_synonyms pass_deny  '(' prop_list ')'
    {
        switch($2) {
            case AT_DENY:
                driver.Config.ClassDeny().AddEssenceSet(*$4);
                break;
            case AT_PASS:
                driver.Config.ClassPass().AddEssenceSet(*$4);
                break;
            default:
                break;
        }
        delete $4;
    }
    ;

import_stmt
    : IMPORT import_option
    {
        driver.Config.ImportAction = *$2;
        delete $2;
    }
    ;

import_option
    : PASS
    {
        $$ = new TImportAction(AT_PASS, TString());
    }
    | DENY
    {
        $$ = new TImportAction(AT_DENY, TString());
    }
    | MERGE
    {
        $$ = new TImportAction(AT_MERGE, TString());
    }
    | import_option_proxy
    {
        $$ = new TImportAction(AT_REWRITE, *$1);
        delete $1;
    }
    ;

import_option_proxy
    : REWRITE STRING
    {
        $$ = $2;
    }
    ;

scheme_stmt
    : SCHEME pass_deny  '(' prop_list ')'
    {
        switch($2) {
            case AT_DENY:
                driver.Config.SchemeDeny().AddEssenceSet(*$4);
                break;
            case AT_PASS:
                driver.Config.SchemePass().AddEssenceSet(*$4);
                break;
            default:
                break;
        }
        delete $4;
    }
    ;

selector_append_stmt
    : SELECTOR APPEND string_list
    {
        driver.Config.AddSelectorAppend(*$3);
        delete $3;
    }
    ;

string_list
    : STRING
    {
        $$ = new TStrokaList;
        $$->push_back(*$1);
        delete $1;
    }
    | string_list STRING
    {
        $$ = $1;
        $$->push_back(*$2);
        delete $2;
    }
    ;
%%
