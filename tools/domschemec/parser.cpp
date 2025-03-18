#include "parser.h"
#include "builtins.h"
#include "ops.h"
#include "svn_keywords.h"

#include <util/string/cast.h>
#include <util/generic/algorithm.h>

namespace NDomSchemeCompiler {

TParser::TParser(const TString& file)
    : TLexer(file)
    , CurrentScope(nullptr)
{
}

TParser::TParser(THolder<ILexerContext>&& ctx)
    : TLexer(std::move(ctx))
    , CurrentScope(nullptr)
{
}

const TProgram& TParser::GetProgram() const {
    return Program;
}

struct TParser::TScope {
    struct TTypeLookupResult {
        bool IsKnown;
        bool IsTopLevel;
    };

    TScope* Parent;
    TParser& Parser;
    TVector<TNamedType>& LocalTypes;

    TScope(TParser& parser, TVector<TNamedType>& localTypes)
        : Parser(parser)
        , LocalTypes(localTypes)
    {
        Parent = parser.CurrentScope;
        parser.CurrentScope = this;
    }

    TTypeLookupResult LookupType(const TString& name) {
        for (const auto& t : LocalTypes) {
            if (t.Name == name) {
                return {true, t.Type.IsTopLevel};
            }
        }
        if (Parent) {
            return Parent->LookupType(name);
        }
        if (Builtins().SchemeNameToType.contains(name)) {
            return {true, true};
        }
        return {false, false};
    }

