%code top {

#include <tools/idl/flex_bison/flex.h>

#include <memory>
#include <string>
#include <vector>

using namespace yandex::maps::idl;
using namespace yandex::maps::idl::nodes;
using namespace yandex::maps::idl::parser;

void yyerror(
    YYLTYPE* locp,
    void*,
    std::unique_ptr<Root>&,
    Errors& errors,
    const char* message)
{
    errors.setLineNumberAndMessage(locp->first_line, message);
}

inline int yylex(YYSTYPE* lval, YYLTYPE* llocp, BisonedFlexLexer* lexer)
{
    return lexer->yylex(lval, llocp);
}

#define ERROR_CONTEXT(context) { errors.addWithContext(context); yyerrok; }

} // %code top

%code requires {
#include <tools/idl/flex_bison/internal/bison_helpers.h>

#include <yandex/maps/idl/nodes/custom_code_link.h>
#include <yandex/maps/idl/nodes/doc.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/function.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/name.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/nodes/property.h>
#include <yandex/maps/idl/nodes/protobuf.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/scope.h>

#include <util/system/win_undef.h>
#undef CONST

using TargetSpecificName = yandex::maps::idl::TargetSpecificValue<std::string>;
using TargetSpecificNames = std::vector<TargetSpecificName>;

using TargetSpecificFunctionName =
    yandex::maps::idl::TargetSpecificValue<yandex::maps::idl::Scope>;
using TargetSpecificFunctionNames = std::vector<TargetSpecificFunctionName>;

class BisonedFlexLexer;

} // %code requires

%locations
%define parse.error verbose
%define api.pure true
%lex-param { BisonedFlexLexer* lexer }
%parse-param { BisonedFlexLexer* lexer }
%parse-param { std::unique_ptr<yandex::maps::idl::nodes::Root>& root }
%parse-param { yandex::maps::idl::parser::Errors& errors }

%union {
    std::string* stringPointer;
    bool booleanValue;
    std::vector<std::string>* strings;
    yandex::maps::idl::Scope* scope;

    yandex::maps::idl::nodes::Root* root;

    yandex::maps::idl::nodes::Name* name;
    std::vector<TargetSpecificName>* targetSpecificNames;
    TargetSpecificName* targetSpecificName;
    std::vector<TargetSpecificFunctionName>* targetSpecificFunctionNames;
    TargetSpecificFunctionName* targetSpecificFunctionName;

    yandex::maps::idl::nodes::Nodes* nodes;

    yandex::maps::idl::nodes::TypeRef* typeRef;

    yandex::maps::idl::nodes::Doc* doc;
    yandex::maps::idl::nodes::DocBlock* docBlock;
    yandex::maps::idl::nodes::DocLink* docLink;
    std::vector<yandex::maps::idl::nodes::TypeRef>* docLinkParamTypeRefs;

    yandex::maps::idl::nodes::Function::ThreadRestriction threadRestriction;
    yandex::maps::idl::nodes::Function* function;
    std::vector<yandex::maps::idl::nodes::FunctionParameter>* functionParams;
    yandex::maps::idl::nodes::FunctionParameter* functionParam;
    yandex::maps::idl::nodes::FunctionName* functionName;

    yandex::maps::idl::nodes::Property* property;

    yandex::maps::idl::nodes::CustomCodeLink* customCodeLink;

    yandex::maps::idl::nodes::ProtoMessage* protoMessage;

    yandex::maps::idl::nodes::Enum* enumNode;
    std::vector<yandex::maps::idl::nodes::EnumField>* enumFields;
    yandex::maps::idl::nodes::EnumField* enumField;

    yandex::maps::idl::nodes::Struct* structNode;
    yandex::maps::idl::nodes::StructField* structField;
    yandex::maps::idl::nodes::StructKind structKind;

    yandex::maps::idl::nodes::Variant* variant;
    std::vector<yandex::maps::idl::nodes::VariantField>* variantFields;
    yandex::maps::idl::nodes::VariantField* variantField;

    yandex::maps::idl::nodes::Interface* interface;
    yandex::maps::idl::nodes::Interface::Ownership interfaceOwnership;

    yandex::maps::idl::nodes::Listener* listener;
    std::vector<yandex::maps::idl::nodes::Function>* listenerFunctions;
} // union

