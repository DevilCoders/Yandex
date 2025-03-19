#pragma once

#include <kernel/proto_codegen/codegen.h>

#include <util/charset/wide.h>
#include <util/string/cast.h>
#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>
#include <util/digest/multi.h>
#include <util/generic/maybe.h>
#include <util/generic/xrange.h>

namespace Ngp = ::google::protobuf;

class TGeneratedName {
public:
    TGeneratedName() = default;

    TGeneratedName(const TStringBuf& name, const TStringBuf& enumTypeName)
        : EnumTypeName(enumTypeName)
        , Name(name)
    {
        TUtf16String wName = TUtf16String::FromAscii(name);
        wName.to_upper();
        EnumName = WideToASCII(wName);
    }
    TGeneratedName(const TStringBuf& name, const TStringBuf& enumName, const TStringBuf& enumTypeName)
        : EnumTypeName(enumTypeName)
        , Name(name)
        , EnumName(enumName)
    {}

    const TString& GetName() const {
        return Name;
    }
    const TString& GetEnumName() const {
        return EnumName;
    }
    const TString GetCppName() const {
        return EnumTypeName + "::" + EnumName;
    }

    bool IsDistinctFrom(const TGeneratedName& other) const {
        return Name != other.Name && EnumName != other.EnumName;
    }
    bool operator == (const TGeneratedName& other) const {
        return Name == other.Name && EnumName == other.EnumName;
    }

private:
    TString EnumTypeName;
    TString Name;
    TString EnumName;
};

class TSliceName
    : public TGeneratedName
{
public:
    TSliceName() = default;

    TSliceName(const TStringBuf& name)
        : TGeneratedName(name, "EFactorSlice")
    {}
    TSliceName(const TStringBuf& name, const TStringBuf& enumName)
        : TGeneratedName(name, enumName, "EFactorSlice")
    {}

    static const TSliceName& All() {
        static TSliceName sliceName("all");
        return sliceName;
    }
    static const TSliceName& Count() {
        static TSliceName sliceName("count");
        return sliceName;
    }
};

class TRoleName
    : public TGeneratedName
{
public:
    TRoleName() = default;

    TRoleName(const TStringBuf& name)
        : TGeneratedName(name, "ESliceRole")
    {}
    TRoleName(const TStringBuf& name, const TStringBuf& enumName)
        : TGeneratedName(name, enumName, "ESliceRole")
    {}
};

class TUniverseName
    : public TGeneratedName
{
public:
    TUniverseName() = default;

    TUniverseName(const TStringBuf& name)
        : TGeneratedName(name, "EFactorUniverse")
    {}
    TUniverseName(const TStringBuf& name, const TStringBuf& enumName)
        : TGeneratedName(name, enumName, "EFactorUniverse")
    {}
};

template<>
inline void Out<TSliceName>(IOutputStream& os, const TSliceName& sliceName)
{
    os << sliceName.GetName() << "(" << sliceName.GetEnumName() << ")";
}

template<>
inline void Out<TRoleName>(IOutputStream& os, const TRoleName& roleName)
{
    os << roleName.GetName() << "(" << roleName.GetEnumName() << ")";
}

template<>
inline void Out<TUniverseName>(IOutputStream& os, const TUniverseName& universeName)
{
    os << universeName.GetName() << "(" << universeName.GetEnumName() << ")";
}

template <class TCodeGenInput>
class TFactorSlicesCodegen {
private:
    const char* Indent = "    ";

    struct TSliceProps {
        TMaybe<ui32> NumFactors;
        bool Hierarchical = false;
        TSliceName SliceName;
        TSliceName ParentName;
        TSliceName NextName;
        TSliceName NextSiblingName;
        TSliceName FirstChildName;
    };

    struct TRoleProps {
        TSliceName DefaultSlice;
    };

    struct TUniverseProps {
        THashMap<TString, TSliceName> SliceByRole;
    };

    TVector<TString> Namespaces;
    TVector<TSliceName> SliceNames;
    TVector<TRoleName> RoleNames;
    TVector<TUniverseName> UniverseNames;
    THashMap<TString, TSliceProps> SliceProps;
    THashMap<TString, TUniverseProps> UniverseProps;

public:
    void GenCode(TCodeGenInput& input, TCodegenParams& params) {
        SliceNames.clear();
        CollectSliceNames(input, SliceNames);
        SliceProps.clear();
        CollectSliceProps(input, SliceProps, SliceNames);

        RoleNames.clear();
        CollectRoleNames(input, RoleNames);

        UniverseNames.clear();
        UniverseProps.clear();
        CollectUniverseNamesAndProps(input, UniverseNames, UniverseProps);

        GetNamespaces(params, Namespaces);

        FillTop(params);
        OpenNamespaces(params);

        FillForwardDecl(params);
        FillSlicesEnum(params);
        FillRolesEnum(params);
        FillUniversesEnum(params);
        FillStaticSliceInfo(params);
        FillStaticRoleInfo(params);
        FillStaticUniverseInfo(params);
        FillSliceMapTemplate(params);
        FillMetaInfoClass(input, params);

        CloseNamespaces(params);

        FillSliceFromStringFunction(params);
        FillSliceOutFunction(params);
        FillRoleOutFunction(params);
        FillUniverseOutFunction(params);
    }

private:
    // Utility functions
    //
    static void GetNamespaces(TCodegenParams& params, TVector<TString>& result) {
        for(int i = 0; i < params.ArgcRest; ++i ) {
            result.push_back(params.ArgvRest[i]);
        }
    }

