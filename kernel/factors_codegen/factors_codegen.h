#pragma once

#include "factor_slices_mixin.h"

#include <kernel/proto_codegen/codegen.h>

#include <library/cpp/protobuf/json/proto2json.h>

#include <util/digest/multi.h>
#include <util/digest/sequence.h>
#include <util/generic/adaptor.h>
#include <util/string/builder.h>
#include <util/string/escape.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>

namespace Ngp = ::google::protobuf;

typedef TMap<TString, TVector<size_t> > TFactorNameToGroupsMap;

struct TFactorsCodegenDefaultTraits {
    static const bool UseFactorSlices = false;
    static const bool UseExtJsonInfo = false;
};

template <class TCodeGenInput, class TCodeGenTraits = TFactorsCodegenDefaultTraits>
class TFactorsCodegen
    : public TFactorSlicesMixin<TCodeGenInput, TCodeGenTraits>
{
public:
    using TSlicesCodegen = TFactorSlicesMixin<TCodeGenInput, TCodeGenTraits>;

public:
    void GenCode(TCodeGenInput& input, TCodegenParams& params) {
        CutUnwantedSlices(input, params);
        TVector<TStringBuf> namespaces;
        GetNamespace(params, namespaces);
        TSlicesCodegen::PrepareSliceDescriptors(input);
        PreprocessInput(input);
        CheckFactorIndexes(input);
        CheckFactorRequiredFields(input);
        CheckTagsRestrictions(input);
        FillTop(params);

        for (TStringBuf& ns : namespaces) {
            params.Hdr << "namespace " << ns << " {\n\n";
            params.Cpp << "namespace " << ns << " {\n\n";
        }

        FillGroupsData(input, params);
        FillFactorsMask(input, params);
        FillFactorsID(input, params);
        TSlicesCodegen::FillSliceFactorIdEnums(input, params, GetNamespacePrefix(namespaces));
        FillAntiSEO(input, params);
        FillFactorGroupMask(input, params);
        TSlicesCodegen::FillSlicesInitializer(input, params);
        FillFactorInfo(params);
        FillFactorInfoConstructor(params);
        for (TStringBuf& ns : Reversed(namespaces)) {
            params.Hdr << "\n} // namespace " << ns;
            params.Cpp << "\n} // namespace " << ns;
        }

        for (auto&& cpp : params.ExtraCpp) {
            TStringStream ss;

            ss << "#include \"" << params.HeaderFileName << "\"\n\n";

            for (TStringBuf& ns : namespaces) {
                ss << "namespace " << ns << " {\n\n";
            }

            ss << cpp.second->Str();

            for (TStringBuf& ns : Reversed(namespaces)) {
                ss << "\n} // namespace " << ns;
            }

            cpp.second->Str().swap(ss.Str());
        }

        TSlicesCodegen::UseSliceNamespacesInGlobalNamespace(input, params, GetNamespacePrefix(namespaces));
    }

private:

    template <class TFunc>
    static void ForEachCppStream(TCodegenParams& params, TFunc&& action) {
        if (params.CppParts) {
            for (size_t i = 0; i < params.CppParts; ++i) {
                action(params.CppStream(i));
            }
        } else {
            action(params.Cpp);
        }
    };

    void FillTop(TCodegenParams& params) {
        params.Hdr << "#pragma once\n\n"
                    << "#include <kernel/factor_storage/factor_storage.h>\n\n"
                    << "#include <kernel/factors_info/factors_info.h>\n\n"
                    << "#include <kernel/factors_codegen/stdlib_deps.h>\n\n"
                    << "#include <util/generic/array_ref.h>\n"
                    << "#include <util/generic/bitmap.h>\n"
                    << "#include <util/generic/hash.h>\n"
                    << "#include <util/generic/noncopyable.h>\n"
                    << "#include <util/generic/string.h>\n"
                    << "#include <util/generic/utility.h>\n"
                    << "#include <util/generic/vector.h>\n"
                    << "#include <util/stream/output.h>\n"
                    << "#include <util/system/defaults.h>\n\n";

        params.Cpp << "#include \"" << params.HeaderFileName <<  "\"\n\n";
    }

    void PrintGetTagsFunction(TCodeGenInput& input, TCodegenParams& params) {
        params.Hdr << "struct TTagsData {\n"
                << "    THashSet<TString> TagsNames;\n"
                << "    THashSet<int> TagsIds;\n";
        params.Hdr << "};\n\n";

        params.Hdr << R"(
namespace NDetail {
    struct TTagsDataInit: public TTagsData {
        struct TTagPair {
            TStringBuf Name;
            int Id;
        };
        using TType = TTagPair;
    };
} // namespace NDetail

)";
        {
            auto& cpp = params.MethodStream(0);
            cpp << R"(
namespace {
    struct TTagsDataConstructor: public NDetail::TTagsDataInit {
        template <class TStringConstructor>
        TTagsDataConstructor(const TArrayRef<const NDetail::TTagsDataInit::TType> tags, TStringConstructor& toString) {
            for (const auto& tag : tags) {
                TagsNames.insert(toString(tag.Name));
                TagsIds.insert(tag.Id);
            }
        }
    };
} // namespace

)";
        }
        TConstexprArrayPrinter arrayPrinter;
        for (size_t i = 0; i < input.FactorSize(); ++i) {
            const typename TCodeGenInput::TFactorDescriptor& descr = input.GetFactor(i);
            auto& cpp = params.UniformStream(i, input.FactorSize());

            TVector<TString> items(Reserve(descr.TagsSize()));
            for (size_t t = 0; t < descr.TagsSize(); ++t) {
                items.push_back(TStringBuilder() << "{" << WrapStringBuf(GetTagName(descr, t)) << ", " << static_cast<int>(descr.GetTags(t)) << "}");
            }
            arrayPrinter.PrintUnordered(cpp, "", "NDetail::TTagsDataInit::TTagPair", "TagsDataInitList" + GetHandle(descr), std::move(items), ToString(descr.GetIndex()));
        }

    }

    static TString GetHandle(const typename TCodeGenInput::TFactorDescriptor& descr) {
        const bool useLongNames = false;
        if (useLongNames) {
            return descr.GetName();
        } else {
            return "F" + ToString(descr.GetIndex());
        }
    }

    static TString WrapStringBuf(const TStringBuf rawString) {
        return TStringBuilder() << "\"" << EscapeC(rawString) << "\"sv";
    }

    void PrintGetGroupsFunction(TCodeGenInput& input, TCodegenParams& params) {
        params.Hdr << "struct TGroupsData {\n"
                << "    THashSet<TString> GroupNames;\n"
                << "};\n\n";

        params.Hdr << R"(
namespace NDetail {
    struct TGroupsDataInit: public TGroupsData {
        using TType = TStringBuf;
    };
} // namespace NDetail

)";
        {
            auto& cpp = params.MethodStream(0);
            cpp << R"(
namespace {
    struct TGroupsDataConstructor: public NDetail::TGroupsDataInit {
        template <class TStringConstructor>
        TGroupsDataConstructor(const TArrayRef<const NDetail::TGroupsDataInit::TType> groups, TStringConstructor& toString) {
            for (const auto& groupName : groups) {
                GroupNames.insert(toString(groupName));
            }
        }
    };
} // namespace

)";
        }

        TConstexprArrayPrinter arrayPrinter;
        for (size_t i = 0; i < input.FactorSize(); ++i) {
            const typename TCodeGenInput::TFactorDescriptor& descr = input.GetFactor(i);
            auto& cpp = params.UniformStream(i, input.FactorSize());

            TVector<TString> items(Reserve(descr.GroupSize()));
            for (size_t g = 0; g < descr.GroupSize(); ++g) {
                items.push_back(WrapStringBuf(descr.GetGroup(g)));
            }
            arrayPrinter.PrintUnordered(cpp, "", "TStringBuf", "GroupsDataInitList" + GetHandle(descr), std::move(items), ToString(descr.GetIndex()));
        }
    }

    void FillGroupsData(TCodeGenInput& input, TCodegenParams& params) {
        if (input.GroupSize() > 0 || NeedOffsetGators(input)) {
            TStringStream protectedFactorStorage;
            protectedFactorStorage
                << "class TProtectedFactorStorage: private TNonCopyable {\n"
                << "private:\n"
                << "    float* Factor;\n"
                << "public:\n"
                << "    TProtectedFactorStorage(const TFactorView& factor)\n"
                << "        : Factor(factor.GetRawFactors())\n"
                << "    {}\n";

            GenFactorsAccessor(input, params, "All", false, protectedFactorStorage);
            TFactorNameToGroupsMap factorNameToGroups;
            for (size_t i = 0; i != input.GroupSize(); ++i) {
                const TString& groupName = input.GetGroup(i);
                GenFactorsAccessor(input, params, groupName, true, protectedFactorStorage);
            }
            GenOffsetGators(input, protectedFactorStorage);
            protectedFactorStorage << "};";
            params.Hdr << "\n" << protectedFactorStorage.Str() << "\n\n";
            TVector<TString> fields;
            for (size_t i = 0; i != input.GroupSize(); ++i) {
                fields.push_back(input.GetGroup(i));
            }
            GenMask(params, "TFactorGroupMask", fields);
        } else {
            GenMask(params, "TFactorGroupMask", TVector<TString>());
        }
    }

    void PrintMoveFromFactorSourceFunction(TCodeGenInput& input, TCodegenParams& params) {
        params.Hdr << "struct TSourcesArgClassNameData {\n"
                << "       THashMap<TString, TString> SourceArgToClassName;\n";
        params.Hdr << "};\n\n";

        params.Hdr << "struct TMoveFromFactorSource {\n"
                   << "    TSourcesArgClassNameData Sources;\n"
                   << "    const char* Expression;\n"
                   << "};\n\n";

        params.Hdr << R"(
namespace NDetail {
    struct TSourcesArgClassNameDataInit: public TSourcesArgClassNameData {
        struct TSourcePair {
            TStringBuf Name;
            TStringBuf Arg;
        };
        using TType = TSourcePair;
    };
} // namespace NDetail

)";
        {
            auto& cpp = params.MethodStream(0);
            cpp << R"(
namespace {
    struct TSourcesArgClassNameDataConstructor: public NDetail::TSourcesArgClassNameDataInit {
        template <class TStringConstructor>
        TSourcesArgClassNameDataConstructor(const TArrayRef<const NDetail::TSourcesArgClassNameDataInit::TType> sources, TStringConstructor& toString) {
            for (const auto& source : sources) {
                SourceArgToClassName[toString(source.Arg)] = toString(source.Name);
            }
        }
    };
} // namespace

)";
        }
        TConstexprArrayPrinter arrayPrinter;
        for (size_t i = 0; i < input.FactorSize(); ++i) {
            const typename TCodeGenInput::TFactorDescriptor& descr = input.GetFactor(i);
            if (descr.HasMoveFromFactorSource()) {
                auto& cpp = params.UniformStream(i, input.FactorSize());

                TVector<TString> items(Reserve(descr.GetMoveFromFactorSource().SourcesSize()));
                for (size_t t = 0; t < descr.GetMoveFromFactorSource().SourcesSize(); ++t) {
                    items.push_back(TStringBuilder() << "{" << WrapStringBuf(descr.GetMoveFromFactorSource().GetSources(t).GetClassName()) << ", "
                        << WrapStringBuf(descr.GetMoveFromFactorSource().GetSources(t).GetArg()) << "}");
                }
                arrayPrinter.PrintUnordered(cpp, "", "NDetail::TSourcesArgClassNameDataInit::TSourcePair", "SourcesArgClassNameDataInitList" + GetHandle(descr), std::move(items), ToString(descr.GetIndex()));
            }
        }


    }

    void PrintDependentFeaturesFunction(TCodeGenInput& input, TCodegenParams& params) {
        params.Hdr << "struct TDependsOnList {\n"
                << "    THashMap<TString, TVector<TString>> DependentSlices;\n"
                << "};\n\n";

        params.Hdr << R"(
namespace NDetail {
    struct TDependsOnListDataInit: public TDependsOnList {
        struct TDepPair {
            TStringBuf SliceName;
            TStringBuf FeatureName;
        };
        using TType = TDepPair;
    };
} // namespace NDetail

)";
        {
            auto& cpp = params.MethodStream(0);
            cpp << R"(
namespace {
    struct TDependsOnListDataConstructor: public NDetail::TDependsOnListDataInit {
        template <class TStringConstructor>
        TDependsOnListDataConstructor(const TArrayRef<const NDetail::TDependsOnListDataInit::TType> deps, TStringConstructor& toString) {
            TStringBuf previousSliceName;
            TVector<TString>* currentSlicePtr = nullptr;
            for (size_t i = 0; i < deps.size(); ++i) {
                const TStringBuf sliceName = deps[i].SliceName;
                const bool sliceSwitch = (i == 0) || (previousSliceName != sliceName);
                if (sliceSwitch) {
                    previousSliceName = sliceName;
                    currentSlicePtr = &DependentSlices[toString(sliceName)];
                }
                currentSlicePtr->push_back(toString(deps[i].FeatureName));
            }
            for (auto& dep : DependentSlices) {
                dep.second.shrink_to_fit();
            }
        }
    };
} // namespace

)";
        }
        TConstexprArrayPrinter arrayPrinter;
        for (size_t i = 0; i < input.FactorSize(); ++i) {
            const typename TCodeGenInput::TFactorDescriptor& descr = input.GetFactor(i);
            auto& cpp = params.UniformStream(i, input.FactorSize());

            TVector<TString> expandedDeps;
            for (const auto& dependency : descr.GetDependsOn()) {
                const TString sliceName = dependency.GetSlice().empty() ? TSlicesCodegen::GetFactorSliceName(i) : dependency.GetSlice();
                for (const TString& feature : dependency.GetFeature()) {
                    expandedDeps.emplace_back(TStringBuilder() << "{" << WrapStringBuf(sliceName) << ", " << WrapStringBuf(feature) <<"}");
                }
            }
            arrayPrinter.PrintUnordered(cpp, "", "NDetail::TDependsOnListDataInit::TDepPair", "DependsOnListDataInitList" + GetHandle(descr), std::move(expandedDeps), ToString(descr.GetIndex()));
        }
    }

    void PrintExtJsonData(TCodeGenInput& input, TCodegenParams& params) {
        NProtobufJson::TProto2JsonConfig config;
        config.SetEnumMode(NProtobufJson::TProto2JsonConfig::EnumName);

        for (size_t idx = 0; idx != input.FactorSize(); ++idx) {
            const typename TCodeGenInput::TFactorDescriptor& descr = input.GetFactor(idx);
            auto& cpp = params.UniformStream(idx, input.FactorSize());

            const TString extJson = (TCodeGenTraits::UseExtJsonInfo) ? NProtobufJson::Proto2Json(descr, config) : TString{};
            cpp << "static constexpr const char* const FactorExtJson" << GetHandle(descr) << " = \"" << EscapeC(extJson) << "\";\n";
        }
    }

    void PrintBitOffsetsData(TCodeGenInput& input, TCodegenParams& params) {
        TConstexprArrayPrinter arrayPrinter;
        for (size_t idx = 0; idx != input.FactorSize(); ++idx) {
            const typename TCodeGenInput::TFactorDescriptor& descr = input.GetFactor(idx);
            auto& cpp = params.UniformStream(idx, input.FactorSize());

            {
                TVector<TString> items;
                for (size_t i = 0; i != descr.GroupSize(); ++i) {
                    items.push_back(TStringBuilder() << "TFactorGroupMask::EBitOffset::" << descr.GetGroup(i));
                }
                arrayPrinter.PrintUnordered(cpp, "", "NDetail::TFactorGroupMaskDataInit::TType", "FactorGroupMaskInitList" + GetHandle(descr), std::move(items), ToString(descr.GetIndex()));
            }

            {
                TVector<TString> items;
                items.push_back(TStringBuilder() << "TFactorMask::EBitOffset::" << GetHandle(descr));
                arrayPrinter.PrintUnordered(cpp, "", "NDetail::TFactorMaskDataInit::TType", "FactorMaskInitList" + GetHandle(descr), std::move(items));
            }
        }
    }

    void PrintFactorInitBundle(TCodeGenInput& input, TCodegenParams& params) {
        for (size_t idx = 0; idx != input.FactorSize(); ++idx) {
            const typename TCodeGenInput::TFactorDescriptor& descr = input.GetFactor(idx);
            auto& cpp = params.UniformStream(idx, input.FactorSize());

            cpp << "\n    // " << descr.GetIndex() << "\n";

            auto getInitListPair = [&descr](const TStringBuf prefix) -> TString {
                return prefix + GetHandle(descr);
            };

            cpp << "    static constexpr NDetail::TFactorInfoInit " << "cid" << GetHandle(descr) << "{\n";
            cpp << "        " << idx << ",\n";
            cpp << "        \"" << descr.GetCppName() << "\",\n";
            cpp << "        \"" << descr.GetName() << "\",\n";
            cpp << "        \"" << TSlicesCodegen::GetFactorSliceName(idx) << "\",\n";
            cpp << "        " << getInitListPair("FactorGroupMaskInitList") << ",\n";
            cpp << "        " << getInitListPair("FactorMaskInitList") << ",\n";
            cpp << "        " << getInitListPair("TagsDataInitList") << ",\n";
            cpp << "        float(" << GetCanonicalValue(descr) << "),\n";
            cpp << "        float(" << GetMinValue(descr) << "),\n";
            cpp << "        float(" << GetMaxValue(descr) << "),\n";
            cpp << "        " << getInitListPair("GroupsDataInitList") << ",\n";
            cpp << "        FactorExtJson" << GetHandle(descr) << ",\n";
            cpp << "        " << getInitListPair("DependsOnListDataInitList") << ",\n";
            if (descr.HasMoveFromFactorSource()) {
                cpp << "        " << getInitListPair("SourcesArgClassNameDataInitList") << ",";
                TString expression = descr.GetMoveFromFactorSource().GetExpression();
                cpp << "        \"" << expression << "\",";
            } else {
                cpp << "        {},\n";
                cpp << "        \"\"\n";
            }
            cpp << "    };\n";
        }
    }

    void FillFactorsID(TCodeGenInput& input, TCodegenParams& params) {
        ForEachCppStream(params, [](auto& cpp) {
            cpp << "namespace { // anonymous\n\n";
        });

        PrintGetTagsFunction(input, params);
        PrintGetGroupsFunction(input, params);
        PrintMoveFromFactorSourceFunction(input, params);
        PrintDependentFeaturesFunction(input, params);
        PrintExtJsonData(input, params);
        PrintBitOffsetsData(input, params);

        ForEachCppStream(params, [](auto& cpp) {
            cpp << "} // anonymous namespace\n\n";
        });

        params.Hdr << "\nenum EFactorId {\n";
        const TString structName = "TFactorInfosGen";

        for (size_t i = 0; i < params.CppParts; ++i) {
            params.Cpp << "extern void " << structName << "DoInit" << i << "(TVector<TFactorInfo>& self);\n";
        }

        params.Cpp << "namespace {struct " << structName << ": public TVector<TFactorInfo> {\n"
                << "    inline " << structName << "() {\n"
                << "        this->reserve(" << input.FactorSize() << ");\n";

        if (params.CppParts) {
            params.Cpp << "        static constexpr void (*doInitFunctions[])(TVector<TFactorInfo>&){\n";
            for (size_t i = 0; i < params.CppParts; ++i) {
                params.Cpp << "            &" << structName << "DoInit" << i << ",\n";
            }
            params.Cpp << "        };\n"
                       << "        for (auto* doInit : doInitFunctions) {\n"
                       << "            (*doInit)(*this);\n"
                       << "        }\n";
        } else {
            params.Cpp << "        " << structName << "DoInit" << "(*this);\n";
        }

        params.Cpp << "    }\n\n";




        //const size_t partCnt = params.CppParts ? (input.FactorSize() / params.CppParts) + 1 : 1;
        for (size_t idx = 0; idx != input.FactorSize(); ++idx) {
            const typename TCodeGenInput::TFactorDescriptor& descr = input.GetFactor(idx);

            size_t lineEnd = 50 + params.Hdr.Size();
            params.Hdr << "    " << descr.GetCppName() << ",";
            do {
                params.Hdr << " ";
            } while (params.Hdr.Size() < lineEnd);
            params.Hdr << "// " << descr.GetIndex();

            if (GetComment(descr))
                params.Hdr << " [" << GetComment(descr) << ']';

            params.Hdr << "\n";
        }


        if (params.CppParts) {
            for (size_t i = 0; i < params.CppParts; ++i) {
                params.CppStream(i) << "void " << structName << "DoInit" << i << "(TVector<TFactorInfo>& self) {\n";
            }
        } else {
            params.Cpp << "void " << structName << "DoInit" << "(TVector<TFactorInfo>& self) {\n";
        }

        PrintFactorInitBundle(input, params);

        ForEachCppStream(params, [](auto& cpp) {
            cpp << "\n\n";
            cpp << "    static constexpr const NDetail::TFactorInfoInit* const localInitData[] = {\n";
        });
        for (size_t idx = 0; idx != input.FactorSize(); ++idx) {
            const typename TCodeGenInput::TFactorDescriptor& descr = input.GetFactor(idx);
            auto& cpp = params.UniformStream(idx, input.FactorSize());
            cpp << "        &cid" << GetHandle(descr) << ",\n";
        }
        ForEachCppStream(params, [](auto& cpp) {
            cpp << "        nullptr\n";
            cpp << "    };\n\n"; // localInitData
        });

        ForEachCppStream(params, [](auto& cpp) {
            cpp << "\n"
                << "    NDetail::ConstructAndPushFactorInfos(self, localInitData);\n\n";
        });

        ForEachCppStream(params, [](auto& cpp) {
            cpp << "}\n\n"; // DoInit*
        });

        params.Hdr << "\n    FI_FACTOR_COUNT\n"
                << "};\n";

        params.Cpp << "};}\n\n#define pszFacNames (*Singleton<" << structName << ">())\n\n"
                << "const TFactorInfo* GetFactorsInfo() {\n"
                << "    return &pszFacNames[0];\n"
                << "}\n\n";
    }

    void FillFactorsMask(TCodeGenInput& input, TCodegenParams& params) {
        TVector<TString> factors(Reserve(input.FactorSize()));
        TVector<TString> handles(Reserve(input.FactorSize()));
        for (size_t idx = 0; idx != input.FactorSize(); ++idx) {
            factors.push_back(input.GetFactor(idx).GetName());
            handles.push_back(GetHandle(input.GetFactor(idx)));
        }
        GenMask(params, "TFactorMask", factors, &handles);
    }

    void FillAntiSEO(TCodeGenInput& input, TCodegenParams& params) {
        bool hasHeader = false;
        for (size_t idx = 0; idx != input.FactorSize(); ++idx) {
            const typename TCodeGenInput::TFactorDescriptor& descr = input.GetFactor(idx);
            if (!descr.HasAntiSeoUpperBound())
                continue;
            if (!hasHeader) {
                params.Cpp << "struct TAntiSEOFactorInfo {\n"
                           << "    EFactorId FactorID;\n"
                           << "    float   UpperBound;\n"
                           << "};\n\n"
                           << "static Y_DECLARE_UNUSED const TAntiSEOFactorInfo AntiSEOFactors[] = {\n";
                hasHeader = true;
            }
            params.Cpp << "    { " << descr.GetCppName() << ", ";
            const float upperBound = descr.GetAntiSeoUpperBound();
            if (upperBound == 1)
                params.Cpp << "1.0";
            else
                params.Cpp << upperBound;
            params.Cpp << "f },\n";
        }
        if (hasHeader)
            params.Cpp << "};\n";
    }

    void FillFactorGroupMask(TCodeGenInput& input, TCodegenParams& params) {
        if (input.GroupSize() > 0) {
            params.Hdr << "\nstruct TCombinedFactorMask {\n"
                       << "    TFactorGroupMask Group;\n"
                       << "    bool AllFactors;\n"
                       << "    TFactorMask Factor;\n"
                       << "    TCombinedFactorMask(bool setAll)\n"
                       << "        : Group(setAll)\n"
                       << "        , AllFactors(setAll)\n"
                       << "    {}\n"
                       << "};\n";

            params.Hdr << "\nvoid FillFactorGroupMask(TCombinedFactorMask& mask, TVector<TString>& factorNames, const IFactorsInfo& info);\n";

            params.Cpp << "\nvoid FillFactorGroupMask(TCombinedFactorMask& mask, TVector<TString>& factorNames, const IFactorsInfo& info) {\n"
                       << "    size_t factorId;\n"
                       << "    for (size_t i = 0 ; i < factorNames.size() ; ++i) {\n"
                       << "        if (!info.GetFactorIndex(factorNames[i].data(), &factorId))\n"
                       << "            ythrow yexception() << \"unknown factor name: \" << factorNames[i];\n"
                       << "        mask.Group |= pszFacNames[factorId].GroupMask;\n"
                       << "        mask.Factor |= pszFacNames[factorId].FactorMask;\n"
                       << "    }\n"
                       << "};\n";
        }
    }

    void CheckFactorRequiredFields(TCodeGenInput& input);

    void CheckTagsRestrictions(TCodeGenInput& /*input*/) {
        // By default, doing nothing
    }

    void CutUnwantedSlices(TCodeGenInput& /*input*/, TCodegenParams& /*params*/) {
        // By default, doing nothing.
    }

    void PreprocessInput(TCodeGenInput& /*input*/) {
        // By default, doing nothing.
    }

    bool NeedOffsetGators(TCodeGenInput& /*input*/) const {
        return false;
    }

    void GenOffsetGators(TCodeGenInput& /*input*/, TStringStream& /*protectedFactorStorage*/) {
        // By default, doing nothing.
    }

    void FillFactorInfo(TCodegenParams& params) {
        params.Hdr << "\nstruct TFactorInfo {\n    ";
        EnumDescr2Cpp(params.Hdr, GetTagDescriptor());
        params.Hdr << "\n";
        params.Hdr << "    const int Index;\n"
                << "    const char* InternalName;\n"
                << "    const char* Name;\n"
                << "    const char* SliceName;\n"
                << "    TFactorGroupMask GroupMask;\n"
                << "    TFactorMask FactorMask;\n"
                << "    TTagsData TagsData;\n"
                << "    float CanonicalValue;\n"
                << "    float MinValue;\n"
                << "    float MaxValue;\n"
                << "    TGroupsData GroupsData;\n"
                << "    const char* ExtJson;\n"
                << "    TDependsOnList DependsOn;\n"
                << "    TMoveFromFactorSource MoveFromFactorSource;\n"
                << "};\n\n"
                << "const TFactorInfo* GetFactorsInfo();\n\n"
                << "// Body of this function is not auto-generated. Add manually, where needed\n"
                << "TAutoPtr<IFactorsInfo> GetFactorsInfoIf(size_t begin, size_t end);\n";
    }

    void FillFactorInfoConstructor(TCodegenParams& params) {
        params.Hdr << R"(
namespace NDetail {
        struct TFactorInfoInit {
            const int Index;
            const char* InternalName;
            const char* Name;
            const char* SliceName;
            const TArrayRef<const TFactorGroupMaskDataInit::TType> GroupMask;
            const TArrayRef<const TFactorMaskDataInit::TType> FactorMask;
            const TArrayRef<const TTagsDataInit::TType> TagsData;
            const float CanonicalValue;
            const float MinValue;
            const float MaxValue;
            const TArrayRef<const TGroupsDataInit::TType> GroupsData;
            const char* ExtJson;
            const TArrayRef<const TDependsOnListDataInit::TType> DependsOn;
            const TArrayRef<const TSourcesArgClassNameDataInit::TType> SourcesArgClassNameData;
            const char* Method;
        };

        void ConstructAndPushFactorInfos(TVector<TFactorInfo>& target, const TFactorInfoInit* const initData[]);

    } // namespace NDetail

)";
        {
            auto& cpp = params.MethodStream(0);
            cpp << R"(
namespace {
    class TCopyOnWriteStringIntern {
        THashSet<TString> Strings;
    public:
        template <class T>
        const TString& operator()(T&& str) {
            return *(Strings.emplace(std::forward<T>(str)).first);
        }
    };

    template <class TStringConstructor>
    static TFactorInfo ConstructFactorInfo(const NDetail::TFactorInfoInit& init, TStringConstructor& toString) {
        return TFactorInfo{
            init.Index,
            init.InternalName,
            init.Name,
            init.SliceName,
            TFactorGroupMaskDataConstructor{init.GroupMask},
            TFactorMaskDataConstructor{init.FactorMask},
            TTagsDataConstructor{init.TagsData, toString},
            init.CanonicalValue,
            init.MinValue,
            init.MaxValue,
            TGroupsDataConstructor{init.GroupsData, toString},
            init.ExtJson,
            TDependsOnListDataConstructor{init.DependsOn, toString},
            TMoveFromFactorSource{TSourcesArgClassNameDataConstructor{init.SourcesArgClassNameData, toString}, init.Method}
        };
    }
} // namespace

namespace NDetail {
    void ConstructAndPushFactorInfos(TVector<TFactorInfo>& target, const TFactorInfoInit* const initData[]) {
        Y_ENSURE(initData != nullptr);
        TCopyOnWriteStringIntern strings;
        for (ssize_t i = 0; ; ++i) {
            const TFactorInfoInit* factorInfoInitPtr = initData[i];
            if (!factorInfoInitPtr) {
                break;
            }
            target.push_back(ConstructFactorInfo(*factorInfoInitPtr, strings));
        }
    }
} // namespace NDetail

)";

        }
    }

    float GetCanonicalValue(const typename TCodeGenInput::TFactorDescriptor& descr) const {
        return descr.HasCanonicalValue() ? descr.GetCanonicalValue() : 0.0f;
    }
    float GetMinValue(const typename TCodeGenInput::TFactorDescriptor& descr) const {
        const auto* protoDescriptor = descr.GetDescriptor()->FindFieldByName("MinValue");
        return (descr.HasMinValue() || protoDescriptor->has_default_value()) ? descr.GetMinValue() : std::numeric_limits<float>::lowest();

    }
    float GetMaxValue(const typename TCodeGenInput::TFactorDescriptor& descr) const {
        const auto* protoDescriptor = descr.GetDescriptor()->FindFieldByName("MaxValue");
        return (descr.HasMaxValue() || protoDescriptor->has_default_value()) ? descr.GetMaxValue() : std::numeric_limits<float>::max();
    }

    TString GetTagName(const typename TCodeGenInput::TFactorDescriptor& descr, size_t t) const;
    TString GetComment(const typename TCodeGenInput::TFactorDescriptor& descr) const;
    const Ngp::EnumDescriptor& GetTagDescriptor() const;
