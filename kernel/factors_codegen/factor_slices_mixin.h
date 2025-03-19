#pragma once

#include <kernel/factor_slices/factor_slices.h>
#include <kernel/factor_slices/factors_source.h>
#include <kernel/proto_codegen/codegen.h>

#include <util/string/ascii.h>
#include <util/string/split.h>
#include <util/generic/hash.h>

template <class TCodeGenInput>
class TNullFactorSlicesCodegen {
public:
    void PrepareSliceDescriptors(TCodeGenInput& /*input*/) {
        // By default, doing nothing.
    }
    void FillSlicesInitializer(const TCodeGenInput& /*input*/, TCodegenParams& /*params*/) {
        // By default, doing nothing.
    }
    void FillSliceFactorIdEnums(TCodeGenInput& /*input*/, TCodegenParams& /*params*/, const TStringBuf& /*namespacePrefix*/) {
        // By default, doing nothing.
    }
    void UseSliceNamespacesInGlobalNamespace(TCodeGenInput& /*input*/, TCodegenParams& /*params*/, const TStringBuf& /*namespacePrefix*/) {
        // By default, doing nothing.
    }
    const char* GetFactorSliceName(size_t /*index*/) {
        return "";
    }
};

template <class TCodeGenInput>
class TFactorSlicesCodegen {
public:
    void PreprocessGeneratedSlices(TCodeGenInput& input) {
        for (size_t sliceIndex = 0; sliceIndex != input.SliceSize(); ++sliceIndex) {
            auto& sliceDescr = *input.MutableSlice(sliceIndex);
            if (sliceDescr.GetGenerate()) {
                auto slice = FromString<NFactorSlices::EFactorSlice>(sliceDescr.GetName());

                Y_ENSURE(sliceDescr.FactorSize() == 0,
                    "explicit declaration of factors is not allowed in generated slice " <<  slice);

                GetFactorsSource<TCodeGenInput>(slice).UpdateCodegen(sliceDescr, 0);
            }
        }
    }

    void PreprocessCustomSlices(TCodeGenInput&) {
        // Specialize to handle special cases
    }

    void PrepareSliceDescriptors(TCodeGenInput& input) {
        using namespace NFactorSlices;

        if (input.SliceSize() == 0) {
            return;
        }

        PreprocessGeneratedSlices(input);
        PreprocessCustomSlices(input);

        ui32 nextIndex = 0;
        for (size_t i = 0; i < input.FactorSize(); ++i) {
            nextIndex = Max<ui32>(nextIndex, input.GetFactor(i).GetIndex() + 1);
        }

        NFactorSlices::TSliceMap<bool> inputSlices(false);
        for (size_t i = 0; i != input.SliceSize(); ++i) {
            Y_ENSURE(!inputSlices[input.GetSlice(i).GetName()],
                "second declaration for slice " << input.GetSlice(i).GetName());
            inputSlices[input.GetSlice(i).GetName()] = true;
        }

        size_t prevSliceIndex = Max<size_t>();
        for (auto slice : GetAllFactorSlices()) {
            if (!inputSlices[slice]) {
                continue;
            }

            Y_ENSURE(!GetStaticSliceInfo(slice).Hierarchical,
                "slice " << slice << " is hierarchical and can only contain other slices"
                << ", as specified in kernel/factor_slices/factor_slices_gen.in");

            size_t sliceBeginIndex = nextIndex;

            for (size_t sliceIndex = 0; sliceIndex != input.SliceSize(); ++sliceIndex) {
                auto& sliceDescr = *input.MutableSlice(sliceIndex);
                if (sliceDescr.GetName() == ToString(slice)) {
                    Y_ENSURE((sliceIndex == 0 && prevSliceIndex == Max<size_t>()) ||
                        sliceIndex > prevSliceIndex,
                        "slice " << slice << " is declared out of order"
                        << ", as specified in kernel/factor_slices/factor_slices_gen.in");

                    size_t sliceSize = 0;

                    for (size_t factorIndex = 0; factorIndex != sliceDescr.FactorSize(); ++factorIndex) {
                        const auto& sliceFactor = sliceDescr.GetFactor(factorIndex);
                        auto& factor = *input.AddFactor();
                        factor.CopyFrom(sliceFactor);
                        factor.SetIndex(sliceBeginIndex + sliceFactor.GetIndex());
                        sliceSize = Max<size_t>(sliceSize, sliceFactor.GetIndex() + 1);
                    }

                    nextIndex = sliceBeginIndex + sliceSize;

                    for (size_t offsetIndex = 0; offsetIndex != sliceDescr.OffsetSize(); ++offsetIndex) {
                        const auto& sliceOffset = sliceDescr.GetOffset(offsetIndex);

                        Y_ENSURE(sliceOffset.GetIndex() <= sliceSize,
                            "incorrect offset " << sliceOffset.GetName()
                            <<  " in slice " << slice);

                        auto& offset = *input.AddOffset();
                        offset.CopyFrom(sliceOffset);
                        offset.SetIndex(sliceBeginIndex + sliceOffset.GetIndex());
                    }

                    InitSliceData(slice, sliceBeginIndex, nextIndex, sliceDescr);
                    prevSliceIndex = sliceIndex;
                    break;
                }
            }
        }
    }

