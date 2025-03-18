%{
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include <util/generic/string.h>

#include <library/cpp/html/sanitize/css/gc.h>
#include <library/cpp/html/sanitize/css/expressions.h>

#include <library/cpp/html/sanitize/css/css2-drv.h>
#include <library/cpp/html/sanitize/css/css2-lexer.h>

#ifdef WITH_DEBUG_OUT
    #ifdef YYDEBUG
        #undef YYDEBUG
    #endif
    #define YYDEBUG 1
    #define DEBUG_OUT(a) driver.OutDebug a
#else
    #define DEBUG_OUT(a)
#endif


inline TString ToStr(double v)
{
    char buf[64];
    sprintf(buf, "%g", v);

    return TString(buf);
}


#if 0
void operator delete(void *) noexcept
{
    Cout << "delete()\n";
}
#endif

%}

%pure-parser

%union
{
  int ival;
  float fval;
  NCssSanit::TNodePtr<TString>* text;
  NCssSanit::TNodePtr<NCssConfig::TContext>* context;
  NCssSanit::TNodePtr<NCssConfig::TContextList>* context_list;
  NCssSanit::TNodePtr<NCssSanit::TStrokaList>* stroka_list;
}

%token        END  0  "end of file"
%token <text> STRING IDENT HASH DIMEN FUNCTION  URI EXPRESSION
%token <fval> EMS EXS LENGTH_PX
%token <fval> LENGTH_CM LENGTH_MM LENGTH_IN LENGTH_PT LENGTH_PC
%token <fval> ANGLE_DEG ANGLE_RAD ANGLE_GRAD
%token <fval> TIME_MS TIME_S
%token <fval> FREQ_KHZ FREQ_HZ PERCENTAGE NUMBER

%token <ival> S CHARSET_SYM IMPORT_SYM COMMA_T LBRACE URL
%token <ival> RBRACE MEDIA_SYM
%token <ival> PAGE_SYM PLUS MINUS GREATER INCLUDES DASHMATCH
%token <ival> IMPORTANT_SYM
%token <ival> INVALID DIMENSION FONT_FACE_SYM
%token <ival> ATKEYWORD UNICODERANGE
%token <ival> START_FULL START_INLINE

%type <context_list> term_list expr opt_expr
%type <context> term operator opt_term term_number hexcolor function
%type  <text> string_or_uri  url_stmt
%type  <ival> unary_operator opt_unary_operator
%type <text> simple_selector selector combinator
%type <text> predicate_list predicate hd_item elt
%type <text> opt_attrib_quals attrib_qual ident_or_string opt_ident pseudo
%type <text> declaration ruleset ruleset_list rule_declaration_list opt_declaration opt_prio prio property
%type <text> opt_pseudo_page pseudo_page medium medium_list opt_medium_list
%type <text> declaration_list
%type <stroka_list> selector_list

%start start


%right COMMA_T
%left S PLUS GREATER

%parse-param { NCssSanit::TCSS2Driver& driver }
%lex-param { NCssSanit::TCSS2Driver& driver }

%{

using namespace NCssSanit;

#define yyparse         yyparse_css2

static int yylex(YYSTYPE *locp, NCssSanit::TCSS2Driver& drv)
{
    return drv.Lexer->Lex(locp);
}

static void yyerror(NCssSanit::TCSS2Driver& drv, const char* str)
{
    drv.Error(str);
}

%}


%%

/* We can handle both full css and inline style definitions.
   See also css2_l.l
*/
start
    : START_FULL stylesheet

    | START_INLINE opt_s rule_declaration_list
    {
        driver.Out($3->Value);
        delete $3;
    }
    ;

stylesheet
  : opt_s
    opt_charset
   import_list
    chooser_list
    opt_s
  ;

opt_charset
    :
    | CHARSET_SYM STRING ';'
    {
        driver.Out ($1, $2->Value, ";");
        delete ($2);
    }
    ;