    template <typename TMessage>
    static void MakeSliceName(const TMessage& slice, TSliceName& sliceName) {
        sliceName = TSliceName(slice.GetName());
    }

    template <typename TMessage>
    static void CollectSliceNamesHelper(const TMessage& input, TVector<TSliceName>& sliceNames) {
        for (size_t sliceIndex = 0; sliceIndex != input.SliceSize(); ++sliceIndex) {
            TSliceName sliceName;
            MakeSliceName(input.GetSlice(sliceIndex), sliceName);

            Y_ENSURE(sliceName.IsDistinctFrom(TSliceName::All()),
                "slice name " << TSliceName::All() << " is reserved");
            Y_ENSURE(sliceName.IsDistinctFrom(TSliceName::Count()),
                "slice name " << TSliceName::Count() << "is reserved");

            for (const TSliceName& otherName : sliceNames) {
                Y_ENSURE(sliceName.IsDistinctFrom(otherName),
                    "slice " << otherName << " occurs second time as " << sliceName);

            }

            sliceNames.push_back(sliceName);
            CollectSliceNamesHelper(input.GetSlice(sliceIndex), sliceNames);
        }
    }

    static void CollectSliceNames(const TCodeGenInput& input,
        TVector<TSliceName>& sliceNames)
    {
        sliceNames.push_back(TSliceName::All());
        CollectSliceNamesHelper(input, sliceNames);
    }

    static void CollectRoleNames(const TCodeGenInput& input,
        TVector<TRoleName>& roleNames)
    {
        for (const auto& role : input.GetRole()) {
            TRoleName roleName(role.GetName());

            for (const auto& otherName : roleNames) {
                Y_ENSURE(roleName.IsDistinctFrom(otherName),
                    "role " << otherName << " occurs second time as " << roleName);

            }

            roleNames.push_back(roleName);
        }
    }

    static void CollectUniverseNamesAndProps(const TCodeGenInput& input,
        TVector<TUniverseName>& universeNames,
        THashMap<TString, TUniverseProps>& universeProps)
    {
        for (const auto& universe : input.GetUniverse()) {
            TUniverseName universeName(universe.GetName());

            for (const auto& otherName : universeNames) {
                Y_ENSURE(universeName.IsDistinctFrom(otherName),
                    "universe " << otherName << " occurs second time as " << universeName);

            }

            universeNames.push_back(universeName);
            for (const auto& assign : universe.GetAssign()) {
                universeProps[universe.GetName()].SliceByRole[assign.GetRole()] = TSliceName(assign.GetSlice());
            }
        }
    }

    template <typename TMessage>
    static void CollectSlicePropsHelper(TMessage& input, const TSliceName& parentName,
        THashMap<TString, TSliceProps>& sliceProps)
    {
        auto& parentProps = sliceProps[parentName.GetName()];

        if (input.SliceSize() == 0) {
            parentProps.FirstChildName = TSliceName::Count();
            return;
        }

        TVector<TSliceName> childNames(input.SliceSize());
        for (size_t sliceIndex = 0; sliceIndex != input.SliceSize(); ++sliceIndex) {
            MakeSliceName(input.GetSlice(sliceIndex), childNames[sliceIndex]);
        }
        parentProps.FirstChildName = childNames[0];

        for (size_t sliceIndex = 0; sliceIndex != input.SliceSize(); ++sliceIndex) {
            const auto& slice = input.GetSlice(sliceIndex);
            auto& props = sliceProps[slice.GetName()];

            if (slice.SliceSize() > 0) {
                props.Hierarchical = true;
            }
            if (slice.HasNumFactors()) {
                props.NumFactors = slice.GetNumFactors();
            }
            props.ParentName = parentName;

            props.SliceName = childNames[sliceIndex];
            if (sliceIndex + 1 < input.SliceSize()) {
                props.NextSiblingName = childNames[sliceIndex + 1];
            } else {
                props.NextSiblingName = TSliceName::Count();
            }

            CollectSlicePropsHelper(slice, childNames[sliceIndex], sliceProps);
        }
    }

