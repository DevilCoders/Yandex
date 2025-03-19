#include <kernel/factor_slices/factor_domain.h>

#include <kernel/fill_factors_codegen/proto/options.pb.h>

#include <library/cpp/protobuf/util/pb_io.h>

#include <util/stream/file.h>
#include <util/system/compiler.h>

int main(int argc, const char** argv) {
    Y_ENSURE(argc > 3);
    TFileInput configFile(argv[1]);
    NFillFactors::TFillFactorsCodegenOptions config;
    ParseFromTextFormat(configFile, config);

    NFactorSlices::EFactorSlice slice = FromString<NFactorSlices::EFactorSlice>(config.GetSlice());
    NFactorSlices::TFactorDomain domain{slice};
    const IFactorsInfo* factorsInfo = domain.GetSliceFactorsInfo(slice);
    Y_ENSURE(factorsInfo);
    TString cppFileName = argv[2];
    TString headerFileName = argv[3];

    /* unused, can't find a way to make this usable */
    TFileOutput cppFile(cppFileName);
    TFileOutput headerFile(headerFileName);

    headerFile << "#pragma once\n\n";
    for (const auto& incl : config.GetIncludes()) {
        headerFile << "#include <" << incl << ">\n";
    }
    headerFile << "\n";

    Y_ENSURE(config.SourcesSize() != 0);

    THashSet<TStringBuf> usedClassNames;
    usedClassNames.reserve(config.SourcesSize());
    for (size_t index = 0; index < config.SourcesSize(); ++index) {
        usedClassNames.insert(config.GetSources(index).GetClassName());
    }

    THashMap<TStringBuf, TStringBuf> vaArgs;
    for (size_t index = 0; index < factorsInfo->GetFactorCount(); index++) {
        bool hasAllTags = true;
        for (const auto& tag : config.GetTags()) {
            if (!factorsInfo->HasTagName(index, tag)) {
                hasAllTags = false;
                break;
            }
        }
        if (hasAllTags) {
            THashMap<TString, TString> factorSources;
            factorsInfo->GetFactorSources(index, factorSources);
            for (auto&& [argName, className] : factorSources) {
                if (usedClassNames.find(className)) {
                    vaArgs[argName] = className;
                }
            }
        }
    }

    TStringStream methodHeader;
    methodHeader << "template <class FactorView>\n";
    methodHeader << "inline void " << config.GetMethodName() << "(";
    bool first = true;
    for (auto&& [argName, className] : vaArgs) {
        if (first) {
            first = false;
        } else {
            methodHeader << ", ";
        }
        methodHeader << "const " << className << "& " << argName;
    }
    methodHeader << ", FactorView&& view)";
    if (config.HasNestedNamespace()) {
        headerFile << "namespace " << config.GetNestedNamespace() << " {\n\n";
    }
    headerFile << methodHeader.Str() << " {\n";

    for ([[maybe_unused]] auto&& [argName, className] : vaArgs) {
        headerFile << "    Y_UNUSED(" << argName << ");\n";
    }

    for (size_t index = 0; index < factorsInfo->GetFactorCount(); index++) {
        bool hasAllTags = true;
        for (const auto& tag : config.GetTags()) {
            if (!factorsInfo->HasTagName(index, tag)) {
                hasAllTags = false;
                break;
            }
        }
        bool hasForbiddenTags = false;
        for (const auto& tag : config.GetForbiddenTags()) {
            if (factorsInfo->HasTagName(index, tag)) {
                hasForbiddenTags = true;
                break;
            }
        }
        if (hasAllTags && !hasForbiddenTags) {
            THashMap<TString, TString> factorSources;
            factorsInfo->GetFactorSources(index, factorSources);
            if (factorSources.empty()) {
                continue;
            }
            bool notSubset = false;
            for (auto&& [argName, className] : factorSources) {
                auto vit = vaArgs.find(argName);
                if (vit == vaArgs.end() || vit->second != className) {
                    notSubset = true;
                    break;
                }
            }
            if (notSubset) {
                continue;
            }
            TString expression(factorsInfo->GetFactorExpression(index));
            headerFile << "    view[" << (config.HasFactorEnumNamespace() ? (config.GetFactorEnumNamespace() + "::") : "") << factorsInfo->GetFactorInternalName(index) << "] = "
                << expression << ";\n";
        } else {
            headerFile << "    view[" << (config.HasFactorEnumNamespace() ? (config.GetFactorEnumNamespace() + "::") : "") << factorsInfo->GetFactorInternalName(index) << "] = "
                << "0.f" << ";\n";
        }
    }
    headerFile << "}\n";

    if (config.HasNestedNamespace()) {
        headerFile << "\n}\n";
    }

    return 0;
}