%token CPP CS JAVA OBJC PROTOCONV
%token OBJC_INFIX IMPORT
%token CONST OPTIONAL
%token BG_THREAD ANY_THREAD
%token VECTOR DICTIONARY
%token BASED_ON
%token ENUM BITFIELD
%token VARIANT
%token STRUCT LITE OPTIONS
%token INTERFACE VIRTUAL VIEW_DELEGATE WEAK_REF SHARED_REF GEN READONLY NATIVE STATIC
%token LISTENER LAMBDA STRONG_REF PLATFORM
%token <stringPointer> PATH_TO_IDL PATH_TO_PROTO PATH_TO_HEADER
%token <stringPointer> IDENTIFIER
%token <stringPointer> DEFAULT_VALUE
%token <stringPointer> DOC_TEXT
%token DOC_LINK_BEGIN DOC_PARAM DOC_RETURN DOC_COMMERCIAL DOC_INTERNAL DOC_UNDOCUMENTED
%token END 0

%type <root> root
%type <scope> scope
%type <stringPointer> target
%type <stringPointer> objc_infix
%type <strings> imports
%type <stringPointer> import
%type <name> name
%type <targetSpecificNames> target_specific_names
%type <targetSpecificName> target_specific_name
%type <targetSpecificFunctionNames> target_specific_function_names
%type <targetSpecificFunctionName> target_specific_function_name
%type <typeRef> type_ref
%type <typeRef> type_ref_base
%type <booleanValue> constant
%type <booleanValue> optional
%type <threadRestriction> thread_restriction
%type <doc> doc
%type <docBlock> doc_block
%type <docLink> doc_link
%type <docLink> doc_link_scope_part
%type <docLinkParamTypeRefs> doc_link_param_type_refs
%type <function> function
%type <functionParams> function_params
%type <functionName> function_name
%type <functionParam> function_param
%type <property> property
%type <property> base_property
%type <booleanValue> readonly
%type <customCodeLink> custom_code_link
%type <protoMessage> proto_message
%type <enumNode> enum
%type <enumNode> enum_nc
%type <booleanValue> bitfield
%type <enumFields> enum_fields
%type <enumField> enum_field
%type <enumField> enum_field_nc
%type <structNode> struct
%type <structNode> struct_nc
%type <structKind> struct_kind
%type <nodes> in_struct_decls;
%type <structField> struct_field
%type <structField> struct_field_body
%type <stringPointer> struct_field_based_on
%type <variant> variant
%type <variant> variant_nc
%type <variantFields> variant_fields
%type <variantField> variant_field
%type <interface> interface
%type <interface> interface_nc
%type <booleanValue> virtual
%type <booleanValue> view_delegate
%type <interfaceOwnership> interface_ownership
%type <scope> base_interface
%type <interface> in_interface_decls;
%type <listener> listener
%type <listener> listener_nc
%type <booleanValue> strong_ref
%type <scope> base_listener
%type <listenerFunctions> listener_functions

%%
start: root doc END { root.reset($1); }
     | root error END { root.release();
                        ERROR_CONTEXT(""); }
     | error END { root.release();
                   ERROR_CONTEXT(""); }
     ;
root: objc_infix { $$ = new Root();
                   move($$->objcInfix, $1); }
    | objc_infix imports { $$ = new Root();
                           move($$->objcInfix, $1);
                           move($$->imports, $2); }
    | imports { $$ = new Root();
                move($$->imports, $1); }
    | enum { $$ = new Root();
             pushNode($$->nodes, $1); }
    | interface { $$ = new Root();
                  pushNode($$->nodes, $1); }
    | listener { $$ = new Root();
                 pushNode($$->nodes, $1); }
    | struct { $$ = new Root();
               pushNode($$->nodes, $1); }
    | variant { $$ = new Root();
                pushNode($$->nodes, $1); }
    | root enum { $$ = $1;
                  pushNode($$->nodes, $2); }
    | root interface { $$ = $1;
                       pushNode($$->nodes, $2); }
    | root listener { $$ = $1;
                      pushNode($$->nodes, $2); }
    | root struct { $$ = $1;
                    pushNode($$->nodes, $2); }
    | root variant { $$ = $1;
                     pushNode($$->nodes, $2); }
    | root error '}' { $$ = $1;
                       ERROR_CONTEXT(""); }
    ;