    ~TScope() {
        Parser.CurrentScope = Parent;
    }
};

void TParser::Run() {
    while (Peek()) {
        TNamespace ns;
        ns.Name = ParseNamespaceName();

        TScope scope(*this, ns.Types);
        for (;;) {
            if (!TryParseNamedType(true, nullptr)) {
                if(TryParsePragmas()) {
                    continue;
                }
                if(!TryParseInclude()) {
                    if (Peek()) {
                        break;
                    }
                    if (!ContextPop()) {
                        Program.Namespaces.push_back(ns);
                        return;
                    }
                }
            }
        }

        Program.Namespaces.push_back(ns);
    }
}

TString RemoveQuotes(const TString& str) {
    if (!str.StartsWith('"') && !str.EndsWith('"')) {
        ythrow yexception() << "Expected string in double quotes, but got: '" << str << "'";
    }
    return str.substr(1, str.size() - 2);
}

bool TParser::TryParseInclude() {
    if (TryNext("include")) {
        TString file = ToString(NextString());
        Next(";");
        try {
            ContextPush(RemoveQuotes(file));
        } catch (const std::exception& e) {
            ThrowSyntaxError("Cannot include file: " + file + ", reason: " + e.what());
        }
        return true;
    }
    return false;
}

bool TParser::TryParsePragmas() {
    bool on = false;
    if ((on = TryNext("pragma")) || TryNext("pragma_off")) {
        do {
            auto pragma =  NextSymbol();
            if (!UpdatePragmas(SetPragmas(), pragma, on)) {
                ThrowSyntaxError("Unknown pragma: '" + ToString(pragma) + "'");
            }
            if (TryNext(";")) {
                break;
            }
            Next(",");
        } while (true);
        return true;
    }
    return false;
}

TVector<TString> TParser::ParseNamespaceName() {
    TVector<TString> name;

    Next("namespace");
    do {
        name.push_back(ToString(NextSymbol()));
        if (TryNext(";")) {
            break;
        }
        Next("::");
    } while (true);

    return name;
}

inline bool IsCamelCase(TStringBuf id) {
    while (id) {
        if (!isupper(id[0])) {
            return false;
        }
        id.Skip(1);
        while (id && (islower(id[0]) || isdigit(id[0]))) {
            id.Skip(1);
        }
    }
    return true;
}

bool TParser::TryParseNamedType(bool toplevel, TFieldAttrs* fieldAttrs) {
    if (TryNext("struct")) {
        TNamedType type(ToString(NextSymbol()), toplevel);
        // Add a type to scope as soon as we know its name
        // to support recursive types Foo {foos : [Foo]}
        auto currentStructIdx = CurrentScope->LocalTypes.ysize();
        CurrentScope->LocalTypes.push_back(type);
        type.Type.Struct.Reset(new TStruct);
        if (Peek() == "(") {
            ParseAttrs(type.Type.Struct.Get(), fieldAttrs, nullptr);
        }
        if (TryNext(":")) {
            ParseParentTypes(*type.Type.Struct.Get());
        }
        ParseStructBody(*type.Type.Struct);
        Next(";");
        CurrentScope->LocalTypes[currentStructIdx] = type;
        return true;
    } else if (TryNext("using")) {
        TNamedType type(ToString(NextSymbol()), toplevel);
        Next("=");
        ParseType(type.Type, false);
        Next(";");
        CurrentScope->LocalTypes.push_back(type);
        return true;
    }
    return false;
}

static TString ToCamelCase(TStringBuf something) {
    TString res;
    bool toTitle = true;
    for (char c : TStringBuf(something)) {
        if (isalpha(c)) {
            if (toTitle) {
                c = toupper(c);
            }
            toTitle = false;
        }
        if (isalnum(c)) {
            res += c;
        } else {
            toTitle = true;
        }
    }
    return res;
}

void TParser::ParseStructBody(TStruct& structDef) {
    Next("{");

    TScope scope(*this, structDef.Types);

    while (!TryNext("}")) {
        if (TryParseNamedType(false, nullptr)) {
            // do nothing: parsed a type
        } else if (TryNext("(") && Next("validate")) {
            TString fieldName;
            TPosition fieldNamePos = GetPosition();
            if (IsSymbol(Peek())) {
                fieldName = Next();
            } else if (IsString(Peek())) {
                TStringBuf quoted = Next();
                fieldName = quoted.SubStr(1, quoted.size() - 2);
            }
            Next(")");
            TString code(SlurpCppCode());
            Next(";");
            TValidationOperation validationOp { "code", {code} };
            if (!fieldName) {
                structDef.ValidationOperators.push_back(validationOp);
            } else {
                TMemberDefinition* member = FindIfPtr(
                    structDef.Members,
                    [&](const TMemberDefinition& m) { return m.Name == fieldName; });
                if (!member) {
                    ThrowSomeError("Uknown field name: " + fieldName, fieldNamePos);
                }
                member->ValidationOperators.push_back(validationOp);
            }
        } else {
            TMemberDefinition member;

            TPosition definitionStart = GetPosition();

            if (IsSymbol(Peek())) {
                member.Name = Next();
            } else if (IsString(Peek())) {
                TStringBuf quoted = Next();
                member.Name = quoted.SubStr(1, quoted.size() - 2);
            } else {
                ThrowSyntaxError("Expected attribute name");
            }
            member.CppName = ToCamelCase(member.Name);

            if (Peek() == "(") {
                ParseAttrs(nullptr, &member, nullptr);
            }
            if (structDef.Packing == TStructAttrs::ArrayPacked && !member.Idx.Defined()) {
                member.Idx = structDef.Members.empty() ? 0 : (1 + *structDef.Members.back().Idx);
            }

            Next(":");

            ParseType(member.Type);

            if (Peek() == "(" && !member.Type.Struct) {
                ParseAttrs(nullptr, &member, &member);
            }

            // replace inline struct definition with named struct if need
            AddStructTypeToScopeIfNeed(member.Type, "T" + member.CppName);
            if (member.Type.Struct) {
                TString typeName = "T" + member.CppName;
                TNamedType namedStruct(typeName, false);
                namedStruct.Type = member.Type;
                CurrentScope->LocalTypes.push_back(namedStruct);
                member.Type.Name = typeName;
                member.Type.Struct.Reset(nullptr);
            }

            if (!IsCamelCase(member.CppName)) {
                ThrowSomeError("All fields must be named in camelcase in c++. Use (cppname = CamelCaseName) if you desire", definitionStart);
            }

            Next(";");
            structDef.Members.push_back(member);
        }
    }
}

void TParser::ParseType(TType& type, bool structAllowed) {
    if (TryNext("[")) {
        type.ArrayOf.Reset(new TType);
        ParseType(*type.ArrayOf);
        Next("]");
        return;
    } else if (TryNext("{")) {
        TString keyTypeName = ToString(NextSymbol());
        TType valueType;

        // compatibility with old syntax
        if (TryNext("}")) {
            // TODO: better warning
            Cerr << "domscheme (.sc) warning: using old dict syntax {type}. Use {string -> type} now\n";
            valueType.Name = keyTypeName;
            auto res = CurrentScope->LookupType(valueType.Name);
            if (!res.IsKnown) {
                ThrowSyntaxError("Expected type (bool, string etc)");
            }
            valueType.IsTopLevel = res.IsTopLevel;
            keyTypeName = "string";
        } else {
            Next("->");
            ParseType(valueType);
            // TODO: validate key type (?)
            Next("}");
        }

        const TBuiltinType* const* keyType = Builtins().SchemeNameToType.FindPtr(keyTypeName);
        if (!keyType) {
            ThrowSyntaxError("Expected type as key of dict");
        } else if (!(*keyType)->CanBeDictKey) {
            ThrowSyntaxError("\"" + keyTypeName + "\" can't be a key of dict");
        }

        type.DictOf.Reset(new std::pair<const TBuiltinType*, TType>(*keyType, valueType));
        return;
    } else if (structAllowed && TryNext("struct")) {
        bool isArray = TryNext("[") && Next("]");
        type.Struct.Reset(new TStruct);
        if (Peek() == "(") {
            ParseAttrs(type.Struct.Get(), nullptr, nullptr);
        }
        ParseStructBody(*type.Struct);
        if (isArray) {
            TType arrayType(type.IsTopLevel);
            arrayType.ArrayOf.Reset(new TType(type));
            type = arrayType;
        }
        return;
    }
    // TODO: validate type name (?)
    type.Name = ToString(NextSymbol());
    auto res = CurrentScope->LookupType(type.Name);
    if (!res.IsKnown) {
        ThrowSyntaxError("Expected type (bool, string etc)");
    }
    type.IsTopLevel = res.IsTopLevel;

    // For compatibility with old syntax
    if (TryNext("[") && Next("]")) {
        TType arrayType(type.IsTopLevel);
        arrayType.ArrayOf.Reset(new TType(type));
        type = arrayType;
    }
}

void TParser::ParseAttrsDefaultValue(TValueAttrs* valueAttrs) {
    Next("=");
    if (TryNext("[")) {
        bool first = true;
        while (Peek() != "]") {
            if (!first) {
                Next(",");
                // allow trailing comma
                if (Peek() == "]") {
                    break;
                }
            }
            valueAttrs->DefaultArray.push_back(ToString(Next()));
            first = false;
        }
        Next("]");
    } else {
        TStringBuf str = Next();
        valueAttrs->DefaultValue = GetPragmas().ProcessSvnKeywords && IsSvnKeyword(str) ?
                                   ExtractSvnKeywordValue(str) :
                                   ToString(str);
    }
}

void TParser::ParseAttrs(TStructAttrs* structAttrs, TFieldAttrs* fieldAttrs, TValueAttrs* valueAttrs) {
    Next("(");
    while (!TryNext(")")) {
        if (structAttrs && TryNext("array")) {
            structAttrs->Packing = TStructAttrs::ArrayPacked;
        } else if (structAttrs && TryNext("strict")) {
            structAttrs->Policy = TStructAttrs::AlwaysStrict;
        } else if (structAttrs && TryNext("relaxed")) {
            structAttrs->Policy = TStructAttrs::AlwaysRelaxed;
        } else if (fieldAttrs && TryNext("cppname")) {
            Next("=");
            fieldAttrs->CppName = ToString(NextSymbol());
        } else if (fieldAttrs && TryNext("idx")) {
            Next("=");
            fieldAttrs->Idx = FromString<size_t>(NextUnsignedNumber());
        } else if (fieldAttrs && TryNext("required")) {
            fieldAttrs->Required = true;
        } else if (fieldAttrs && TryNext("const")) {
            fieldAttrs->Const = true;
            ParseAttrsDefaultValue(valueAttrs);
        } else if (valueAttrs && TryNext("default")) {
            ParseAttrsDefaultValue(valueAttrs);
        } else if (valueAttrs && TryNext("allowed")) {
            TValidationOperation op { "allowed", {} };
            Next("=");
            Next("[");
            bool first = true;
            while (Peek() != "]") {
                if (!first) {
                    Next(",");
                    // allow trailing comma
                    if (Peek() == "]") {
                        break;
                    }
                }
                op.Values.push_back(ToString(Next()));
                first = false;
            }
            Next("]");
            valueAttrs->ValidationOperators.push_back(op);
        } else if (valueAttrs && IsValidationOp(Peek())) {
            TValidationOperation op { ToString(Next()), { ToString(Next()) } };
            valueAttrs->ValidationOperators.push_back(op);
        } else {
            ThrowSyntaxError(TString("Unknown attribute: ") + Peek());
        }
        if (Peek() != ")") {
            Next(",");
        }
    }
}

void TParser::ParseParentTypes(TStruct& structDef) {
    do {
        const TString name = ToString(NextSymbol());
        auto res = CurrentScope->LookupType(name);
        if (!res.IsKnown) {
            ThrowSyntaxError("Expected known type, given " + name);
        }

        TParentType parent{name, res.IsTopLevel};

        if (IsIn(structDef.ParentTypes, parent)) {
            ThrowSyntaxError(name + " is already a parent type");
        }

        structDef.ParentTypes.push_back(parent);
        if (Peek() == "{") {
            break;
        }
        Next(",");
    } while (true);
}

void TParser::AddStructTypeToScopeIfNeed(TType& type, const TString& name) {
    if (type.Struct) {
        TNamedType namedStruct(name, false);
        namedStruct.Type = type;
        CurrentScope->LocalTypes.push_back(namedStruct);
        type.Name = name;
        type.Struct.Reset(nullptr);
    } else if (type.ArrayOf) {
        AddStructTypeToScopeIfNeed(*type.ArrayOf, name);
    } else if (type.DictOf) {
        AddStructTypeToScopeIfNeed(type.DictOf->second, name);
    }
}

}