    static void CollectSliceProps(TCodeGenInput& input,
        THashMap<TString, TSliceProps>& sliceProps,
        const TVector<TSliceName>& sliceNames)
    {
        auto& allProps = sliceProps[TSliceName::All().GetName()];
        allProps.Hierarchical = true;
        allProps.SliceName = TSliceName::All();
        allProps.ParentName = TSliceName::Count(); // Invalid parent
        allProps.NextSiblingName = TSliceName::Count(); // Invalid sibling

        CollectSlicePropsHelper(input, TSliceName::All(), sliceProps);

        for (size_t i = 0; i != sliceNames.size(); ++i) {
            auto& props = sliceProps[sliceNames[i].GetName()];
            if (i + 1 < sliceNames.size()) {
                props.NextName = sliceNames[i + 1];
            } else {
                props.NextName = TSliceName::Count();
            }
        }

    }

    TString GetNamespacesPrefix() const {
        TString res = "::";
        for (const TString& ns : Namespaces) {
            res += ns + TString("::");
        }
        return res;
    }

    // Codegen functions
    //
    void FillTop(TCodegenParams& params) {
        params.Hdr
            << "#pragma once" "\n"
            << "\n"
            << "#include <kernel/factor_slices/slices_codegen/stdlib_deps.h>" "\n"
            << "\n"
            << "#include <util/system/guard.h>" "\n"
            << "#include <util/system/mutex.h>" "\n"
            << "#include <util/string/cast.h>" "\n"
            << "#include <util/generic/string.h>" "\n"
            << "#include <util/generic/hash_set.h>" "\n"
            << "#include <util/generic/hash.h>" "\n"
            << "#include <util/generic/xrange.h>" "\n"
            << "#include <util/generic/typetraits.h>" "\n"
            << "#include <util/ysaveload.h>" "\n"
            << "\n"
            << "class IFactorsInfo;" "\n";

        params.Cpp
            << "#include \"" << params.HeaderFileName << "\"" "\n"
            << "\n"
            << "#include <kernel/factor_slices/factor_borders.h>" "\n"
            << "#include <kernel/factors_info/factors_info.h>" "\n"
            << "\n"
            << "#include <util/generic/singleton.h>" "\n"
            << "\n";

        for (TString &ns : Namespaces) {
            params.Cpp << "using namespace " << ns << ";" "\n";
        }
    }

    void OpenNamespaces(TCodegenParams& params) {
        params.Hdr << "\n";
        params.Cpp << "\n";

        for (const TString& ns : Namespaces) {
            params.Hdr << "namespace " << ns << " {" "\n";
            params.Cpp << "namespace " << ns << " {" "\n";
        }
    }

    void CloseNamespaces(TCodegenParams& params) {
        params.Hdr << "\n";
        params.Cpp << "\n";

        for (const TString& ns : Namespaces) {
            params.Hdr << "} // namespace " << ns << "\n";
            params.Cpp << "} // namespace " << ns << "\n";
        }
    }

    void FillForwardDecl(TCodegenParams& params) {
        params.Hdr << "\n"
            << "struct TSliceStaticInfo;" "\n"
            << "class TSliceOffsets;" "\n"
            << "class TFactorBorders;" "\n"
            << "class TSlicesMetaInfoBase;" "\n";
    }

    void FillSlicesEnum(TCodegenParams& params) {
        params.Hdr << "\n"
            << "enum class EFactorSlice {" "\n";

        for (size_t index = 0; index != SliceNames.size(); ++index) {
            params.Hdr << Indent  << SliceNames[index].GetEnumName()
                << " = " << index << "," "\n";
        }

        params.Hdr
            << Indent << TSliceName::Count().GetEnumName()
                << " = " << SliceNames.size() << "\n"
            << "};" "\n";

        params.Hdr << "\n"
            << "constexpr size_t N_SLICE_COUNT = static_cast<size_t>("
                << TSliceName::Count().GetCppName() << ");" "\n"
            << "\n"
            << "using TAllSlicesArray = std::array<EFactorSlice, N_SLICE_COUNT>;" "\n"
            << "using TSlicesSet = THashSet<EFactorSlice>;" "\n";
    }

   void FillRolesEnum(TCodegenParams& params) {
        params.Hdr << "\n"
            << "enum class ESliceRole {" "\n";

        for (size_t index = 0; index != RoleNames.size(); ++index) {
            params.Hdr << Indent  << RoleNames[index].GetEnumName()
                << " = " << index << (index + 1 < RoleNames.size() ? "," : "") << "\n";
        }

        params.Hdr
            << "};" "\n"
            << "\n"
            << "constexpr size_t N_ROLES_COUNT = " << RoleNames.size() << ";" "\n";
    }