scope: IDENTIFIER { $$ = new Scope();
                    pushMoved(*$$, $1); }
     | '.' IDENTIFIER { $$ = new Scope("");
                        pushMoved(*$$, $2); }
     | scope '.' IDENTIFIER { $$ = $1;
                              pushMoved(*$$, $3); }
     ;

target: CS { $$ = new std::string("cs"); }
      | JAVA { $$ = new std::string("java"); }
      | OBJC { $$ = new std::string("objc"); }
      ;

objc_infix: OBJC_INFIX IDENTIFIER ';' { $$ = new std::string();
                                        move(*$$, $2); }
          ;

imports: import { $$ = new std::vector<std::string>();
                  pushMoved(*$$, $1); }
       | imports import { $$ = $1;
                          pushMoved(*$$, $2); }
       ;
import: IMPORT PATH_TO_IDL ';' { $$ = $2; }
      ;

name: IDENTIFIER { $$ = createCustomizableValue($1); }
    | IDENTIFIER '(' target_specific_names ')'
                { $$ = createCustomizableValue($1, $3); }
    ;
target_specific_names: target_specific_name
                                 { $$ = new std::vector<TargetSpecificName>();
                                   pushMoved(*$$, $1); }
                     | target_specific_names ',' target_specific_name
                                 { $$ = $1;
                                   pushMoved(*$$, $3); }
                     ;
target_specific_name: target ':' IDENTIFIER { $$ = new TargetSpecificName();
                                              move($$->target, $1);
                                              move($$->value, $3); }
                    ;

type_ref: type_ref_base VECTOR '<' type_ref '>'
                      { $$ = $1;
                        $$->id = TypeId::Vector;
                        pushMoved($$->parameters, $4); }
        | type_ref_base DICTIONARY '<' type_ref ',' type_ref '>'
                      { $$ = $1;
                        $$->id = TypeId::Dictionary;
                        pushMoved($$->parameters, $4);
                        pushMoved($$->parameters, $6); }
        | type_ref_base scope
                      { $$ = $1;
                        initializeTypeRefFromName(*$$, $2); }
        ;
type_ref_base: constant optional { $$ = new TypeRef();
                                   $$->isConst = $1;
                                   $$->isOptional = $2; }
             ;
constant: %empty { $$ = false; }
        | CONST { $$ = true; }
        ;
optional: %empty { $$ = false; }
        | OPTIONAL { $$ = true; }
        ;

thread_restriction: %empty { $$ = Function::ThreadRestriction::Ui; }
                  | BG_THREAD { $$ = Function::ThreadRestriction::Bg; }
                  | ANY_THREAD { $$ = Function::ThreadRestriction::None; }
                  ;

doc: %empty { $$ = nullptr; }
   | doc_block { $$ = new Doc();
                 move($$->description, $1); }
   | doc DOC_PARAM IDENTIFIER doc_block { $$ = $1;
                                          pushPair($$->parameters, $3, $4); }
   | doc DOC_RETURN doc_block { $$ = $1;
                                move($$->result, $3); }
   | doc DOC_COMMERCIAL doc_block { $$ = $1;
                                    $$->status = Doc::Status::Commercial;
                                    appendMoved($$->description, $3); }
   | doc DOC_INTERNAL doc_block { $$ = $1;
                                  $$->status = Doc::Status::Internal;
                                  appendMoved($$->description, $3); }
   | doc DOC_UNDOCUMENTED doc_block { $$ = $1;
                                  $$->status = Doc::Status::Undocumented;
                                  appendMoved($$->description, $3); }
   ;
doc_block: DOC_TEXT { $$ = new DocBlock();
                      move($$->format, $1); }
         | doc_block DOC_TEXT { $$ = $1;
                                appendMoved($$->format, $2); }
         | doc_block doc_link { $$ = $1;
                                addDocLink(*$$, $2); }
         ;
doc_link: doc_link_scope_part '}' { $$ = $1; }
        | doc_link_scope_part '#' IDENTIFIER '}' { $$ = $1;
                                                   move($$->memberName, $3); }
        | doc_link_scope_part '#' IDENTIFIER '(' doc_link_param_type_refs ')'
          '}'
                    { $$ = $1;
                      move($$->memberName, $3);
                      move($$->parameterTypeRefs, $5); }
        ;
