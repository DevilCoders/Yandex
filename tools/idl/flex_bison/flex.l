%{

#undef YY_DECL
#define YY_DECL int BisonedFlexLexer::yylex(YYSTYPE* yylval, YYLTYPE* yylloc)

#include <tools/idl/flex_bison/flex.h>

#include <boost/algorithm/string.hpp>

#include <string>

#define LOCATION_NEWLINES(number) yylloc->last_line += number;
#define LOCATION_NEXT yylloc->first_line = yylloc->last_line;

#define PUSH_NEXT_CHAR yylval->stringPointer->push_back(yytext[0]);

using std::cin;
using std::cout;
using std::cerr;

/**
 * Quoted characters array is created with quotation marks at both ends. This
 * function removes them, and returns the string as std::string object.
 */
std::string* quotedCharsToHeapString(const char* chars)
{
    // trim first quotation mark
    std::string* heapString = new std::string(chars + 1);

    // trim last quotation mark
    return &heapString->erase(heapString->size() - 1);
}

/**
 * Default value is everything after '=', and before the first symbol of the
 * next token. So it usually has whitespace symbols at both ends, which need
 * to be trimmed.
 */
void trimDefaultValue(std::string* value)
{
    boost::algorithm::trim(*value);
}
%}

%option warn nodefault
%option c++ noyywrap never-interactive
%option yyclass="BisonedFlexLexer"

%x DEFAULT_VALUE_STATE
%x SINGLE_LINE_DOC MULTI_LINE_DOC
%s DOC_LINK

PATH_TO_IDL     \"[a-zA-Z0-9_/.-]+.idl\"
PATH_TO_PROTO   \"[a-zA-Z0-9_/.-]+.proto\"
PATH_TO_HEADER  \"[a-zA-Z0-9_/.-]+.h\"
IDENTIFIER      [a-zA-Z_][a-zA-Z0-9_]*

%%
    static int numberOfRemainingClosingParensInsideDefaultValue = 0;
    static bool isInMultiLineDoc = false, justFoundParamKeyword = false;

"cpp"       { return CPP;       }
"cs"        { return CS;        }
"java"      { return JAVA;      }
"objc"      { return OBJC;      }
"protoconv" { return PROTOCONV; }

"objc_infix" { return OBJC_INFIX; }
"import"     { return IMPORT;     }

"const"    { return CONST;      }
"optional" { return OPTIONAL;   }

"bg_thread"  { return BG_THREAD;  }
"any_thread" { return ANY_THREAD; }

"vector"     { return VECTOR;     }
"dictionary" { return DICTIONARY; }

"based on" { return BASED_ON; }

"enum"     { return ENUM;     }
"bitfield" { return BITFIELD; }

"variant" { return VARIANT; }

"struct"  { return STRUCT;  }
"lite"    { return LITE;    }
"options" { return OPTIONS; }

"interface"     { return INTERFACE;     }
"virtual"       { return VIRTUAL;       }
"view_delegate" { return VIEW_DELEGATE; }
"weak_ref"      { return WEAK_REF;      }
"shared_ref"    { return SHARED_REF;    }
"gen"           { return GEN;           }
"readonly"      { return READONLY;      }
"native"        { return NATIVE;        }
"static"        { return STATIC;        }

"listener"   { return LISTENER;   }
"lambda"     { return LAMBDA;     }
"strong_ref" { return STRONG_REF; }
"platform"   { return PLATFORM;   }

{PATH_TO_IDL}    { yylval->stringPointer = quotedCharsToHeapString(yytext);
                   return PATH_TO_IDL; }
{PATH_TO_PROTO}  { yylval->stringPointer = quotedCharsToHeapString(yytext);
                   return PATH_TO_PROTO; }
{PATH_TO_HEADER} { yylval->stringPointer = quotedCharsToHeapString(yytext);
                   return PATH_TO_HEADER; }
{IDENTIFIER}     { yylval->stringPointer = new std::string(yytext);
                   return IDENTIFIER; }

