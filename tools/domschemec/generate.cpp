#include "generate.h"
#include "builtins.h"

#include <util/datetime/base.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/escape.h>
#include <util/string/join.h>
#include <util/string/subst.h>

#include <algorithm>

namespace NDomSchemeCompiler {

TGenerator::TGenerator(IOutputStream& out)
    : OutStream(out)
    , Indent(0)
{
}

IOutputStream& TGenerator::O() {
    OutStream << "\n";
    for (size_t i = 0; i < Indent; ++i) {
        OutStream << "    ";
    }
    return OutStream;
}

struct TGenerator::TPath {
    struct TPart {
        bool IsNamespace;
        TString Name;
    };

    TVector<TPart> Parts;

    TPath operator+(const TPart& part) const {
        TPath res { Parts };
        res.Parts.push_back(part);
        return res;
    }

    TString ToString(bool isConst) const {
        TString res;
        bool seenStruct = false;
        for (const auto& part : Parts) {
            res += "::" + part.Name + (!part.IsNamespace && isConst ? "Const" : "");
            if (!part.IsNamespace && !seenStruct) {
                res += "<TTraits>";
                seenStruct = true;
            }
        }
        return res;
    }
};

struct TGenerator::TListGenerator {
    TListGenerator(const TStringBuf& first, const TStringBuf& separator)
        : First(first)
        , Separator(separator)
    {
    }

    TListGenerator& operator<<(const TStringBuf& value) {
        if (Result)
            Result << Separator;
        else
            Result << First;
        Result << value;
        return *this;
    }

    const TString& GetResult() const {
        return Result;
    }