import_list
  :
  | import_list import
  ;

chooser_list
  :
  | chooser_list chooser
  ;

chooser
    :
    ruleset
    {
        driver.Out($1->Value);
        if (!$1->Value.empty())
            driver.Out('\n');
        delete $1;
    }
    | media
    | page
  ;

import
  : IMPORT_SYM url_stmt opt_s opt_medium_list ';'
    {
        TString url = driver.HandleImportUrl($2->Value);
        if (!url.empty())
        {
            driver.Out("@import url(\"", url, "\")");
            if (!$4->Value.empty())
                driver.Out(' ', $4->Value);
            driver.Out(";\n");
        }
        delete $2;
        delete $4;
    }
  | IMPORT_SYM error ';'
  ;

string_or_uri
  : STRING
    {
        $$ = $1;
    }
  | URI
    {
        $$ = $1;
    }
  ;

url_stmt
    : string_or_uri
    {
        $$ = $1;
    }
    ;

opt_medium_list
    :
    {
        $$ = driver.New<TString>();
    }
    | medium_list
    {
        $$ = $1;
    }
  ;

medium_list
    : medium
    {
        $$ = $1;
    }
    | medium_list COMMA_T medium
    {
        $$ = $1;

        bool comma = true;

        if ($1->Value.empty())
            comma = false;

        if (!$3->Value.empty())
        {
            if (comma)
                $$->Value += ",";

            $$->Value += $3->Value;
        }
        delete $3;
    }
    ;

media
    : MEDIA_SYM opt_s medium_list LBRACE ruleset_list RBRACE
    {
        driver.Out("@media ");
        driver.Out($3->Value);
        driver.Out("\n{\n", $5->Value.c_str(), "\n}\n");

        delete $3;
        delete $5;
    }
  ;

ruleset_list
  :
    {
        $$ = driver.New<TString>();
    }
  | ruleset_list ruleset
    {
        $$ = $1;
        if (!$1->Value.empty())
            $$->Value += "\n";
        $$->Value += $2->Value;

        delete $2;
    }
  ;

medium
  : IDENT
    {
        $$ = $1;
    }
  ;

page
  : PAGE_SYM opt_pseudo_page LBRACE opt_declaration declaration_list  RBRACE
    {
        driver.Out("@page ");

        if (!$2->Value.empty())
            driver.Out($2->Value, ' ');

        driver.Out('{', $4->Value, $5->Value, "}\n");

        delete $2;
        delete $4;
        delete $5;
    }
  ;

declaration_list
    :
    {
        $$ = driver.New<TString>();
    }

    | declaration_list ';' opt_declaration
    {
        $$ = $1;
        $$->Value += ";";
        $$->Value += $3->Value;
        delete $3;
    }
    ;

opt_pseudo_page
  :
    {
        $$ = driver.New<TString>();
    }

  | pseudo_page
    {
        $$ = $1;
    }
  ;

pseudo_page
  : ':' IDENT
    {
        $$ = $2;
        $$->Value.insert(size_t(0), 1, ':');
    }
  ;

operator
  : '/' opt_s
    {
        $$ = driver.New<TContext>();
        $$->Value.ValueType = VT_SEPARATOR;
        $$->Value.PropValue = "/";
    }
  | COMMA_T
    {
        $$ = driver.New<TContext>();
        $$->Value.ValueType = VT_SEPARATOR;
        $$->Value.PropValue = ",";
    }
/*  | S { driver.Out(' '); } */
  | '=' opt_s { driver.Out('='); }
    {
        $$ = driver.New<TContext>();
        $$->Value.ValueType = VT_SEPARATOR;
        $$->Value.PropValue = "=";
    }
  ;

unary_operator
  : '-'  { /*driver.Out('-');*/ $$ = MINUS; }
  | PLUS { driver.Out('+'); $$ = PLUS; }
  ;

property
  : IDENT
    {
        $$ = $1;
    }
  ;