    void FillSlicesInitializer(const TCodeGenInput& input, TCodegenParams& params) {
        params.Hdr << "\n"
            << "const IFactorsInfo* GetSliceFactorsInfo(::NFactorSlices::EFactorSlice slice);" "\n";

        if (input.SliceSize() == 0) {
            params.Cpp << "\n"
                << "const IFactorsInfo* GetSliceFactorsInfo(::NFactorSlices::EFactorSlice) {" "\n"
                << "    return nullptr;" "\n"
                << "}" "\n";
            return;
        }

        params.Cpp << "\n"
            << "namespace {" "\n"
            << "    class TStaticInfosMap : public ::NFactorSlices::TSliceMap<TAutoPtr<IFactorsInfo>> {};" "\n"
            << "}" "\n"
            << "\n"
            << "const IFactorsInfo* GetSliceFactorsInfo(::NFactorSlices::EFactorSlice slice) {" "\n"
            << "    TStaticInfosMap& infos = *Singleton<TStaticInfosMap>();" "\n"
            << "    auto& sliceInfo = infos[slice];" "\n"
            << "    if (!!sliceInfo)" "\n"
            << "        return sliceInfo.Get();" "\n"
            << "    switch (slice) {" "\n";

        for (size_t i = 0; i != input.SliceSize(); ++i) {
            const auto& sliceDescr = input.GetSlice(i);
            const auto& sliceData = SliceData[sliceDescr.GetName()];

            TString namespaceName = sliceData.NamespaceName;
            params.Cpp
                << "        case " << sliceData.CppName << ": {" "\n"
                << "            using namespace " << namespaceName << ";" "\n"
                << "            return (sliceInfo = GetFactorsInfoIf(FIRST_FACTOR_INDEX, "
                    << "FIRST_FACTOR_INDEX + " << namespaceName << "::FI_FACTOR_COUNT)).Get();" "\n"
                << "        }" "\n";
        }

        params.Cpp
            << "        default: {" "\n"
            << "            return nullptr;" "\n"
            << "        }" "\n"
            << "    }" "\n"
            << "}" "\n";

        params.Cpp << "\n"
            << "static void InitFactorSlices() {" "\n"
            << "    auto& metaInfo = NFactorSlices::TGlobalSlicesMetaInfo::Instance();" "\n";

        for (size_t i = 0; i != input.SliceSize(); ++i) {
            const auto& sliceDescr = input.GetSlice(i);
            const auto& sliceData = SliceData[sliceDescr.GetName()];

            Y_ASSERT(sliceData.BeginIndex <= sliceData.EndIndex);

            TString cppName = sliceData.CppName;
            TString namespaceName = sliceData.NamespaceName;
            params.Cpp
                << "    // Initialize factor slice \"" << sliceDescr.GetName() << "\"" "\n"
                << "    {" "\n"
                << "        const IFactorsInfo* sliceInfo = GetSliceFactorsInfo(" << cppName << ");" "\n"
                << "        Y_ASSERT(!!sliceInfo);" "\n"
                << "        metaInfo.SetFactorsInfo(" << cppName << ", sliceInfo);" "\n"
                << "        metaInfo.SetNumFactors(" << cppName << ", "
                    << namespaceName << "::FI_FACTOR_COUNT" << ");" "\n";

            if (sliceDescr.GetEnable()) {
                params.Cpp
                    << "        metaInfo.SetSliceEnabled(" << cppName << ", true);" "\n";
            }

            params.Cpp
                << "    }" "\n";
        }

        params.Cpp
            << "}" "\n";

        params.Cpp << "\n"
            << "namespace {" "\n"
            << "    class TSlicesInit {" "\n"
            << "    public:" "\n"
            << "        TSlicesInit() {" "\n"
            << "            InitFactorSlices();" "\n"
            << "        }" "\n"
            << "    };" "\n"
            << "    static TSlicesInit slicesInit;" "\n"
            << "}" "\n";
    }