   void FillUniversesEnum(TCodegenParams& params) {
        params.Hdr << "\n"
            << "enum class EFactorUniverse {" "\n";

        for (size_t index = 0; index != UniverseNames.size(); ++index) {
            params.Hdr << Indent  << UniverseNames[index].GetEnumName()
                << " = " << index << (index + 1 < UniverseNames.size() ? "," : "") << "\n";
        }

        params.Hdr
            << "};" "\n"
            << "\n"
            << "constexpr size_t N_UNIVERSES_COUNT = " << UniverseNames.size() << ";" "\n";
    }

    void FillStaticSliceInfo(TCodegenParams& params) {
        params.Hdr << "\n"
            << "struct TSliceStaticInfo {" << "\n"
            << Indent << "const char* Name;" "\n"
            << Indent << "const char* CppName;" "\n"
            << Indent << "bool Hierarchical;" "\n"
            << Indent << "EFactorSlice Parent;" "\n"
            << Indent << "EFactorSlice Next;" "\n"
            << Indent << "EFactorSlice NextSibling;" "\n"
            << Indent << "EFactorSlice FirstChild;" "\n"
            << "};" "\n";

        params.Hdr << "\n"
            << "const TSliceStaticInfo& GetStaticSliceInfo(EFactorSlice slice);" "\n"
            << "\n"
            << "// Slices are listed in order of factor indices" "\n"
            << "const TAllSlicesArray& GetAllFactorSlices();" "\n";

        params.Cpp << "\n"
            << "const TSliceStaticInfo& GetStaticSliceInfo(EFactorSlice slice)" "\n"
            << "{" "\n"
            << Indent << "static const TSliceStaticInfo staticSliceInfo[N_SLICE_COUNT + 1] = {" "\n";

        for (size_t sliceIndex = 0; sliceIndex != SliceNames.size(); ++sliceIndex) {
            const TSliceName& sliceName = SliceNames[sliceIndex];
            const TSliceProps& props = SliceProps[sliceName.GetName()];

            params.Cpp << Indent << Indent << "{ "
                << "\"" << sliceName.GetName() << "\"" ", "
                << "\"" << GetNamespacesPrefix() << sliceName.GetCppName() << "\"" ", "
                << (props.Hierarchical ? "true" : "false") << ", "
                << (props.ParentName.GetCppName()) << ", "
                << (props.NextName.GetCppName()) << ", "
                << (props.NextSiblingName.GetCppName()) << ", "
                << (props.FirstChildName.GetCppName())
                << " }," "\n";
        }

        params.Cpp << Indent << Indent << "{ "
            << "\"" << TSliceName::Count().GetName() << "\"" ", "
            << "\"" << GetNamespacesPrefix() << TSliceName::Count().GetCppName() << "\"" ", "
            << "false, " << TSliceName::Count().GetCppName() << ", "
            << TSliceName::Count().GetCppName() << ", "
            << TSliceName::Count().GetCppName() << ", "
            << TSliceName::Count().GetCppName()
            << "}" "\n";

        params.Cpp
            << Indent << "};" "\n"
            << "\n"
            << Indent << "Y_ASSERT(static_cast<size_t>(slice) < N_SLICE_COUNT);" "\n"
            << Indent << "if (Y_LIKELY(static_cast<size_t>(slice) < N_SLICE_COUNT)) {" "\n"
            << Indent << Indent << "return staticSliceInfo[static_cast<size_t>(slice)];" "\n"
            << Indent << "}" "\n"
            << Indent << "return staticSliceInfo[N_SLICE_COUNT];" "\n"
            << "}" "\n";

        params.Cpp << "\n"
            << "const TAllSlicesArray& GetAllFactorSlices()" "\n"
            << "{" "\n"
            << Indent << "static TAllSlicesArray staticAllSlices = { {\n";

        for (size_t i : xrange(SliceNames.size())) {
            params.Cpp << Indent << Indent
                << SliceNames[i].GetCppName() << "," "\n";
        }

        params.Cpp
            << Indent << "} };" "\n"
            << Indent << "return staticAllSlices;" "\n"
            << "}" "\n";
    }

    void FillStaticRoleInfo(TCodegenParams& params) {
        params.Hdr << "\n"
            << "struct TRoleStaticInfo {" << "\n"
            << Indent << "const char* Name;" "\n"
            << Indent << "const char* CppName;" "\n"
            << "};" "\n";

        params.Hdr << "\n"
            << "const TRoleStaticInfo& GetStaticRoleInfo(ESliceRole role);" "\n"
            << "\n";

        params.Cpp << "\n"
            << "const TRoleStaticInfo& GetStaticRoleInfo(ESliceRole role)" "\n"
            << "{" "\n"
            << Indent << "static TRoleStaticInfo staticRoleInfo[N_ROLES_COUNT] = {" "\n";

        for (const size_t roleIndex : xrange(RoleNames.size())) {
            const auto& roleName = RoleNames[roleIndex];

            params.Cpp << Indent << Indent << "{ "
                << "\"" << roleName.GetName() << "\"" ", "
                << "\"" << GetNamespacesPrefix() << roleName.GetCppName() << "\" }";

            params.Cpp << (roleIndex + 1 < RoleNames.size() ? "," : "") << "\n";
        }

        params.Cpp
            << Indent << "};" "\n"
            << "\n"
            << Indent << "Y_ASSERT(static_cast<size_t>(role) < N_ROLES_COUNT);" "\n"
            << Indent << "return staticRoleInfo[static_cast<size_t>(role)];" "\n"
            << "}" "\n";
    }