ruleset
  :
    selector_list
    LBRACE
    rule_declaration_list
    RBRACE
    {
        $$ = driver.New<TString>();

        if (!$3->Value.empty())
        {
            if (driver.GetPolicy().DefaultDeny())
                $$->Value = driver.MakeSelectorsAll($1->Value);
            else
                $$->Value = driver.MakeSelectorsDefault($1->Value);

            if (!$$->Value.empty())
            {
                $$->Value += "{" + $3->Value + "}";
            }
        }


        delete $1;
        delete $3;
    }
    | selector_list LBRACE error RBRACE
    {
        $$ = driver.New<TString>();
        delete $1;
    }
  ;


selector_list
  : selector
    {
        $$ = driver.New<TStrokaList>();
        $$->Value.push_back ($1->Value);
        delete $1;
    }
  | selector_list COMMA_T  selector
    {
        $$ = $1;
        $$->Value.push_back ($3->Value);
/*
        bool comma = true;;

        if ($$->empty())
            comma = false;

        if (driver.PassSelector(*$3))
        {
            if (comma)
                *$$ += ",";

            *$$ += *$3;
        }
*/
        delete $3;
    }
  ;


selector
  : simple_selector
    {
        $$ = $1;
    }
  | selector combinator simple_selector
    {
        $$ = $1;
        $$->Value += $2->Value + $3->Value;

        delete $2;
        delete $3;
    }

  ;

combinator
  : S
    {
        $$ = driver.New<TString>();
        $$->Value = " ";
    }
  | PLUS
    {
        $$ = driver.New<TString>();
        $$->Value = "+";
    }
  | GREATER
    {
        $$ = driver.New<TString>();
        $$->Value = ">";
    }
  ;

simple_selector
  : predicate_list
    {
        $$ = $1;
    }
  ;

hd_item
  : elt
    {
        $$ = $1;
    }
  | predicate
    {
        $$ = $1;
    }
  ;


/* gets selp, returns selp */
elt
  : IDENT
    {
        $$ = $1;
    }
  | '*'
    {
        $$ = driver.New<TString>();
        $$->Value = "*";
    }
  ;

/* gets selp, returns selp */
predicate_list
  : hd_item
    {
        $$ = $1;
    }
  | predicate_list predicate
    {
        $$ = $1;
        $$->Value += $2->Value;
        delete $2;
    }
  ;

predicate
  : HASH
    {
        $$ = $1;
    }
  | '.' IDENT
    {
        $$ = $2;
        $$->Value.insert(size_t(0), 1, '.');
    }
  | '['  IDENT opt_attrib_quals ']'
    {
        $$ = $2;
        $$->Value.insert (size_t(0), 1, '[');
        $$->Value += $3->Value + ']';
        delete $3;
    }
  | ':' pseudo
    {
        $$ = $2;
        $$->Value.insert(size_t(0), 1, ':');
    }
  ;

opt_attrib_quals
  :
    {
        $$ = driver.New<TString>();
    }
  | attrib_qual  ident_or_string
    {
        $$ = $1;
        $$->Value += $2->Value;
        delete $2;
    }
  ;

attrib_qual
  : '='
    {
        $$ = driver.New<TString>();
        $$->Value = "=";
    }
  | INCLUDES
    {
        $$ = driver.New<TString>();
        $$->Value = "~=";
    }

  | DASHMATCH
    {
        $$ = driver.New<TString>();
        $$->Value = "!=";
        //driver.Out("!=");
    }
  ;

ident_or_string
  : IDENT
    {
        $$ = $1;
    }
  | STRING
    {
        $$ = $1;
    }
  ;

pseudo
  : IDENT
    {
        $$ = $1;
    }
  | FUNCTION opt_ident  ')'
    {
        $$ = $1;
        $$->Value += "(" + $2->Value + ")";
        delete $2;
    }
    ;

opt_ident
  :
    {
        $$ = driver.New<TString>();
    }
  | IDENT
    {
        $$ = $1;
    }
  ;