doc_link_scope_part: DOC_LINK_BEGIN { $$ = new DocLink(); }
                   | DOC_LINK_BEGIN scope { $$ = new DocLink();
                                            move($$->scope, $2); }
                   ;
doc_link_param_type_refs: %empty { $$ = new std::vector<TypeRef>(); }
                        | type_ref { $$ = new std::vector<TypeRef>();
                                     pushMoved(*$$, $1); }
                        | doc_link_param_type_refs ',' type_ref
                                    { $$ = $1;
                                      pushMoved(*$$, $3); }
                        ;

function: doc type_ref function_name '(' function_params ')' constant thread_restriction ';'
                    { $$ = new Function();
                      move($$->doc, $1);
                      move($$->result.typeRef, $2);
                      move($$->name, $3);
                      move($$->parameters, $5);
                      $$->isConst = $7;
                      $$->threadRestriction = $8; }
        ;

function_name: IDENTIFIER { $$ = createCustomizableValue(new Scope(*$1)); }
             | IDENTIFIER '<' target_specific_function_names '>'
                         { $$ = createCustomizableValue(new Scope(*$1), $3); }
             ;
target_specific_function_names: target_specific_function_name
                                          { $$ = new std::vector<TargetSpecificFunctionName>();
                                            pushMoved(*$$, $1); }
                              | target_specific_function_names ',' target_specific_function_name
                                          { $$ = $1;
                                            pushMoved(*$$, $3); }
                              ;
target_specific_function_name: target ':' scope { $$ = new TargetSpecificFunctionName();
                                                  move($$->target, $1);
                                                  move($$->value, $3); }
                             ;
function_params: %empty { $$ = nullptr; }
               | function_param { $$ = new std::vector<FunctionParameter>();
                                  pushMoved(*$$, $1); }
               | function_params ',' function_param { $$ = $1;
                                                      pushMoved(*$$, $3); }
               ;
function_param: type_ref IDENTIFIER { $$ = new FunctionParameter();
                                      move($$->typeRef, $1);
                                      move($$->name, $2); }
              | type_ref IDENTIFIER DEFAULT_VALUE
                          { $$ = new FunctionParameter();
                            move($$->typeRef, $1);
                            move($$->name, $2);
                            move($$->defaultValue, $3); }
              | type_ref LISTENER { yyerror(&@$, nullptr, root, errors, "Please do not use \'listener\' as argument name - it is a keyword");
                                    YYERROR; }
              ;

property: doc base_property { $$ = $2;
                              move($$->doc, $1);
                              $$->isGenerated = false; }
        | doc GEN base_property { $$ = $3;
                                  move($$->doc, $1);
                                  $$->isGenerated = true; }
        ;
base_property: type_ref IDENTIFIER readonly ';' { $$ = new Property();
                                                  move($$->typeRef, $1);
                                                  move($$->name, $2);
                                                  $$->isReadonly = $3; }
             ;
readonly: %empty { $$ = false; }
        | READONLY { $$ = true; }
        ;

custom_code_link: %empty { $$ = new CustomCodeLink(); }
                | custom_code_link CPP PATH_TO_HEADER
                            { $$ = $1;
                              move($$->baseHeader, $3); }
                | custom_code_link PROTOCONV PATH_TO_HEADER
                            { $$ = $1;
                              move($$->protoconvHeader, $3); }
                ;

proto_message: %empty { $$ = nullptr; }
             | BASED_ON PATH_TO_PROTO ':' scope { $$ = new ProtoMessage();
                                                  move($$->pathToProto, $2);
                                                  move($$->pathInProto, $4); }
             ;

enum: doc enum_nc { $$ = $2;
                    move($$->doc, $1); }
    ;
enum_nc: custom_code_link bitfield ENUM name proto_message '{' enum_fields '}'
                   { $$ = new Enum();
                     move($$->customCodeLink, $1);
                     $$->isBitField = $2;
                     move($$->name, $4);
                     move($$->protoMessage, $5);
                     move($$->fields, $7); }
       | custom_code_link bitfield ENUM error '}'
                   { $$ = new Enum();
                     ERROR_CONTEXT("enum"); }
       | custom_code_link bitfield ENUM error END
                   { $$ = new Enum();
                     ERROR_CONTEXT("enum"); }
       ;
bitfield : %empty { $$ = false; }
         | BITFIELD { $$ = true; }
         ;