private:
    void GetNamespace(TCodegenParams& params, TVector<TStringBuf>& result ) {
        for(int i = 0; i < params.ArgcRest; ++i ) {
            result.push_back(params.ArgvRest[i]);
        }
    }

    TString GetNamespacePrefix(const TVector<TStringBuf>& namespaces) {
        TString result = "::";
        for(size_t i = 0; i < namespaces.size(); ++i ) {
            if (i > 0)
                result += "::";
            result += namespaces[i];
        }
        return result;
    }

    void EnumDescr2Cpp(IOutputStream& out, const Ngp::EnumDescriptor& descr) {
        out << "enum " << descr.name() << " {";
        for (int i = 0; i < descr.value_count(); i++) {
            if (i > 0)
                out << ", ";
            out << descr.value(i)->name() << " = " << descr.value(i)->number();
        }
        out << "};";
    }

    void CheckFactorIndexes(TCodeGenInput& input) {
        for (size_t i = 0; i != input.FactorSize(); ++i) {
            const typename TCodeGenInput::TFactorDescriptor& descr = input.GetFactor(i);
            if (descr.GetIndex() != i)
                ythrow TGenError() << "Incorrect index for factor " << descr.GetName() << ": should be " << i
                                   << " instead of " << descr.GetIndex();
        }
    }

    void GenMask(TCodegenParams& params, const TString& structName, const TVector<TString>& fields, const TVector<TString>* internalFieldNames = nullptr) {
        if (internalFieldNames == nullptr) {
            internalFieldNames = &fields;
        }
        Y_ENSURE(internalFieldNames);
        Y_ENSURE(fields.size() == internalFieldNames->size(), "Array with custom names has invalid size");

        const TString baseType = "std::bitset<" + ToString(fields.size()) + ">";
        params.Hdr << "struct " << structName << ": protected " << baseType << " {\n"
                   << "    using TBase = " << baseType << ";\n"
                   << "    using TBitReference = TBase::reference;\n\n"
                   << "    using TBase::size;\n"
                   << "    using TBase::test;\n\n";
        params.Hdr << "    " << structName << "(bool setAll=false) {\n"
                   << "         if (setAll) {\n"
                   << "             TBase::set();\n"
                   << "         }\n"
                   << "    }\n\n";

        params.Hdr << "    enum class EBitOffset: ui32 {\n";
        for (size_t i = 0; i < fields.size(); ++i) {
            params.Hdr << "        " << internalFieldNames->at(i) << " = " << i << ",\n";
        }
        params.Hdr << "    };\n\n";

        for (size_t i = 0; i < fields.size(); ++i) {
            const TString& intName = internalFieldNames->at(i);
            const TString& extName = fields.at(i);
            params.Hdr << "    Y_FORCE_INLINE bool " << extName << "() const noexcept { return (*this)[static_cast<size_t>(EBitOffset::" << intName << ")]; }\n";
            params.Hdr << "    Y_FORCE_INLINE TBitReference " << extName << "() noexcept { return (*this)[static_cast<size_t>(EBitOffset::" << intName << ")]; }\n\n";
        }

        params.Hdr << "\n";

        params.Hdr << "\n    " << structName << "& operator|=(const " << structName << "& another);\n";

        {
            auto& cpp = params.MethodStream(2);

            cpp << "\n    " << structName << "& " << structName << "::operator|=(const " << structName << "& another) {\n";
            cpp << "        TBase::operator|=(another);\n";
            cpp << "        return *this;\n"
                << "    }\n";
        }

        params.Hdr << "\n    bool operator&&(const " << structName << "& another) const;\n";

        {
            auto& cpp = params.MethodStream(1);
            cpp << "\n    bool " << structName << "::operator&&(const " << structName << "& another) const {\n";
            const size_t shortCircuitThreshold = 384;
            if (fields.size() <= shortCircuitThreshold) {
                cpp << "        return (*this & another).any();\n";
            } else {
                cpp << "        for (size_t i = 0; i < this->size(); ++i) {\n"
                    << "            if (this->test(i) & another.test(i)) {\n"
                    << "                return true;\n"
                    << "            }\n"
                    << "        }\n";
                cpp << "        return false;\n";
            }
            cpp << "    }\n";
        }

        params.Hdr << "\n    " << structName << "& InvertConsistedIn(const " << structName << "& another);\n";

        {
            auto& cpp = params.MethodStream(3);

            cpp << "\n    " << structName << "& " << structName << "::InvertConsistedIn(const " << structName << "& another) {\n";
            cpp << "        TBase::operator^=(another);\n";
            cpp << "        return *this;\n"
                << "    }\n";
        }

        params.Hdr << "\n    " << structName << "& ZeroConsistedIn(const " << structName << "& another);\n";

        {
            auto& cpp = params.MethodStream(3);

            cpp << "\n    " << structName << "& " << structName << "::ZeroConsistedIn(const " << structName << "& another) {\n";
            cpp << "        TBase::operator&=(~another);\n";
            cpp << "        return *this;\n"
                << "    }\n";
        }

        params.Hdr << "};\n\n";

        if (!fields.empty()) {
            params.Hdr << "IOutputStream &operator <<(IOutputStream& stream, const " << structName << "& bm);\n";

            {
                auto& cpp = params.MethodStream(4);

                cpp << "IOutputStream &operator <<(IOutputStream& stream, const " << structName << "& bm) {\n";
                cpp << "    static constexpr std::array<TStringBuf, " << fields.size() << "> fieldNames{{\n";
                for (TVector<TString>::const_iterator it = fields.begin() ; it != fields.end() ; ++it) {
                    cpp << "        " << WrapStringBuf(*it) << ",\n";
                }
                cpp << "    }};\n";
                cpp << R"(
    Y_VERIFY_DEBUG(bm.size() == fieldNames.size(), "inconsistent number of fields");
    for (size_t i = 0; i < bm.size(); ++i) {
        stream << fieldNames[i] << ": " << static_cast<ui32>(bm.test(i)) << "\n";
    }
)";
                cpp << "    stream.Flush();\n";
                cpp << "    return stream;\n}\n\n";
            }
        } else {
            params.Hdr << "inline IOutputStream &operator <<(IOutputStream& stream, const " << structName << "&) {\n";
            params.Hdr << "    stream << \"<Empty mask>\" << Endl;\n";
            params.Hdr << "    return stream;\n}\n\n";
        }

        params.Hdr << "namespace NDetail {\n"
            << "    struct " << structName << "DataInit: public " << structName << " {\n"
            << "        using TType = EBitOffset;\n"
            << "    };\n"
            << "} // namespace NDetail\n\n";

        {
            auto& cpp = params.MethodStream(0);

            cpp << "namespace {\n"
                << "    struct " << structName << "DataConstructor: public NDetail::" << structName << "DataInit {\n"
                << "        " << structName << "DataConstructor(const TArrayRef<const EBitOffset> indices) {\n"
                << "            for (const EBitOffset index : indices) {\n"
                << "                this->set(static_cast<size_t>(index), true);\n"
                << "            }\n"
                << "        }\n"
                << "    };\n"
                << "} // namespace\n\n";
        }
    }

    void GenFactorsAccessor(
        TCodeGenInput& input,
        TCodegenParams& params,
        TString groupName,
        bool filterByGroup,
        TStringStream& protectedFactorStorage)
    {
        TStringStream structName;
        structName << "T" << groupName << "FactorsAccessor";
        params.Hdr << "struct " << structName.Str() << " : private TNonCopyable {\n";

        size_t prevFactor = -1;
        for (size_t idx = 0; idx != input.FactorSize(); ++idx) {
            const typename TCodeGenInput::TFactorDescriptor& descr = input.GetFactor(idx);

            if (filterByGroup) {
                bool skip = true;
                for (size_t i = 0; i != descr.GroupSize(); ++i) {
                    if (descr.GetGroup(i).compare(groupName) == 0)
                        skip = false;
                }
                if (skip)
                    continue;
            }

            size_t holeSize = idx - prevFactor - 1;

            if (holeSize > 0) {
                params.Hdr << "    char _fake_name_instead_of_factors_"
                    << prevFactor + 1 << "_" << idx - 1
                    << "[" << holeSize * sizeof(float) << "];\n";
            }

            params.Hdr << "    float " << descr.GetName() << ";\n";

            prevFactor = idx;
        }

        params.Hdr << "};" << "\n";

        protectedFactorStorage
            << "    " << structName.Str() << "& Get" << groupName << "Group() {\n"
            << "        return *(" << structName.Str() << "*)Factor;\n"
            << "    }\n";
    }

    void GenOffsetGator(
        TString offsetName,
        size_t offsetIndex,
        TStringStream& protectedFactorStorage)
    {
        protectedFactorStorage
            << "    size_t Get" << offsetName << "Offset() {\n"
            << "        return " << offsetIndex << ";\n"
            << "    }\n"
            << "    float* GetRaw" << offsetName << "() {\n"
            << "        return Factor + " << offsetIndex << ";\n"
            << "    }\n";
    }