    void UseSliceNamespacesInGlobalNamespace(TCodeGenInput& input, TCodegenParams& params, const TStringBuf& namespacePrefix) {
        if (namespacePrefix == "::")
            return;

        for (size_t i = 0; i != input.SliceSize(); ++i) {
            const auto& sliceDescr = input.GetSlice(i);
            const auto& sliceData = SliceData[sliceDescr.GetName()];
            TString namespaceName = sliceData.NamespaceName;
            params.Hdr << "\n"
                       << "namespace " << sliceData.NamespaceName << "{" "\n"
                       << "    using namespace  " << namespacePrefix << "::" << sliceData.NamespaceName << ";\n"
                       << "} \n";
        }
    }

    void FillSliceFactorIdEnums(TCodeGenInput& input, TCodegenParams& params,
        const TStringBuf& namespacePrefix)
    {
        for (size_t i = 0; i != input.SliceSize(); ++i) {
            const auto& sliceDescr = input.GetSlice(i);
            const auto& sliceData = SliceData[sliceDescr.GetName()];
            size_t numFactors = sliceData.EndIndex - sliceData.BeginIndex;

            TString namespaceName = sliceData.NamespaceName;
            params.Hdr << "\n"
                << "namespace " << namespaceName << "{" "\n"
                << "    const size_t FIRST_FACTOR_INDEX = " << sliceData.BeginIndex << ";" "\n"
                << "\n";

            if (input.FactorSize() == numFactors && namespacePrefix != "::") {
                params.Hdr
                    << "    using namespace " << namespacePrefix << ";" "\n"
                    << "}" "\n";
                continue;
            }

            params.Hdr
                << "    enum EFactorId {\n";

            for (size_t factorIndex = 0; factorIndex != sliceDescr.FactorSize(); ++factorIndex) {
                const auto& factor = sliceDescr.GetFactor(factorIndex);

                size_t lineEnd = 50 + params.Hdr.Size();
                params.Hdr << "        " << factor.GetCppName() << ",";
                do {
                    params.Hdr << " ";
                } while (params.Hdr.Size() < lineEnd);
                params.Hdr << "// " << factor.GetIndex();

                params.Hdr << "\n";
            }

            if (numFactors > 0) {
                params.Hdr << "\n";
            }

            params.Hdr
                << "        FI_FACTOR_COUNT = " <<  numFactors <<  "\n"
                << "    };" "\n"
                << "}" "\n";
        }
    }

    const char* GetFactorSliceName(size_t index) {
        for (const auto& data : SliceData) {
            if (data.second.BeginIndex <= index && index < data.second.EndIndex) {
                return data.first.data();
            }
        }
        return "";
    }

private:
    template <class SliceDescr>
    void InitSliceData(NFactorSlices::EFactorSlice slice, ui32 beginIndex, ui32 endIndex
        , const SliceDescr& descr)
    {
        TSliceData& data = SliceData[descr.GetName()];
        data.CppName = ToCppString(slice);
        data.NamespaceName = GetSliceNamespaceName(descr.GetName());
        data.BeginIndex = beginIndex;
        data.EndIndex = endIndex;
    }

    TString GetSliceNamespaceName(const TString& sliceName) {
        TString name = "NSlice";

        for (const auto& it : StringSplitter(sliceName).Split('_')) {
            TString part = TString(it.Token());
            if (part) {
                char firstChar = AsciiToUpper(part[0]);
                name.append(firstChar);
                name += part.substr(1);
            }
        }

        return name;
    }

private:
    struct TSliceData {
        TString CppName;
        TString NamespaceName;
        size_t BeginIndex = 0;
        size_t EndIndex = 0;
    };

    THashMap<TString, TSliceData> SliceData;
};

template <class TCodeGenInput, class TCodeGenTraits>
using TFactorSlicesMixin
    = std::conditional_t<TCodeGenTraits::UseFactorSlices,
         TFactorSlicesCodegen<TCodeGenInput>,
         TNullFactorSlicesCodegen<TCodeGenInput>>;