enum_fields: enum_field { $$ = new std::vector<EnumField>();
                          pushMoved(*$$, $1); }
           | enum_fields ',' enum_field { $$ = $1;
                                          pushMoved(*$$, $3); }
           ;
enum_field: doc enum_field_nc { $$ = $2;
                                move($$->doc, $1); }
          ;
enum_field_nc: IDENTIFIER { $$ = new EnumField();
                            move($$->name, $1); }
             | IDENTIFIER DEFAULT_VALUE { $$ = new EnumField();
                                          move($$->name, $1);
                                          move($$->value, $2); }
             ;

struct: doc struct_nc { $$ = $2;
                        move($$->doc, $1); }
      ;
struct_nc: custom_code_link struct_kind STRUCT name proto_message
           '{' in_struct_decls '}' { $$ = new Struct();
                                     move($$->customCodeLink, $1);
                                     $$->kind = $2;
                                     move($$->name, $4);
                                     move($$->protoMessage, $5);
                                     move($$->nodes, $7); }
         | custom_code_link struct_kind STRUCT error in_struct_decls '}'
                     { $$ = new Struct();
                       ERROR_CONTEXT("struct"); }
         | custom_code_link struct_kind STRUCT error in_struct_decls END
                     { $$ = new Struct();
                       ERROR_CONTEXT("struct"); }
         ;
struct_kind: %empty { $$ = StructKind::Bridged; }
           | LITE { $$ = StructKind::Lite; }
           | OPTIONS { $$ = StructKind::Options; }
           ;
in_struct_decls: %empty { $$ = new Nodes(); }
               | in_struct_decls enum { $$ = $1;
                                        pushNode(*$$, $2); }
               | in_struct_decls struct { $$ = $1;
                                          pushNode(*$$, $2); }
               | in_struct_decls struct_field { $$ = $1;
                                                pushNode(*$$, $2); }
               | in_struct_decls variant { $$ = $1;
                                           pushNode(*$$, $2); }
               ;
struct_field: doc struct_field_body ';' { $$ = $2;
                                          move($$->doc, $1); }
            ;
struct_field_body: type_ref IDENTIFIER struct_field_based_on
                             { $$ = new StructField();
                               move($$->typeRef, $1);
                               move($$->name, $2);
                               move($$->protoField, $3); }
                 | type_ref IDENTIFIER struct_field_based_on DEFAULT_VALUE
                             { $$ = new StructField();
                               move($$->typeRef, $1);
                               move($$->name, $2);
                               move($$->defaultValue, $4);
                               move($$->protoField, $3); }
                 ;
struct_field_based_on: %empty { $$ = nullptr; }
                     | BASED_ON IDENTIFIER { $$ = $2; }
                     ;

variant: doc variant_nc { $$ = $2;
                          move($$->doc, $1); }
       ;
variant_nc: custom_code_link VARIANT name '{' variant_fields '}'
                      { $$ = new Variant();
                        move($$->customCodeLink, $1);
                        move($$->name, $3);
                        move($$->fields, $5); }
          | custom_code_link VARIANT error '}' { $$ = new Variant();
                                                 ERROR_CONTEXT("variant"); }
          | custom_code_link VARIANT error END { $$ = new Variant();
                                                 ERROR_CONTEXT("variant"); }
          ;
variant_fields: variant_field { $$ = new std::vector<VariantField>();
                                pushMoved(*$$, $1); }
              | variant_fields variant_field { $$ = $1;
                                               pushMoved(*$$, $2); }
              ;
variant_field: type_ref IDENTIFIER ';'
                         { $$ = new VariantField();
                           move($$->typeRef, $1);
                           move($$->name, $2); }
             ;

interface: doc interface_nc { $$ = $2;
                              move($$->doc, $1); }
         ;