    void FillStaticUniverseInfo(TCodegenParams& params) {
        params.Hdr << "\n"
            << "struct TUniverseStaticInfo {" << "\n"
            << Indent << "const char* Name;" "\n"
            << Indent << "const char* CppName;" "\n"
            << Indent << "std::array<EFactorSlice, N_ROLES_COUNT> SliceByRole;" "\n"
            << "};" "\n";

        params.Hdr << "\n"
            << "const TUniverseStaticInfo& GetStaticUniverseInfo(" << GetNamespacesPrefix() << "EFactorUniverse universe);" "\n"
            << "\n";

        params.Cpp << "\n"
            << "const TUniverseStaticInfo& GetStaticUniverseInfo(EFactorUniverse universe)" "\n"
            << "{" "\n"
            << Indent << "static TUniverseStaticInfo staticUniverseInfo[N_UNIVERSES_COUNT] = {" "\n";

        for (const size_t universeIndex : xrange(UniverseNames.size())) {
            const auto& universeName = UniverseNames[universeIndex];
            const auto& props = UniverseProps[universeName.GetName()];

            params.Cpp << Indent << Indent << "{ "
                << "\"" << universeName.GetName() << "\"" ", "
                << "\"" << GetNamespacesPrefix() << universeName.GetCppName() << "\"" ", {{";

            for (const size_t roleIndex : xrange(RoleNames.size())) {
                const auto& roleName = RoleNames[roleIndex];
                auto iter = props.SliceByRole.find(roleName.GetName());
                if (iter != props.SliceByRole.end()) {
                    params.Cpp << iter->second.GetCppName();
                } else {
                    params.Cpp << TSliceName::Count().GetCppName();
                }
                params.Cpp << (roleIndex + 1 < RoleNames.size() ? ", " : "");
            }

            params.Cpp << "}} }" << (universeIndex + 1 < UniverseNames.size() ? "," : "") << "\n";
        }

        params.Cpp
            << Indent << "};" "\n"
            << "\n"
            << Indent << "Y_ASSERT(static_cast<size_t>(universe) < N_UNIVERSES_COUNT);" "\n"
            << Indent << "return staticUniverseInfo[static_cast<size_t>(universe)];" "\n"
            << "}" "\n";
    }

    void FillSliceOutFunction(TCodegenParams& params) {
        params.Hdr << "\n"
            << "TString ToCppString(" << GetNamespacesPrefix() << "EFactorSlice slice);" "\n";

        params.Cpp << "\n"
            << "template<>" "\n"
            << "void Out< " << GetNamespacesPrefix() << "EFactorSlice >"
                << "(IOutputStream& os, TTypeTraits< " << GetNamespacesPrefix() << "EFactorSlice >::TFuncParam slice)" "\n"
            << "{" "\n"
            << Indent << "os << GetStaticSliceInfo(slice).Name;" "\n"
            << "}" "\n";

        params.Cpp << "\n"
            << "TString ToCppString(EFactorSlice slice)" "\n"
            << "{" "\n"
            << Indent << "return GetStaticSliceInfo(slice).CppName;" "\n"
            << "}" "\n";
    }

    void FillRoleOutFunction(TCodegenParams& params) {
        params.Hdr << "\n"
            << "TString ToCppString(" << GetNamespacesPrefix() << "ESliceRole role);" "\n";

        params.Cpp << "\n"
            << "template<>" "\n"
            << "void Out< " << GetNamespacesPrefix() << "ESliceRole >"
                << "(IOutputStream& os, TTypeTraits< " << GetNamespacesPrefix() << "ESliceRole >::TFuncParam role)" "\n"
            << "{" "\n"
            << Indent << "os << GetStaticRoleInfo(role).Name;" "\n"
            << "}" "\n";

        params.Cpp << "\n"
            << "TString ToCppString(ESliceRole role)" "\n"
            << "{" "\n"
            << Indent << "return GetStaticRoleInfo(role).CppName;" "\n"
            << "}" "\n";
    }