<INITIAL>"=" { BEGIN(DEFAULT_VALUE_STATE);
               yylval->stringPointer = new std::string("");
               numberOfRemainingClosingParensInsideDefaultValue = 0; }
<DEFAULT_VALUE_STATE>{
    <<EOF>>   { yyterminate(); }
    [;,}]     { BEGIN(INITIAL);
                unput(yytext[0]);
                trimDefaultValue(yylval->stringPointer);
                return DEFAULT_VALUE; }
    "("       { PUSH_NEXT_CHAR;
                ++numberOfRemainingClosingParensInsideDefaultValue; }
    ")"       {
                if (numberOfRemainingClosingParensInsideDefaultValue > 0) {
                    --numberOfRemainingClosingParensInsideDefaultValue;
                    PUSH_NEXT_CHAR;
                } else {
                    BEGIN(INITIAL);
                    unput(yytext[0]);
                    trimDefaultValue(yylval->stringPointer);
                    return DEFAULT_VALUE;
                }
              }
    \'[^']*\' { yylval->stringPointer->append(yytext); }
    \"[^"]*\" { yylval->stringPointer->append(yytext); }
    \n        { LOCATION_NEWLINES(1); LOCATION_NEXT;
                PUSH_NEXT_CHAR; }
    .         { LOCATION_NEXT;
                PUSH_NEXT_CHAR; }
}

<INITIAL>"\/\/" { BEGIN(SINGLE_LINE_DOC);
                  yylval->stringPointer = new std::string(yytext);
                  isInMultiLineDoc = false;
                  justFoundParamKeyword = false;
                  return DOC_TEXT; }
<INITIAL>"/*" { BEGIN(MULTI_LINE_DOC);
                yylval->stringPointer = new std::string(yytext);
                isInMultiLineDoc = true;
                justFoundParamKeyword = false;
                return DOC_TEXT; }
<MULTI_LINE_DOC>{
    "*/" { BEGIN(INITIAL);
           LOCATION_NEXT;
           yylval->stringPointer = new std::string(yytext);
           return DOC_TEXT; }
}
<SINGLE_LINE_DOC,MULTI_LINE_DOC>{
    <<EOF>>        { yyterminate(); }
    \n             {
                     if (!isInMultiLineDoc) {
                         BEGIN(INITIAL);
                     }
                     LOCATION_NEWLINES(1); LOCATION_NEXT;
                     yylval->stringPointer = new std::string("\n");
                     return DOC_TEXT;
                   }
    "@commercial"  { return DOC_COMMERCIAL; }
    "@internal"    { return DOC_INTERNAL; }
    "@undocumented" { return DOC_UNDOCUMENTED; }
    "{@link"       { BEGIN(DOC_LINK);
                     return DOC_LINK_BEGIN; }
    "@param"[\t ]+ { justFoundParamKeyword = true;
                     return DOC_PARAM; }
    "@return"      { return DOC_RETURN; }
    {IDENTIFIER}   {
                     yylval->stringPointer = new std::string(yytext);
                     if (justFoundParamKeyword) {
                         justFoundParamKeyword = false;
                         return IDENTIFIER;
                     } else {
                         return DOC_TEXT;
                     }
                   }
    [^@{*\t\n ]+   { yylval->stringPointer = new std::string(yytext);
                     return DOC_TEXT; }
    .              { yylval->stringPointer = new std::string(yytext);
                     return DOC_TEXT; }
}
<DOC_LINK>{
    "}" {
          if (isInMultiLineDoc) {
              BEGIN(MULTI_LINE_DOC);
          } else {
              BEGIN(SINGLE_LINE_DOC);
          }
          return '}';
        }
}

[#(),./:;<>{}] { return yytext[0]; }

\n+      { LOCATION_NEWLINES(yyleng); LOCATION_NEXT; }
[ \t\r]+ { LOCATION_NEXT; }
.        { LOCATION_NEXT; }
%%