interface_nc: virtual view_delegate interface_ownership INTERFACE name
              base_interface '{' in_interface_decls '}'
                        { $$ = $8;
                          $$->isVirtual = $1;
                          $$->isViewDelegate = $2;
                          $$->isStatic = false;
                          $$->ownership = $2 ? Interface::Ownership::Weak : $3;
                          move($$->name, $5);
                          move($$->base, $6); }
            | STATIC INTERFACE name '{' in_interface_decls '}'
                        { $$ = $5;
                          $$->isVirtual = false;
                          $$->isViewDelegate = false;
                          $$->isStatic = true;
                          $$->ownership = Interface::Ownership::Strong;
                          move($$->name, $3); }
            | NATIVE LISTENER name '{' in_interface_decls '}'
                        { $$ = $5;
                          $$->isVirtual = false;
                          $$->isViewDelegate = false;
                          $$->isStatic = false;
                          $$->ownership = Interface::Ownership::Strong;
                          move($$->name, $3); }
            | virtual view_delegate interface_ownership INTERFACE error
              in_interface_decls '}'
                        { $$ = new Interface();
                          ERROR_CONTEXT("interface"); }
            | virtual view_delegate interface_ownership INTERFACE error
              in_interface_decls END
                        { $$ = new Interface();
                          ERROR_CONTEXT("interface"); }
            | STATIC INTERFACE error in_interface_decls '}'
                        { $$ = new Interface();
                          ERROR_CONTEXT("static interface"); }
            | STATIC INTERFACE error END { $$ = new Interface();
                                           ERROR_CONTEXT("static interface"); }
            | NATIVE LISTENER error in_interface_decls '}'
                        { $$ = new Interface();
                          ERROR_CONTEXT("native listener"); }
            | NATIVE LISTENER error END { $$ = new Interface();
                                          ERROR_CONTEXT("native listener"); }
            ;
virtual: %empty { $$ = false; }
       | VIRTUAL { $$ = true; }
       ;
view_delegate: %empty { $$ = false; }
             | VIEW_DELEGATE { $$ = true; }
             ;
interface_ownership: %empty { $$ = Interface::Ownership::Strong; }
                   | SHARED_REF { $$ = Interface::Ownership::Shared; }
                   | WEAK_REF { $$ = Interface::Ownership::Weak; }
                   ;
base_interface: %empty { $$ = nullptr; }
              | ':' scope { $$ = $2; }
              ;
in_interface_decls: %empty { $$ = new Interface(); }
                  | in_interface_decls enum { $$ = $1;
                                              pushNode($$->nodes, $2); }
                  | in_interface_decls interface { $$ = $1;
                                                   pushNode($$->nodes, $2); }
                  | in_interface_decls function { $$ = $1;
                                                  pushNode($$->nodes, $2); }
                  | in_interface_decls listener { $$ = $1;
                                                  pushNode($$->nodes, $2); }
                  | in_interface_decls property { $$ = $1;
                                                  pushNode($$->nodes, $2); }
                  | in_interface_decls struct { $$ = $1;
                                                pushNode($$->nodes, $2); }
                  | in_interface_decls variant { $$ = $1;
                                                 pushNode($$->nodes, $2); }
                  ;

listener: doc listener_nc { $$ = $2;
                            move($$->doc, $1); }
        | doc LAMBDA listener_nc { $$ = $3;
                                   $$->isLambda = true;
                                   $$->isStrongRef = false;
                                   move($$->doc, $1); }
        ;
listener_nc: LISTENER name base_listener '{' listener_functions '}'
                       { $$ = new Listener();
                         $$->isStrongRef = false;
                         move($$->name, $2);
                         move($$->base, $3);
                         move($$->functions, $5); }
           | strong_ref PLATFORM INTERFACE name base_listener '{'
             listener_functions '}'
                       { $$ = new Listener();
                         $$->isStrongRef = $1;
                         move($$->name, $4);
                         move($$->base, $5);
                         move($$->functions, $7); }
           | LISTENER error '}' { $$ = new Listener();
                                  ERROR_CONTEXT("listener"); }
           | LISTENER error END { $$ = new Listener();
                                  ERROR_CONTEXT("listener"); }
           | strong_ref PLATFORM INTERFACE error '}'
                       { $$ = new Listener();
                         ERROR_CONTEXT("platform interface"); }
           | strong_ref PLATFORM INTERFACE error END
                       { $$ = new Listener();
                         ERROR_CONTEXT("platform interface"); }
           ;
strong_ref: %empty { $$ = false; }
          | STRONG_REF { $$ = true; }
          ;
base_listener: %empty { $$ = nullptr; }
             | ':' scope { $$ = $2; }
             ;
listener_functions: function { $$ = new std::vector<Function>();
                               pushMoved(*$$, $1); }
                  | listener_functions function { $$ = $1;
                                                  pushMoved(*$$, $2); }
                  ;
%%