    void FillUniverseOutFunction(TCodegenParams& params) {
        params.Hdr << "\n"
            << "TString ToCppString(" << GetNamespacesPrefix() << "EFactorUniverse universe);" "\n";

        params.Cpp << "\n"
            << "template<>" "\n"
            << "void Out< " << GetNamespacesPrefix() << "EFactorUniverse >"
                << "(IOutputStream& os, TTypeTraits< " << GetNamespacesPrefix() << "EFactorUniverse >::TFuncParam universe)" "\n"
            << "{" "\n"
            << Indent << "os << GetStaticUniverseInfo(universe).Name;" "\n"
            << "}" "\n";

        params.Cpp << "\n"
            << "TString ToCppString(EFactorUniverse universe)" "\n"
            << "{" "\n"
            << Indent << "return GetStaticUniverseInfo(universe).CppName;" "\n"
            << "}" "\n";
    }

    void FillSliceFromStringFunction(TCodegenParams& params) {
        params.Hdr << "\n"
            << "bool FromString(const TStringBuf& str, " << GetNamespacesPrefix() << "EFactorSlice& slice);" "\n";

        params.Cpp << "\n"
            << "bool FromString(const TStringBuf& str, EFactorSlice& slice)" "\n"
            << "{" "\n"
            << Indent << "static const THashMap<TStringBuf, EFactorSlice> names = {" "\n";
        for (const TSliceName& sliceName : SliceNames) {
           params.Cpp
               << Indent << Indent << "{\"" << sliceName.GetName() << "\"sv, " << sliceName.GetCppName() << "}," "\n";
        }
        params.Cpp
            << Indent << "};" "\n"
            << Indent << "if (const EFactorSlice* result = names.FindPtr(str)) {" "\n"
            << Indent << Indent << "slice = *result;" "\n"
            << Indent << Indent << "return true;" "\n"
            << Indent << "}" "\n";

        params.Cpp
            << Indent << "return false;" "\n"
            << "}" "\n";

        params.Cpp << "\n"
            << "template<> EFactorSlice FromStringImpl<EFactorSlice, char>(char const* data, size_t size)" "\n"
            << "{" "\n"
            << Indent << "EFactorSlice slice = " << TSliceName::Count().GetCppName() << ";" "\n"
            << Indent << "if (!FromString(TStringBuf(data, size), slice)) {" "\n"
            << Indent << Indent << "ythrow yexception() << \"bad slice name: \" << TStringBuf(data, size);" "\n"
            << Indent << "}" "\n"
            << Indent << "return slice;" "\n"
            << "}" << "\n";

        params.Cpp << "\n"
            << "template<>" "\n"
            << "bool TryFromStringImpl< " << GetNamespacesPrefix() << "EFactorSlice, char >"
                << "(char const* data, size_t size, " << GetNamespacesPrefix() << "EFactorSlice& slice)" "\n"
            << "{" "\n"
            << Indent << "return FromString(TStringBuf(data, size), slice);" "\n"
            << "}" << "\n";
    }

    void FillSliceMapTemplate(TCodegenParams& params) {
        params.Hdr << "\n"
            << "#include <kernel/factor_slices/slice_map.inc>" "\n";
    }

    struct TSameHierarchicalSliceGroup {
        bool Hierarchical;
        TVector<size_t> Indices;
    };

    template <typename TMessage>
    TVector<TSameHierarchicalSliceGroup> GroupLeafSlices(TMessage& input) const {
        TVector<TSameHierarchicalSliceGroup> result;
        for (size_t sliceIndex = 0; sliceIndex != input.SliceSize(); ++sliceIndex) {
            const bool hierarchical = GetSliceProps(input, sliceIndex).Hierarchical;
            if (result.empty() || hierarchical || result.back().Hierarchical) {
                result.push_back(TSameHierarchicalSliceGroup{hierarchical, {}});
            }
            result.back().Indices.push_back(sliceIndex);
        }
        return result;
    }

    template <typename TMessage>
    const TSliceProps& GetSliceProps(TMessage& input, const size_t sliceIndex) const {
        const auto& slice = input.GetSlice(sliceIndex);
        TString name = slice.GetName();
        const TSliceProps& props = SliceProps.at(name);
        return props;
    }

    void FillMakeBordersMethodLeafSliceHelper(const TString& sliceVarName, TCodegenParams& params, const TString& curIndent) const {
        params.Cpp
            << curIndent << "if (IsSliceEnabled(" << sliceVarName << ") && GetNumFactors(" << sliceVarName << ") > 0) {" "\n"
            << curIndent << Indent << "borders[" << sliceVarName << "].Begin = curOffset;" "\n"
            << curIndent << Indent << "curOffset += GetNumFactors(" << sliceVarName << ");" "\n"
            << curIndent << Indent << "borders[" << sliceVarName << "].End = curOffset;" "\n"
            << curIndent << "}" "\n";
    }