private:

    class TConstexprArrayPrinter {
    public:
        TConstexprArrayPrinter(const bool deduplicate = true)  {
            if (deduplicate) {
                ContentTracker.ConstructInPlace();
            }
        }

        void PrintUnordered(TStringStream& cpp, const TStringBuf indent, const TString& type, const TString& name, TVector<TString> items, const TStringBuf comment = {}) {
            if (ContentTracker) {
                Sort(items); // sort unordered set for better cache hit ratio
            }
            return Print(cpp, indent, type, name, std::move(items), comment);
        }

        void Print(TStringStream& cpp, const TStringBuf indent, const TString& type, const TString& name, const TVector<TString>& items, const TStringBuf comment = {}) {
            if (comment) {
                cpp << indent << "// " << comment << "\n";
            }

            if (items.empty()) { // avoid zero size arrays
                cpp << indent << "static constexpr const TArrayRef<const " << type << "> " << name << ";\n";
            } else {
                const TDupKey cacheKey{type, &cpp, items};
                const TMaybe<TString> previonsInstance = CheckDuplicate(cacheKey, name);

                if (previonsInstance) {
                    cpp << indent << "static constexpr const TArrayRef<const " << type << ">& " << name << " = " << *previonsInstance << ";\n";
                    cpp << indent << "/* duplicated content\n"; // start of multiline comment
                }

                cpp << indent << "static constexpr const " << type << " " << name << "Payload_[" << items.size() << "] = {\n";
                for (const TString& it : items) {
                    cpp << indent << "    " << it << ",\n";
                }
                cpp << indent << "};\n";
                cpp << indent << "static constexpr const TArrayRef<const " << type << "> " << name << "{" << name << "Payload_" << "};\n";

                if (previonsInstance) {
                    cpp << indent << "*/\n"; // end of multiline comment
                }
            }

            cpp << "\n";
        }

    private:
        struct TDupKey {
            TString Type;
            TStringStream* StreamPtr;
            TVector<TString> Content;

            bool operator==(const TDupKey& other) const {
                return Type == other.Type
                    && StreamPtr == other.StreamPtr
                    && Content == other.Content;
            }
        };

        struct TDupKeyHash {
            size_t operator()(const TDupKey& k) const {
                return MultiHash(k.Type, k.StreamPtr, TSimpleRangeHash()(k.Content));
            }
        };

        TMaybe<TString> CheckDuplicate(const TDupKey& key, const TString& name) {
            if (ContentTracker) {
                const auto itPair = ContentTracker->emplace(key, name);
                if (!itPair.second) {
                    return itPair.first->second;
                }
            }
            return Nothing();
        }
    private:

        TMaybe<THashMap<TDupKey, TString, TDupKeyHash>> ContentTracker; // Maps content to variable name
    };
};