rule_declaration_list
    :
    {
        $$ = driver.New<TString>();
    }
    | declaration
    {
        $$ = $1;
        if (!$$->Value.empty())
            $$->Value += ';';
    }
    | rule_declaration_list ';' declaration
    {
        $$ = $1;

        $$->Value += $3->Value;

        if ($$->Value.size() && $$->Value.back() != ';')
            $$->Value += ";";

        delete $3;
    }
    | rule_declaration_list ';'
    {
        $$ = $1;
        //*$$ += ';';
    }
    ;

opt_declaration
    :
    {
        $$ = driver.New<TString>();
    }
    | declaration
    {
        $$ = $1;
    }
    ;

declaration
    :
    property ':' expr opt_prio
    {
        $$ = $1;

        TString expr_str = driver.PropertyToString($1->Value, $3->Value);
        if (!expr_str.empty())
        {
            $$->Value += ":" + expr_str;
            if (!$4->Value.empty())
                $$->Value += " " + $4->Value;
        } else
            $$->Value = "";

        delete $3;
        delete $4;
    }
    ;


opt_prio
    :
    {
        $$ = driver.New<TString>();
    }
    | prio opt_s
    {
        $$ = $1;
    }
    ;

prio
    : IMPORTANT_SYM
    {
        $$ = driver.New<TString>();
        $$->Value = "!important";
    }
    ;

expr
    : term_list opt_s
    {
        $$ = $1;
    }

  ;

opt_expr
    :
    {
        $$ = driver.New<TContextList>();
    }

    | expr
    {
        $$ = $1;
    }
    ;

term_list
  : term
    {
        $$ = driver.New<TContextList>();
        $$->Value.push_back($1->Value);
        delete $1;
    }

  | term_list operator term
    {
        $$ = $1;
        $$->Value.push_back($2->Value);
        $$->Value.push_back($3->Value);
        delete $2;
        delete $3;
    }
  | term_list S opt_term
    {
        $$ = $1;
        if ($3)
        {
            TContext context;
            context.ValueType = VT_SEPARATOR;
            context.PropValue = " ";
            $$->Value.push_back(context);

            $$->Value.push_back($3->Value);
            delete $3;
        }
    }
  ;

term
  : opt_unary_operator term_number
    {
        $$ = $2;

        if ($1 == MINUS)
        {
            $$->Value.PropValue = TString("-") + $$->Value.PropValue;
            $$->Value.PropValueDouble = -$$->Value.PropValueDouble;
        }
    }
  | STRING
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValue = $1->Value;
        $$->Value.ValueType = VT_STRING;
        delete($1);
    }
  | IDENT
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValue = $1->Value;
        $$->Value.ValueType = VT_STRING;
        delete($1);
    }
  | URI
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValue = $1->Value;
        $$->Value.ValueType = VT_URI;
        delete($1);
    }
  | hexcolor
    {
        $$ = $1;
    }
  | function
    {
        $$ = $1;
    }
  ;

opt_term
  :
    {
        $$ = 0;
    }
  | term
    {
        $$ = $1;
    }
  ;