    template <typename TMessage>
    void FillMakeBordersMethodBodyHelper(TMessage& input, TCodegenParams& params, const TString& curIndent) {
        const TVector<TSameHierarchicalSliceGroup> sameHierarchicalSliceGroup = GroupLeafSlices(input);
        for (const size_t groupIndex : xrange(sameHierarchicalSliceGroup.size())) {
            const TSameHierarchicalSliceGroup& sliceGroup = sameHierarchicalSliceGroup[groupIndex];
            if (sliceGroup.Hierarchical) {
                Y_ENSURE(sliceGroup.Indices.size() == 1);
                const size_t sliceIndex = sliceGroup.Indices.front();
                const auto& slice = input.GetSlice(sliceIndex);
                const TSliceName& sliceName = GetSliceProps(input, sliceIndex).SliceName;
                params.Cpp
                    << curIndent << "if (IsSliceEnabled(" << sliceName.GetCppName() << ")) {" "\n"
                    << curIndent << Indent << "ptrdiff_t beginOffset = curOffset;" "\n";

                FillMakeBordersMethodBodyHelper(slice, params, curIndent + TString(Indent));

                params.Cpp
                    << curIndent << Indent << "if (curOffset > beginOffset) {" "\n"
                    << curIndent << Indent << Indent << "borders[" << sliceName.GetCppName() << "].Begin = beginOffset;" "\n"
                    << curIndent << Indent << Indent << "borders[" << sliceName.GetCppName() << "].End = curOffset;" "\n"
                    << curIndent << Indent << "}" "\n"
                    << curIndent << "}" "\n";
            } else {
                if (sliceGroup.Indices.size() <= 2) {
                    for (const size_t sliceIndex : sliceGroup.Indices) {
                        const TSliceName& sliceName = GetSliceProps(input, sliceIndex).SliceName;
                        FillMakeBordersMethodLeafSliceHelper(sliceName.GetCppName(), params, curIndent);
                    }
                } else {
                    const TString groupName = "group" + ToString(groupIndex) + "H" + ToString(curIndent.size());
                    params.Cpp
                        << curIndent << "static constexpr EFactorSlice " << groupName << "[] {" "\n";
                    for (const size_t sliceIndex : sliceGroup.Indices) {
                        const TSliceName& sliceName = GetSliceProps(input, sliceIndex).SliceName;
                        params.Cpp
                            << curIndent << Indent << sliceName.GetCppName() << "," "\n";
                    }
                    params.Cpp
                        << curIndent << "};" "\n";
                    params.Cpp
                        << curIndent << "for (const EFactorSlice slice : " << groupName << ") {" "\n";
                    FillMakeBordersMethodLeafSliceHelper("slice", params, curIndent + TString(Indent));
                    params.Cpp
                        << curIndent << "}" "\n";
                }
            }
        }
    }

    void FillMakeBordersMethod(TCodeGenInput& input, TCodegenParams& params) {
        params.Cpp << "\n"
            << "void TSlicesMetaInfoBase::MakeBorders(TFactorBorders& borders) const" "\n"
            << "{" "\n"
            << Indent << "ptrdiff_t curOffset = 0;" "\n"
            << "\n";

        params.Cpp
            << Indent << "if (IsSliceEnabled(" << TSliceName::All().GetCppName() << ")) {" "\n"
            << Indent << Indent << "borders[" << TSliceName::All().GetCppName() << "].Begin = curOffset;" "\n"
            << "\n";

        FillMakeBordersMethodBodyHelper(input, params, TString(Indent) + TString(Indent));

        params.Cpp << "\n"
            << Indent << Indent << "borders[" << TSliceName::All().GetCppName() << "].End = curOffset;" "\n"
            << Indent << "}" "\n"
            << "}" "\n";
    }

