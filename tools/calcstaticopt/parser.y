%{
#include <tools/calcstaticopt/common.h>
#include <tools/calcstaticopt/parser.h>
#include <util/string/printf.h>
#include <util/string/cast.h>

int yylex(YYSTYPE* yylval, void* ptr);

void yyerror(void* /*ptr*/, char const *s) {
    fprintf(stderr, "%s\n", s);
    exit(1);
}

static TState* State = NULL;

%}

%parse-param {void* ptr}
%lex-param   {void* ptr}

%pure-parser

%union {
    const char* str;
    struct TBlock* block;
    int num;
    TVector<TString>* vec;
}

%token<str> ID
%token<str> NUM
%token<str> QUOTED
%token<str> CPP_EXPR
%token<str> CPP_BLOCK

%token<str> FACTOR
%token<str> BLOCK
%token<str> STRUCTURE
%token<str> FASTRANK_FORMULAS
%token<str> FASTRANK_FACTORS
%token<str> EXTERNAL_FACTORS
%token<str> ARGS

%token<str> DEPENDS
%token<str> COMPUTES
%token<str> T_FLOAT

%type<block> block
%type<block> block_head
%type<vec> identifiers
%type<vec> identifiers_and_fields
%type<num> field_type

%%

input:
    /* empty */
  | input statement;

statement:
    directive
  | block { State->AddBlock($1); }
  ;

directive:
    FASTRANK_FORMULAS QUOTED ';'     { State->FastRankFiles.push_back($2); }
  | FASTRANK_FACTORS identifiers ';' { State->FastRankFactors.insert(State->FastRankFactors.end(), $2->begin(), $2->end()); delete $2; }
  | EXTERNAL_FACTORS identifiers ';' { State->ExternalFactors.insert(State->ExternalFactors.end(), $2->begin(), $2->end()); delete $2; }
  | ARGS QUOTED ';'                  { State->ArgList = $2; }
  ;

block:
    block_head '=' ID '.' ID ':' field_type ';' {
        $$ = $1;
        const char* structure = $3;
        const char* field = $5;
        int bitness = $7;

        if ($$->Type != TBlock::EXPR_FACTOR)
            yyerror(ptr, "Syntax error in factor statement\n");

        $$->Type = TBlock::SIMPLE_FACTOR;
        $$->Deps.push_back(Sprintf("%s.%s", structure, field));

        if (bitness == -1) {  // float
            $$->Code = Sprintf("factor[%s] = %s.%s;", $$->Id.c_str(), structure, field);
        } else if (bitness == 1) {
            $$->Code = Sprintf("factor[%s] = BOOL_TO_FLOAT(%s.%s);", $$->Id.c_str(), structure, field);
        } else if (bitness >= 2 && bitness <= 16) {
            $$->Code = Sprintf("factor[%s] = TUi2Float<%d>{}(%s.%s);", $$->Id.c_str(), bitness, structure, field);
        } else {
            Cerr << "Unsupported field bitness: " << bitness << Endl;
            exit(1);
        }
    }

  | block_head '=' CPP_EXPR ';' {
        $1->Code = Sprintf("factor[%s] = %s;", $1->Id.c_str(), $3);
        $$ = $1;
    }

  | block_head CPP_BLOCK {
        $1->Code = $2;
        $$ = $1;
    }
  ;

block_head:
    FACTOR ID {
        $$ = new TBlock();
        $$->Id = $2;
        $$->Type = TBlock::EXPR_FACTOR;
        $$->Factors.push_back($2);
    }

  | BLOCK ID {
        $$ = new TBlock();
        $$->Id = $2;
        $$->Type = TBlock::USER_BLOCK;
    }

  | STRUCTURE ID {
        $$ = new TBlock();
        $$->Id = $2;
        $$->Type = TBlock::STRUCTURE_BLOCK;
    }

  | block_head DEPENDS '(' identifiers_and_fields ')' {
        $1->Deps.insert($1->Deps.end(), $4->begin(), $4->end());
        delete $4;
    }

  | block_head COMPUTES '(' identifiers ')' {
        $1->Factors.insert($1->Factors.end(), $4->begin(), $4->end());
        delete $4;
    }
  ;

identifiers:
    ID {
        $$ = new TVector<TString>();
        $$->push_back($1);
    }
  | identifiers ',' ID {
        $1->push_back($3);
        $$ = $1;
    }
  ;

identifiers_and_fields:
    ID {
        $$ = new TVector<TString>();
        $$->push_back($1);
    }
  | ID '.' ID {
        $$ = new TVector<TString>();
        $$->push_back(Sprintf("%s.%s", $1, $3));
    }
  | identifiers_and_fields ',' ID {
        $1->push_back($3);
        $$ = $1;
    }
  | identifiers_and_fields ',' ID '.' ID {
        $1->push_back(Sprintf("%s.%s", $3, $5));
        $$ = $1;
    }
  ;

field_type:
    NUM {
        $$ = FromString<int>($1);
    }
  | T_FLOAT {
        $$ = -1;
    }
  ;

%%

TState* ParseFile(const TString& filename) {
    State = new TState();
    State->InputFile = filename;
    ScanFile(filename);
    yyparse(NULL);
    return State;
}