term_number
  : NUMBER
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValueDouble = $1;
        $$->Value.PropValue = ToStr($1);
        $$->Value.ValueType = VT_NUMBER;
    }
  | PERCENTAGE
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValueDouble = $1;
        $$->Value.PropValue = ToStr($1);
        $$->Value.Unit = "%";
        $$->Value.ValueType = VT_NUMBER;
    }
  | LENGTH_PX
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValueDouble = $1;
        $$->Value.PropValue = ToStr($1);
        $$->Value.Unit = "px";
        $$->Value.ValueType = VT_NUMBER;
    }
  | LENGTH_CM
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValueDouble = $1;
        $$->Value.PropValue = ToStr($1);
        $$->Value.Unit = "cm";
        $$->Value.ValueType = VT_NUMBER;
    }
  | LENGTH_MM
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValueDouble = $1;
        $$->Value.PropValue = ToStr($1);
        $$->Value.Unit = "mm";
        $$->Value.ValueType = VT_NUMBER;
    }
  | LENGTH_IN
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValueDouble = $1;
        $$->Value.PropValue = ToStr($1);
        $$->Value.Unit = "in";
        $$->Value.ValueType = VT_NUMBER;
    }
  | LENGTH_PT
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValueDouble = $1;
        $$->Value.PropValue = ToStr($1);
        $$->Value.Unit = "pt";
        $$->Value.ValueType = VT_NUMBER;
    }
  | LENGTH_PC
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValueDouble = $1;
        $$->Value.PropValue = ToStr($1);
        $$->Value.Unit = "pc";
        $$->Value.ValueType = VT_NUMBER;
    }
  | EMS
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValueDouble = $1;
        $$->Value.PropValue = ToStr($1);
        $$->Value.Unit = "em";
        $$->Value.ValueType = VT_NUMBER;
    }
  | EXS
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValueDouble = $1;
        $$->Value.PropValue = ToStr($1);
        $$->Value.Unit = "ex";
        $$->Value.ValueType = VT_NUMBER;
    }
  | ANGLE_DEG
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValueDouble = $1;
        $$->Value.PropValue = ToStr($1);
        $$->Value.Unit = "deg";
        $$->Value.ValueType = VT_NUMBER;
    }
  | ANGLE_RAD
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValueDouble = $1;
        $$->Value.PropValue = ToStr($1);
        $$->Value.Unit = "rad";
        $$->Value.ValueType = VT_NUMBER;
    }
  | ANGLE_GRAD
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValueDouble = $1;
        $$->Value.PropValue = ToStr($1);
        $$->Value.Unit = "grad";
        $$->Value.ValueType = VT_NUMBER;
    }
  | TIME_MS
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValueDouble = $1;
        $$->Value.PropValue = ToStr($1);
        $$->Value.Unit = "ms";
        $$->Value.ValueType = VT_NUMBER;
    }
  | TIME_S
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValueDouble = $1;
        $$->Value.PropValue = ToStr($1);
        $$->Value.Unit = "s";
        $$->Value.ValueType = VT_NUMBER;
    }
  | FREQ_HZ
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValueDouble = $1;
        $$->Value.PropValue = ToStr($1);
        $$->Value.Unit = "hz";
        $$->Value.ValueType = VT_NUMBER;
    }

  | FREQ_KHZ
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValueDouble = $1;
        $$->Value.PropValue = ToStr($1);
        $$->Value.Unit = "khz";
        $$->Value.ValueType = VT_NUMBER;
    }
  ;

opt_unary_operator
  : { $$ = PLUS; }
  | unary_operator { $$ = $1; }
  ;

function
  : FUNCTION opt_expr ')'
    {
        $$ = driver.New<TContext>();
        $$->Value.ValueType = VT_FUNCTION;
        $$->Value.PropValue = $1->Value;
        if ($2)
        {
            $$->Value.FuncArgs = $2->Value;
            delete $2;
        }

        delete($1);
    }
  | EXPRESSION
    {
        $$ = driver.New<TContext>();
        $$->Value.ValueType = VT_EXPRESSION;
        $$->Value.PropValue = TString("expression(") + $1->Value;
        delete($1);
    }
  ;
/*
 * There is a constraint on the color that it must
 * have either 3 or 6 hex-digits (i.e., [0-9a-fA-F])
 * after the "#"; e.g., "#000" is OK, but "#abcd" is not.
 */
hexcolor
  : HASH
    {
        $$ = driver.New<TContext>();
        $$->Value.PropValue = $1->Value;
        $$->Value.ValueType = VT_HASHVALUE;
        delete($1);
    }
  ;

opt_s
  :
  | opt_s S { /*driver.Out(" ");*/ }
  ;
%%
