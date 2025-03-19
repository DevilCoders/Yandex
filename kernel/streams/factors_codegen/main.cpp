// Generator of kernel/streams/onotole_factors.cpp

#include <kernel/streams/metadata/factors_metadata.pb.h>

#include <kernel/proto_codegen/codegen.h>

#include <util/generic/algorithm.h>
#include <util/generic/yexception.h>

inline
bool FactorDescriptorsLess(const NStreams::TCodeGenInput::TFactorDescriptor& x, const NStreams::TCodeGenInput::TFactorDescriptor& y)
{
    return (x.GetLevel() < y.GetLevel());
}

void GenCode(const NStreams::TCodeGenInput& input, TCodegenParams& params)
{
    TVector<NStreams::TCodeGenInput::TFactorDescriptor> descriptors;
    descriptors.reserve(input.FactorSize());

    for (size_t i = 0; i < input.FactorSize(); ++i) {
        descriptors.push_back(input.GetFactor(i));
    }

    StableSort(descriptors.begin(), descriptors.end(), FactorDescriptorsLess);

    if (descriptors.empty()) {
        ythrow yexception() << "No onotole factors descriptors found!" << Endl;
    }

    params.Hdr
        << "#pragma once" << "\n\n"
        << "#include <kernel/generated_factors_info/metadata/factors_metadata.pb.h>" << "\n\n"
        << "#include <kernel/u_tracker/u_tracker.h>" << "\n\n"
        << "TVector<TString> GetStreamFactorNames(int level);" << "\n\n"
        << "ui32 GetStreamFactorsSize(int level);" << "\n\n"
        << "void CalculateStreamFactors(const TUTracker& uTracker, float* dstFeatures, int level);" << "\n\n"
        << "\n";

    params.Cpp
        << "#include \"" << params.HeaderFileName << "\"\n\n"
        << "#include <util/charset/unidata.h>" << "\n\n";

    // Phase 1. GetStreamFactorsSize
    {
        params.Cpp
            << "ui32 GetStreamFactorsSize(int level)" << "\n"
            << "{" << "\n";

        int lastLevel = Min<int>();
        ui32 countFeatures = 0;

        for (const auto& d : descriptors) {
            if (d.GetLevel() != lastLevel) {
                params.Cpp
                    << "    if (level < " << d.GetLevel() << ") {" << "\n"
                    << "        return " << countFeatures << ";" << "\n"
                    << "    }" << "\n";
                lastLevel = d.GetLevel();
            }
            ++countFeatures;
        }

        params.Cpp
            << "    return " << countFeatures << ";" << "\n"
            << "}" << "\n";
    }

    params.Cpp << "\n";


    // Phase 3. CalculateStreamFactors
    {
        params.Cpp
            << "void CalculateStreamFactors(const TUTracker& uTracker, float* dstFeatures, int level)" << "\n"
            << "{" << "\n";

        size_t offset = 0;
        int lastLevel = Min<int>();

        for (const auto& d : descriptors) {
            if (d.GetLevel() != lastLevel) {
                params.Cpp
                    << "    if (level < " << d.GetLevel() << ") {" << "\n"
                    << "        return;" << "\n"
                    << "    }" << "\n";
                lastLevel = d.GetLevel();
            }
            params.Cpp
                << "    dstFeatures[" << offset << "] = uTracker." << d.GetCppCode() << ";" << "\n";
            ++offset;
        }

        params.Cpp
            << "    return;" << "\n"
            << "}" << "\n";
    }

    params.Cpp << "\n";

    // Phase 4. GenerateCppName
    {
        params.Cpp
            << "inline" << "\n"
            << "TString GenerateCppName(const TString& src)" << "\n"
            << "{" << "\n"
            << "    TString result = \"FI_\";" << "\n"
            << "    for (size_t i = 0; i < src.size(); ++i) {" << "\n"
            << "        const char c = src[i];" << "\n"
            << "        result += ToUpper(c);" << "\n"
            << "        if ((i + 1 < src.size()) && !IsUpper(c) && IsUpper(src[i + 1])) {" << "\n"
            << "            result += \"_\";" << "\n"
            << "        }" << "\n"
            << "    }" << "\n"
            << "    return result;" << "\n"
            << "}" << "\n";
    }

    params.Cpp << "\n";

    // generate offsets
    TVector<std::pair<int, size_t>> offsets;

    {
        size_t offset = 0;
        int lastLevel = Min<int>();

        for (const auto& d : descriptors) {
            if (d.GetLevel() != lastLevel) {
                offsets.push_back(std::make_pair(lastLevel, offset));
                lastLevel = d.GetLevel();
            }

            ++offset;
        }

        offsets.push_back(std::make_pair(lastLevel, offset));
    }

    // output data tables
    {
        auto& out = params.Cpp;

        out << "namespace {\n";
        out << "    static const char* Tables[] = {\n";

        {
            for (const auto& d : descriptors) {
                out << "        \"_" << d.GetName() << "\",\n";
            }
        }

        out << "    };\n";
        out << "}\n\n";
    }

    // Phase 2. GetStreamFactorNames
    {
        params.Cpp
            << "TVector<TString> GetStreamFactorNames(int level)" << "\n"
            << "{" << "\n"
            << "    TVector<TString> result;" << "\n"
            << "    result.reserve(GetStreamFactorsSize(level));" << "\n";

        for (size_t i = 1; i < offsets.size(); ++i) {
            params.Cpp
                << "    if (level < " << offsets[i].first << ") {" << "\n"
                << "        return result;" << "\n"
                << "    }" << "\n";

            params.Cpp
                << "    for (size_t i = " << offsets[i - 1].second << "; i < " << offsets[i].second << "; ++i) {\n"
                << "        result.push_back(Tables[i] + 1);\n"
                << "    }\n";
        }

        params.Cpp
            << "    return result;" << "\n"
            << "}" << "\n";
    }
}

int main(int argc, const char **argv) {
    return MainImpl<NStreams::TCodeGenInput>(argc, argv);
}