    void FillMetaInfoClass(TCodeGenInput& input, TCodegenParams& params) {
        params.Hdr << "\n"
            << "class TSlicesMetaInfoBase {" "\n"
            << "public:" "\n"
            << Indent << "TSlicesMetaInfoBase();" "\n"
            << Indent << "bool operator == (const TSlicesMetaInfoBase& other) const;"
            << "\n"
            << Indent << "void InitSliceOnce(EFactorSlice slice, ui32 numFactors);" "\n"
            << Indent << "\n"
            << Indent << "void SetSliceEnabled(EFactorSlice slice, bool enabled);" "\n"
            << Indent << "bool IsSliceEnabled(EFactorSlice slice) const;" "\n"
            << "\n"
            << Indent << "void SetNumFactors(EFactorSlice slice, ui32 numFactors);" "\n"
            << Indent << "ui32 GetNumFactors(EFactorSlice slice) const;" "\n"
            << "\n"
            << Indent << "bool IsSliceInitialized(EFactorSlice slice) const;" "\n"
            << "\n"
            << Indent << "TFactorBorders GetBorders() const;" "\n"
            << "\n"
            << "private:" "\n"
            << Indent << "friend class TFactorBorders;" "\n"
            << Indent << "void MakeBorders(TFactorBorders& borders) const;" "\n"
            << Indent << "\n"
            << "private:" "\n"
            << Indent << "struct TSliceInfo {" "\n"
            << Indent << Indent << "bool Enabled = false;" "\n"
            << Indent << Indent << "bool Initialized = false;" "\n"
            << Indent << Indent << "ui32 NumFactors = 0;" "\n"
            << "\n"
            << Indent << Indent << "bool operator == (const TSliceInfo& other) const {" "\n"
            << Indent << Indent << Indent << "return Enabled == other.Enabled &&" "\n"
            << Indent << Indent << Indent << Indent << "Initialized == other.Initialized &&" "\n"
            << Indent << Indent << Indent << Indent << "NumFactors == other.NumFactors;" "\n"
            << Indent << Indent << "}" "\n"
            << Indent << "};" "\n"
            << "\n"
            << Indent << "TSliceMap<TSliceInfo> Info;" "\n"
            << "};" "\n";

        params.Cpp << "\n"
            << "TSlicesMetaInfoBase::TSlicesMetaInfoBase()" "\n"
            << "{" "\n";

        for (const TSliceName& sliceName : SliceNames) {
            const auto& props = SliceProps[sliceName.GetName()];

            if (sliceName == TSliceName::All()) {
                params.Cpp << Indent << "Info[" << sliceName.GetCppName()
                    << "].Enabled = true;" "\n";
            }

            if (props.NumFactors.Defined() || props.Hierarchical) {
                params.Cpp << Indent << "Info[" << sliceName.GetCppName()
                    << "].Initialized = true;" "\n";
            }

            if (props.NumFactors.Defined()) {
                params.Cpp << Indent << "Info[" << sliceName.GetCppName()
                    << "].NumFactors = " << props.NumFactors.GetRef() << ";" "\n";
            }
        }

        params.Cpp
            << "}" "\n"
            << "\n"
            << "bool TSlicesMetaInfoBase::operator == (const TSlicesMetaInfoBase& other) const" "\n"
            << "{" "\n"
            << Indent << "return Info == other.Info;" "\n"
            << "}" "\n"
            << "\n"
            << "void TSlicesMetaInfoBase::InitSliceOnce(EFactorSlice slice, ui32 numFactors)" "\n"
            << "{" "\n"
            << Indent << "Y_VERIFY_DEBUG(!Info[slice].Initialized," "\n"
            << Indent << Indent << "\"slice %s was already initialized\", GetStaticSliceInfo(slice).Name);" "\n"
            << Indent << "SetNumFactors(slice, numFactors);" "\n"
            << "}" "\n"
            << "\n"
            << "void TSlicesMetaInfoBase::SetSliceEnabled(EFactorSlice slice, bool enabled)" "\n"
            << "{" "\n"
            << Indent << "Info[slice].Enabled = enabled;" "\n"
            << "}" "\n"
            << "\n"
            << "bool TSlicesMetaInfoBase::IsSliceEnabled(EFactorSlice slice) const" "\n"
            << "{" "\n"
            << Indent << "return Info[slice].Enabled;" "\n"
            << "}" "\n";

        params.Cpp << "\n"
            << "void TSlicesMetaInfoBase::SetNumFactors(EFactorSlice slice, ui32 numFactors)" "\n"
            << "{" "\n"
            << Indent << "Y_VERIFY_DEBUG(!GetStaticSliceInfo(slice).Hierarchical," "\n"
            << Indent << Indent << "\"slice %s is hierarchical\", GetStaticSliceInfo(slice).Name);" "\n"
            << Indent << "Info[slice].NumFactors = numFactors;" "\n"
            << Indent << "Info[slice].Initialized = true;" "\n"
            << "}" "\n"
            << "\n"
            << "ui32 TSlicesMetaInfoBase::GetNumFactors(EFactorSlice slice) const" "\n"
            << "{" "\n"
            << Indent << "Y_VERIFY_DEBUG(IsSliceInitialized(slice), \"slice %s was not initialized; maybe you forgot to list its factors codegen in your project PEERDIR?\", GetStaticSliceInfo(slice).Name);" "\n"
            << Indent << "if (GetStaticSliceInfo(slice).Hierarchical) {" "\n"
            << Indent << Indent << "return 0;" "\n"
            << Indent << "}" "\n"
            << Indent << "return Info[slice].NumFactors;" "\n"
            << "}" "\n"
            << "\n"
            << "bool TSlicesMetaInfoBase::IsSliceInitialized(EFactorSlice slice) const" "\n"
            << "{" "\n"
            << Indent << "return Info[slice].Initialized;" "\n"
            << "}" "\n"
            << "\n"
            << "TFactorBorders TSlicesMetaInfoBase::GetBorders() const" "\n"
            << "{" "\n"
            << Indent << "TFactorBorders borders;" "\n"
            << Indent << "MakeBorders(borders);" "\n"
            << Indent << "return borders;" "\n"
            << "}" "\n";

        FillMakeBordersMethod(input, params);
    }
 };