    TStringBuilder Result;
    TStringBuf First;
    TStringBuf Separator;
};

void TGenerator::Run(const TProgram& program) {
    for (const auto& ns : program.Namespaces) {
        TPath path;
        for (const auto& name : ns.Name) {
            O() << "namespace " << name << " {\n";
            path.Parts.push_back(TPath::TPart{true, name});
        }

        ++Indent;
        for (const auto& t : ns.Types) {
            if (t.Type.Struct) {
                GenerateStruct(path, t.Name, t.Type.IsTopLevel, true, *t.Type.Struct);
                GenerateStruct(path, t.Name, t.Type.IsTopLevel, false, *t.Type.Struct);
            } else {
                GenerateTypedef(t);
            }
        }
        --Indent;

        for (const auto& name : ns.Name) {
            Y_UNUSED(name);
            O() << "}\n";
        }
    }
    O();
}

void TGenerator::GenerateStruct(const TPath& path, const TString& astName, bool isTopLevel, bool isConst1, const TStruct& s) {
    TString name = astName + (isConst1 ? "Const" : "");

    if (isTopLevel) {
        O() << "template <typename TTraits>";
    } else {
        if (!isConst1) {
            O() << "using " << name << "Const = typename" << (path + TPath::TPart{ false, astName }).ToString(true) << ";\n";
        }
    }
    const auto tValueRef =  TString("typename TTraits::TValueRef");
    const auto tConstValueRef =  TString("typename TTraits::TConstValueRef");
    const auto& tValueRefText = isConst1 ? tConstValueRef : tValueRef;
    const auto valueField = TString("Value__");

    if (!s.ParentTypes.empty()) {
        TListGenerator parentTypesList(": public ", ", public ");
        for (const auto& parent : s.ParentTypes)
            parentTypesList << GetUserCppType(parent.Name, isConst1, parent.IsTopLevel);
        O() << "struct " << name << parentTypesList.GetResult() << " {";
    } else {
        O() << "struct " << name << " {";
    }

    ++Indent;
    if (isTopLevel) {
        O() << "using TValueRef = " << tValueRefText << ";\n";
    }
    O() << "using TConst = typename" << (path + TPath::TPart{ false, astName }).ToString(true) << ";\n";

    TPath newPath = path + TPath::TPart{ false, astName };
    for (const auto& ss : s.Types) {
        if (ss.Type.Struct) {
            GenerateStruct(newPath, ss.Name, ss.Type.IsTopLevel, isConst1, *ss.Type.Struct);
        } else {
            Y_FAIL("typedef inside structs NOT IMPLEMENTED");
        }
    }

    O() << "TValueRef " << valueField << ";\n";

    O() << "inline " << name << "(const TValueRef& value)";
    TListGenerator memberInitializerList("    : ", ", ");
    for (const auto& parent : s.ParentTypes)
        memberInitializerList << (GetUserCppType(parent.Name, isConst1, parent.IsTopLevel) + "(value)");
    memberInitializerList << (valueField + "(value)");
    O() << memberInitializerList.GetResult();
    O() << "{";
    O() << "}\n";

    if (!isConst1) {
        O() << "inline operator TConst() const {";
        ++Indent;
        O() << "return TConst(" << valueField << ");";
        --Indent;
        O() << "}\n";
    }

    O() << "inline const " << name << "* operator->() const noexcept {";
    ++Indent;
    O() << "return this;";
    --Indent;
    O() << "}\n";

    if (!isConst1) {
        O() << "inline " << name << "* operator->() noexcept {";
        ++Indent;
        O() << "return this;";
        --Indent;
        O() << "}\n";
    }

    O() << "inline TValueRef GetRawValue() const {";
    ++Indent;
    O() << "return " << valueField << ";";
    --Indent;
    O() << "}\n";

    for (const auto& m : s.Members) {
        auto makeField = [&](bool isConst2) {
            O() << "inline " << GetCppType(m.Type, isConst2) << " " << m.CppName << "() " << (isConst2 ? "const " : "") << "{";
            ++Indent;
            IOutputStream& line = O();
            if (s.Packing == TStructAttrs::ArrayPacked) {
                line << "return " << GetCppType(m.Type, isConst2) << "{TTraits::ArrayElement(" << valueField << ", " << *m.Idx << ")";
            } else {
                line << "return " << GetCppType(m.Type, isConst2) << "{TTraits::GetField(" << "static_cast<" << (isConst2 ? tConstValueRef : tValueRef) << ">(" << valueField << "), TStringBuf(\"" << m.Name << "\"))";
            }
            if (m.Type.ArrayOf) {
                if (const auto& defaultArray = GetDefaultArray(m)) {
                    line << ", {";
                    bool first = true;
                    for (const auto& value : *defaultArray) {
                        if (!first) {
                            line << ", ";
                        }
                        line << value;
                        first = false;
                    }
                    line << "}";
                }
            } else if (m.Type.Name) {
                if (const auto& defaultValue = GetDefaultValue(m)) {
                    line << ", " << *defaultValue;
                }
            }
            line << "};";
            --Indent;
            O() << "}\n";

            // simplified accessors
            if (m.Type.ArrayOf || m.Type.DictOf) {
                TStringBuf idxType = m.Type.ArrayOf ? TStringBuf("size_t") : TStringBuf(m.Type.DictOf->first->CppName);
                TType& elemType = m.Type.ArrayOf ? *m.Type.ArrayOf : m.Type.DictOf->second;
                O() << "inline " << GetCppType(elemType, isConst2) << " " << m.CppName << "(" << idxType << " idx) " << (isConst2 ? "const " : "") << "{";
                ++Indent;
                O() << "return " << m.CppName << "()[idx];";
                --Indent;
                O() << "}\n";
            }
        };

        if (s.Packing == TStructAttrs::DictPacked) {
            O() << "inline bool Has" << m.CppName << "() " << "const {";
            O() << "    return !this->" << m.CppName << "().IsNull();";
            O() << "}\n";
        }

        makeField(true);
        if (!isConst1) {
            makeField(false);
        }
    }

    O() << "inline bool IsNull() const {";
    O() << "    return TTraits::IsNull(" << valueField << ");";
    O() << "}";

    O() << "inline TString ToJson() const {";
    O() << "    return TTraits::ToJson(" << valueField << ");";
    O() << "}";

    if (!isConst1) {
        TVector<const TMemberDefinition> nonConstMembers;
        std::for_each(s.Members.cbegin(), s.Members.cend(), [&](const auto &m) { if (!m.Const) { nonConstMembers.emplace_back(m); } });

        O() << "inline bool Validate(const TString& path = TString(), bool strict = false, const NDomSchemeRuntime::TErrorCallback& onError = NDomSchemeRuntime::TErrorCallback()) const {";
        O() << "    return Validate(path, strict, [&onError](TString p, TString e, NDomSchemeRuntime::TValidateInfo) { if (onError) { onError(p, e); } });";
        O() << "}";
        O() << "template <typename THelper = std::nullptr_t>";
        O() << "inline bool Validate(const TString& path = TString(), bool strict = false, const NDomSchemeRuntime::TErrorCallbackEx& onError = NDomSchemeRuntime::TErrorCallbackEx(), THelper helper = THelper()) const {";
        O() << "    return TConst(*this).Validate(path, strict, onError, helper);";
        O() << "}";

        O() << "inline void Clear() {";
        ++Indent;
        for (const auto& parent : s.ParentTypes)
            O() << GetUserCppType(parent.Name, isConst1, parent.IsTopLevel) << "::Clear();";
        if (s.Packing == TStructAttrs::DictPacked) {
            O() << "return TTraits::DictClear(this->" << valueField << ");";
        } else {
            O() << "return TTraits::ArrayClear(this->" << valueField << ");";
        }
        --Indent;
        O() << "}";

        O() << "template <typename TRhs>";
        O() << "inline " << name << "& Assign(const TRhs& rhs) {";
        ++Indent;
        O() << "Clear();";
        if (nonConstMembers.empty() && s.ParentTypes.empty()) {
            O() << "Y_UNUSED(rhs);";
        }
        for (const auto& parent : s.ParentTypes)
            O() << GetUserCppType(parent.Name, isConst1, parent.IsTopLevel) << "::Assign(rhs);";
        for (const auto& m : nonConstMembers) {
            O() << "if (!rhs." << m.CppName << "().IsNull()) {";
            O() << "    " << m.CppName << "() = rhs." << m.CppName << "();";
            O() << "}";
        }
        O() << "return *this;";
        --Indent;
        O() << "}";

        O() << "template <typename TRhs>";
        O() << "inline " << name << "& operator=(const TRhs& rhs) {";
        O() << "    return Assign(rhs);";
        O() << "}";

        O() << "inline " << name << "& operator=(const " << name << "& rhs) {";
        O() << "    if (GetRawValue() == rhs.GetRawValue()) {";
        O() << "        return *this;";
        O() << "    }";
        O() << "    return Assign(rhs);";
        O() << "}";

        O() << name << "() = default;";
        O() << name << "(const " << name << "& other) = default;";

        O() << "inline " << name << "& SetDefault() {";
        ++Indent;
        for (const auto& m : s.Members) {
            if (m.Type.ArrayOf) {
                if (const auto& defaultArray = GetDefaultArray(m)) {
                    O() << "if (!Has" << m.CppName << "()) {";
                    ++Indent;
                    IOutputStream& line = O();
                    line << m.CppName << "() = {";
                    bool first = true;
                    for (const auto& value : *defaultArray) {
                        if (!first) {
                            line << ", ";
                        }
                        line << value;
                        first = false;
                    }
                    line << "};";
                    --Indent;
                    O() << "}";
                }
            } else if (m.Type.Name) {
                if (const auto& defaultValue = GetDefaultValue(m)) {
                    if (m.Const) {
                        O() << m.CppName << "() = " << *defaultValue << ";";
                    } else {
                        O() << "if (!Has" << m.CppName << "()) {";
                        ++Indent;
                        O() << m.CppName << "() = " << *defaultValue << ";";
                        --Indent;
                        O() << "}";
                    }
                } else if (!Builtins().SchemeNameToType.FindPtr(m.Type.Name)) {
                    O() << m.CppName << "().SetDefault();";
                }
            }
        }
        for (const auto& parent : s.ParentTypes)
            O() << GetUserCppType(parent.Name, isConst1, parent.IsTopLevel) << "::SetDefault();";
        O() << "return *this;";
        --Indent;
        O() << "}";

    } else {
        O() << "inline bool Validate(const TString& path = TString(), bool strict = false, const NDomSchemeRuntime::TErrorCallback& onError = NDomSchemeRuntime::TErrorCallback()) const {";
        O() << "    return Validate(path, strict, [&onError](TString p, TString e, NDomSchemeRuntime::TValidateInfo) { if (onError) { onError(p, e); } });";
        O() << "}";
        O() << "template <typename THelper = std::nullptr_t>";
        O() << "inline bool Validate(const TString& path = TString(), bool strict = false, const NDomSchemeRuntime::TErrorCallbackEx& onError = NDomSchemeRuntime::TErrorCallbackEx(), THelper helper = THelper()) const {";
        ++Indent;
        O() << "(void)strict;";
        O() << "(void)helper;";
        O() << "if (TTraits::IsNull(" << valueField << ")) {";
        O() << "    return true;";
        if (s.Packing == TStructAttrs::DictPacked) {
            O() << "} else if (!TTraits::IsDict(" << valueField << ")) {";
            O() << "    if (onError) { onError(path, \"is not a dict\", NDomSchemeRuntime::TValidateInfo()); }";
        } else {
            O() << "} else if (!TTraits::IsArray(" << valueField << ")) {";
            O() << "    if (onError) { onError(path, \"is not an array\", NDomSchemeRuntime::TValidateInfo()); }";
        }
        O() << "    return false;";
        O() << "}";

        {
            if (s.ParentTypes.empty()) {
                O() << "bool ok = true;";
            } else {
                TListGenerator okDefinition("bool ok = ", " && ");
                for (const auto& parent : s.ParentTypes)
                    okDefinition << (GetUserCppType(parent.Name, isConst1, parent.IsTopLevel) + "::Validate(path, strict, onError, helper)");
                O() << okDefinition.GetResult() << ";";
            }
        }

        for (const auto& m : s.Members) {
            if (m.Required) {
                O() << "bool found_" << m.Name << " = false;";
            }
        }

        auto generateValidationCode = [&](const TMemberDefinition& m, const TString& varName) {
            O() << "if (!" << varName << ".Validate(subPath, strict, onError, helper)) {";
            O() << "    ok = false;";
            O() << "}";
            if (!m.ValidationOperators.empty()) {
                O() << "auto AddError = [&](const TString& msg) {";
                O() << "    if (onError) { onError(subPath, msg, NDomSchemeRuntime::TValidateInfo()); }";
                O() << "    ok = false;";
                O() << "};";
                O() << "(void)AddError;";
            }
            for (const auto& validationOp : m.ValidationOperators) {
                if (validationOp.Op == "code") {
                    Y_VERIFY(validationOp.Values.size() == 1);
                    O() << "{";
                    O() << "    auto validator = [this, &AddError, &subPath, &ok, &helper]() {";
                    O() << "        Y_UNUSED(AddError);";
                    O() << "        Y_UNUSED(subPath);";
                    O() << "        Y_UNUSED(ok);";
                    O() << "        Y_UNUSED(helper);";
                    O() << validationOp.Values[0];
                    O() << "    };";
                    O() << "    validator();";
                    O() << "}";
                } else if (validationOp.Op == "allowed") {
                    O() << "if (!(false";
                    for (const auto& val : validationOp.Values) {
                        O() << "      || (" << varName << ".Get() == " << val << ")";
                    }
                    O() << ")) {";
                    O() << "    if (onError) { onError(subPath, \"should be one of [" << EscapeC(JoinSeq(", ", validationOp.Values)) << "]\", NDomSchemeRuntime::TValidateInfo()); }";
                    O() << "    ok = false;";
                    O() << "}";
                } else {
                    Y_VERIFY(validationOp.Values.size() == 1);
                    O() << "if (!(" << varName << ".Get() " << validationOp.Op << " " << validationOp.Values[0] <<")) {";
                    O() << "    if (onError) { onError(subPath, \"should be " << validationOp.Op << " " << EscapeC(validationOp.Values[0]) << "\", NDomSchemeRuntime::TValidateInfo()); }";
                    O() << "    ok = false;";
                    O() << "}";
                }
            }
        };

        if (s.Packing == TStructAttrs::DictPacked) {
            O() << "auto keys = TTraits::GetKeys(" << valueField << ");";
            O() << "for (const auto& key : keys) {";
            ++Indent;

            O() << "static constexpr std::array<TStringBuf, " << s.Members.size() << "> knownNames {{";
            for (const auto& m : s.Members) {
                O() << "    TStringBuf(\"" << m.Name << "\"),";
            }
            O() << "}};";
            O() << "const bool isKnown = (Find(knownNames, key) != knownNames.end());";
            O() << "Y_UNUSED(isKnown);";

            O() << "bool found = false;";
            O() << "TString loweredKey = to_lower(key);";

            for (const auto& m : s.Members) {
                TString fuzzyMatchCondition;

                switch (s.Policy) {
                    case TStructAttrs::AlwaysRelaxed: {
                        fuzzyMatchCondition = "false";
                        break;
                    }
                    case TStructAttrs::AlwaysStrict: {
                        fuzzyMatchCondition = "!isKnown";
                        break;
                    }
                    case TStructAttrs::Flexible: {
                        fuzzyMatchCondition = "strict && !isKnown";
                        break;
                    }
                }

                O() << "if (key == TStringBuf(\"" << m.Name << "\") || "
                    << fuzzyMatchCondition << " && NDomSchemeRuntime::IsSimilar(loweredKey, "
                    << "TStringBuf(\"" << to_lower(m.Name) << "\"))) {";

                ++Indent;
                O() << "found = true;";
                O() << "const TString subPath = path + \"/\" + key;";
                O() << "if (key != TStringBuf(\"" << m.Name << "\")) {";
                O() << "    if (onError) { onError(subPath, \"looks like misspelled \\\""
                    << m.Name << "\\\"\", NDomSchemeRuntime::TValidateInfo{NDomSchemeRuntime::TValidateInfo::ESeverity::Warning}); }";
                O() << "    ok = false;";
                O() << "} else {";
                if (m.Required) {
                    O() << "    found_" << m.Name << " = true;";
                }
                O() << "}";
                O() << GetCppType(m.Type, true) << " f {TTraits::GetField(" << valueField << ", key)};";
                generateValidationCode(m, "f");
                --Indent;
                O() << "}";
            }

            switch (s.Policy) {
                case TStructAttrs::AlwaysRelaxed: {
                    if (s.Members.empty()) {
                        O() << "Y_UNUSED(found);";
                    }
                    break;
                }
                case TStructAttrs::AlwaysStrict:
                case TStructAttrs::Flexible: {
                    if (s.Policy == TStructAttrs::AlwaysStrict) {
                        O() << "if (!found) {";
                    } else {
                        O() << "if (!found && strict) {";
                    }

                    O() << "    if (onError) { onError(path + \"/\" + key, \"is an unknown key\", NDomSchemeRuntime::TValidateInfo()); }";
                    O() << "    ok = false;";
                    O() << "}";
                    break;
                }
            }

            --Indent;
            O() << "}";
        } else { // IsArray
            for (const auto& m : s.Members) {
                TString varName = "field_" + ToString(*m.Idx);
                O() << GetCppType(m.Type, true) << " " << varName << " {TTraits::ArrayElement(" << valueField << ", " << *m.Idx << ")};";
                O() << "if (!" << varName << ".IsNull()) {";
                ++Indent;
                if (m.Required) {
                    O() << "found_" << m.Name << " = true;";
                }
                O() << "{";
                ++Indent;
                O() << "const TString subPath = path + \"/" + ToString(*m.Idx) + "\";";
                generateValidationCode(m, varName);
                --Indent;
                O() << "}";
                --Indent;
                O() << "}";
            }

            Y_VERIFY(s.Policy == TStructAttrs::Flexible);
        }
        for (const auto& m : s.Members) {
            if (m.Required) {
                O() << "if (!found_" << m.Name << ") {";
                O() << "    ok = false;";
                O() << "    if (onError) { onError(path + \"/" << m.Name << "\", \"is a required field and is not found\", NDomSchemeRuntime::TValidateInfo()); }";
                O() << "}";
            }
        }
        if (!s.ValidationOperators.empty()) {
            O() << "auto AddError = [&](const TString& msg) {";
            O() << "    if (onError) { onError(path, msg, NDomSchemeRuntime::TValidateInfo()); }";
            O() << "    ok = false;";
            O() << "};";
            O() << "(void)AddError;";
            for (const auto& validationOp : s.ValidationOperators) {
                if (validationOp.Op != "code") {
                    ythrow yexception() << "Internal compiler error: Unexpected validation code";
                }
                Y_VERIFY(validationOp.Values.size() == 1);
                O() << "{";
                O() << "    auto validator = [this, &AddError, &path, &ok, &helper]() {";
                O() << "        Y_UNUSED(AddError);";
                O() << "        Y_UNUSED(path);";
                O() << "        Y_UNUSED(ok);";
                O() << "        Y_UNUSED(helper);";
                O() << validationOp.Values[0];
                O() << "    };";
                O() << "    validator();";
                O() << "}";
            }
        }
        O() << "return ok;";
        --Indent;
        O() << "}";
    }

    --Indent;
    O() << "};\n";
}

void TGenerator::GenerateTypedef(const TNamedType& type) {
    TString templatePrefix = type.Type.IsTopLevel ? "template <typename TTraits> " : "";
    O() << templatePrefix << "using " << type.Name << " = " << GetCppType(type.Type, false) << ";";
    O() << templatePrefix << "using " << type.Name << "Const = " << GetCppType(type.Type, true) << ";";
}

TMaybe<TString> TGenerator::GetDefaultValue(const TMemberDefinition& member) const {
    if (!member.DefaultValue) {
        return Nothing();
    }
    TString defaultValue = member.DefaultValue;
    if (member.Type.Name == "duration") {
        // TODO: move this check to parsing phase so we can give exact position
        TDuration defDuration;
        TString defaultWithoutQuotes = defaultValue;
        SubstGlobal(defaultWithoutQuotes, "\"", "");
        if (!TDuration::TryParse(defaultWithoutQuotes, defDuration)) {
            ythrow yexception() << "Can't parse default duration value: " << defaultValue;
        }
        defaultValue = "TDuration::MicroSeconds(" + ToString(defDuration.MicroSeconds()) + ")";
    }
    return defaultValue;
}

TMaybe<TVector<TString>> TGenerator::GetDefaultArray(const TMemberDefinition& member) const {
    if (!member.DefaultArray) {
        return Nothing();
    }
    // TODO: TDuration, see above
    return member.DefaultArray;
}

TString TGenerator::GetUserCppType(const TString& name, bool isConst, bool isTopLevel) {
    return TStringBuilder() << name << (isConst ? "Const" : "") << (isTopLevel ? "<TTraits>" : "");
}

TString TGenerator::GetCppType(const TType& t, bool isConst) {
    const TString constPart = isConst ? "Const" : "";

    if (t.ArrayOf) {
        return "::NDomSchemeRuntime::T" + constPart + "Array" + "<TTraits, " + GetCppType(*t.ArrayOf, isConst) + ">";
    } else if (t.DictOf) {
        return "::NDomSchemeRuntime::T" + constPart + "Dict" + "<TTraits, " + t.DictOf->first->CppName + ", " + GetCppType(t.DictOf->second, isConst) + ">";
    } else if (t.Name) {
        const TBuiltinType* const* bt = Builtins().SchemeNameToType.FindPtr(t.Name);
        return
            bt ?
                (t.Name == "any" ?
                 "::NDomSchemeRuntime::T" + constPart + "AnyValue" + "<TTraits>" :
                 "::NDomSchemeRuntime::T" + constPart + "Primitive" + "<TTraits, " + (*bt)->CppName + ">") :
            GetUserCppType(t.Name, isConst, t.IsTopLevel);
    }

    Y_FAIL("UNSUPPORTED type (struct should not get this far in codegen)");
}

}
