#include "factors_usage_tester.h"

#include <kernel/relevfml/models_archive/models_archive.h>

#include <util/folder/path.h>
#include <util/generic/algorithm.h>
#include <util/generic/singleton.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/strip.h>
#include <util/string/vector.h>

#include <util/memory/blob.h>
#include <util/system/filemap.h>

TFactorsUsageTester::TFactorsUsageTester(const IFactorsSet* factorsSet)
    : FactorsSet(factorsSet)
    , FactorsAccountant(factorsSet)
{
}

bool ExpandFileName(TString& filename, const TString& sourceFile) {
    TFsPath filePath(sourceFile);
    if (filename[0] != '/') {
        filename = filePath.Dirname() + '/' + filename;
    }
    if (filename.EndsWith(".cpp")) {
        filename += ".factors";
        return true;
    } else if (filename.EndsWith(".info")) {
        return true;
    } else if (filename.EndsWith(".pln")) {
        return true;
    }
    return false;
}

void TFactorsUsageTester::ParseDefaultFormulasList(const TString& formulasList,
        TVector<TString>& result, const TString& sourcePath)
{
    TVector<TString> files;
    files.push_back(formulasList);
    for (size_t i = 0; i < files.size(); ++i) {
        bool isCMakeFile = false;
        if (files[i].EndsWith(".inc")) {
            isCMakeFile = true;
        }
        TFileInput in(files[i]);
        TString line;
        while (in.ReadLine(line)) {
            line = StripInPlace(line);
            if (isCMakeFile) {
                if (ExpandFileName(line, files[i])) {
                    result.push_back(line);
                }
            } else {
                if (!line.empty() && line[0] != '#') {
                    if (line.EndsWith(".factors") || line.EndsWith(".fml") ||
                            line.EndsWith(".fml2") || line.EndsWith(".fml3") ||
                            line.EndsWith(".info") || line.EndsWith(".pln") ||
                            line.EndsWith(".mnmc")) {
                        result.push_back(sourcePath + line);
                    } else if (line.EndsWith(".inc")) {
                        if (line[0] != '/') {
                            line = sourcePath + line;
                        }
                        files.push_back(line);
                    } else {
                        ythrow yexception() << "Unrecognized file format: " << line;
                    }
                }
            }
        }
    }
}

void TFactorsUsageTester::LoadAdditionalFormulas(const char **additionalSources, int numAdditionalSouces) {
    for (; numAdditionalSouces > 0; --numAdditionalSouces) {
        AddFormula(*additionalSources);
        ++additionalSources;
    }
}

void TFactorsUsageTester::AddFormula(const char* source) {
    FactorsAccountant.AddFormulaIndices(source);
}

void TFactorsUsageTester::LoadDefaultFormulas(const TString& formulasList, const TString& sourcesPath, bool justPrintFormulas) {
    TVector<TString> defaultFiles;
    ParseDefaultFormulasList(formulasList, defaultFiles, sourcesPath);

    for (TVector<TString>::const_iterator i = defaultFiles.begin(); i != defaultFiles.end(); ++i) {
        if (!justPrintFormulas)
            FactorsAccountant.AddFormulaIndices(*i);
        else
            Cout << *i << '\n';
    }
}

// Load information from .archive formulas (models.archive file)
void TFactorsUsageTester::AddFormulasFromArchive(const TString& archiveFile, bool justPrintFormulas) {
    try {
        TFileMap modelsArchive(archiveFile);
        modelsArchive.Map(0, modelsArchive.Length());

        TMap<TString, NMatrixnet::TMnSsePtr> models;
        NModelsArchive::LoadModels(modelsArchive, models, archiveFile);
        for (const auto& model: models) {
            TString formulaName = archiveFile + ":" + model.first;
            if (!justPrintFormulas) {
                FactorsAccountant.AddInfoModelFromArchive(*model.second.Get(), formulaName);
            } else {
                Cout << formulaName << '\n';
            }
        }
    } catch(yexception &e) {
        ythrow e << "Can't load models from archive " << archiveFile.Quote();
    }
}

TString TFactorsUsageTester::TestUnusedFactors() {
    TStringStream errors;
    TVector<size_t> unusedIndices;
    FactorsAccountant.GetUnusedIndices(&unusedIndices);
    bool isFailed = false;

    for (TVector<size_t>::const_iterator it = unusedIndices.begin(); it != unusedIndices.end(); ++it) {
        if (FactorsSet->IsDeprecatedFactor(*it) && !FactorsSet->IsRemovedFactor(*it)) {
            if (!isFailed) {
                errors << FactorsSet->GetDescription() <<
                        " TG_DEPRECATED factors not encountered in any formula:\n";
                isFailed = true;
            }
            errors << *it << '\t' << FactorsSet->GetFactorName(*it) << '\n';
        }
    }

    return errors.Str();
}

TString TFactorsUsageTester::TestUnusedButNotMarkedFactors(const int tillFactor) {
    TStringStream errors;
    TVector<size_t> unusedIndices;
    FactorsAccountant.GetUnusedIndices(&unusedIndices);
    bool isFailed = false;

    for (TVector<size_t>::const_iterator it = unusedIndices.begin(); it != unusedIndices.end(); ++it) {
        if (static_cast<int>(*it) <= tillFactor && !FactorsSet->IsUnusedFactor(*it)) {
            if (!isFailed) {
                errors << FactorsSet->GetDescription() <<
                    " Factors not encountered in any formula but not marked as TG_UNUSED/TG_DEPRECATED:\n";
                isFailed = true;
            }
            errors << *it << '\t' << FactorsSet->GetFactorName(*it) << '\n';
        }
    }

    return errors.Str();
}

TString TFactorsUsageTester::TestRemovedFactors() {
    TStringStream errors;
    TVector<size_t> indicesInUse;
    FactorsAccountant.GetIndicesInUse(&indicesInUse);
    bool isFailed = false;
    for (TVector<size_t>::const_iterator it = indicesInUse.begin(); it != indicesInUse.end(); ++it) {
        if (FactorsSet->IsRemovedFactor(*it)) {
            if (!isFailed) {
                errors << FactorsSet->GetDescription() <<
                    " TG_REMOVED factors encountered in some formula:\n";
                isFailed = true;
            }
            errors << *it << '\t' << FactorsSet->GetFactorName(*it) <<
                    "\n\t\tUsed in: " << FactorsAccountant.GetFilenames(*it) << '\n';
        }
    }
    return errors.Str();
}

TString TFactorsUsageTester::TestUnusedRearrangeFactors() {
    TStringStream errors;
    TVector<size_t> unusedIndices;
    FactorsAccountant.GetUnusedIndices(&unusedIndices);
    bool isFailed = false;

    for (TVector<size_t>::const_iterator it = unusedIndices.begin(); it != unusedIndices.end(); ++it)
        if (FactorsSet->IsRearrangeFactor(*it) &&
                (FactorsSet->IsUnusedFactor(*it) || FactorsSet->IsDeprecatedFactor(*it) ||
                 FactorsSet->IsRemovedFactor(*it)))
        {
            if (!isFailed) {
                errors << FactorsSet->GetDescription() <<
                    " Factors marked as TG_REARR_USE but it also marked as"
                    " TG_UNUSED/TG_DEPRECATED/TG_REMOVED and not encountered in any formula:\n";
                isFailed = true;
            }
            errors << *it << '\t' << FactorsSet->GetFactorName(*it) << "\n";
        }

    return errors.Str();
}

TString TFactorsUsageTester::TestRearrangeFactors() {
    TStringStream errors;
    bool isFailed = false;
    const TVector<TString>& FactorsNames = FactorsSet->GetFactorsNames();
    for (size_t idx = 0; idx < FactorsNames.size(); ++idx) {
        if (FactorsSet->IsRearrangeFactor(idx) &&
                (FactorsSet->IsUnusedFactor(idx) || FactorsSet->IsDeprecatedFactor(idx) ||
                 FactorsSet->IsRemovedFactor(idx)) && FactorsNames[idx].size())
        {
            if (!isFailed) {
                errors << FactorsSet->GetDescription() <<
                    " Factors marked as TG_REARR_USE but it also marked as TG_UNUSED/TG_DEPRECATED/TG_REMOVED:\n";
                isFailed = true;
            }
            errors << idx << '\t' << FactorsNames[idx] << "\n";
        }
    }

    return errors.Str();
}

TString TFactorsUsageTester::TestUnimplementedFactors() {
    TStringStream errors;
    TVector<size_t> indicesInUse;
    FactorsAccountant.GetIndicesInUse(&indicesInUse);
    bool isFailed = false;
    for (TVector<size_t>::const_iterator it = indicesInUse.begin(); it != indicesInUse.end(); ++it) {
        if (FactorsSet->IsUnimplementedFactor(*it)) {
            if (!isFailed) {
                errors << FactorsSet->GetDescription() <<
                    " TG_UNIMPLEMENTED factors encountered in some formula:\n";
                isFailed = true;
            }
            errors << *it << '\t' << FactorsSet->GetFactorName(*it) <<
                "\n\t\tUsed in: " << FactorsAccountant.GetFilenames(*it) << '\n';
        }
    }
    return errors.Str();
}

TString TFactorsUsageTester::GetInfoForUnusedFactors(bool groupByID) {
    TString result;
    TVector<size_t> indicesInUse;
    FactorsAccountant.GetIndicesInUse(&indicesInUse);

    if (groupByID) {
        TMap<TString, TVector<size_t> > data;
        for (TVector<size_t>::const_iterator it = indicesInUse.begin(); it != indicesInUse.end(); ++it) {
            if (FactorsSet->IsDeprecatedFactor(*it)) {
                TVector<TString> fmls;
                FactorsAccountant.GetFilenamesList(*it, fmls);
                for (size_t idx = 0; idx < fmls.size(); ++idx) {
                    data[fmls[idx]].push_back(*it);
                }
            }
        }
        for (TMap<TString, TVector<size_t> >::const_iterator it = data.begin(); it != data.end(); ++it) {
            result += it->first + "\n\t Use deprecated factors: ";
            for (size_t idx = 0; idx < it->second.size(); ++idx) {
                result += FactorsSet->GetFactorName(it->second[idx]) + " ("
                        + ToString(it->second[idx]) + ")";
                result += (idx == it->second.size() - 1) ? "" : ", ";
            }
            result += "\n\n";
        }
    } else {
        for (TVector<size_t>::const_iterator it = indicesInUse.begin(); it != indicesInUse.end(); ++it) {
            if (FactorsSet->IsDeprecatedFactor(*it)) {
                result += FactorsSet->GetDescription() + ' ' + ToString<size_t>(*it) +
                        '\t' + FactorsSet->GetFactorName(*it) +
                        "\n\t\t Used in: " + FactorsAccountant.GetFilenames(*it) + '\n';
                }
        }
    }

    return result;
}

TString TFactorsUsageTester::ShowFactorUsage(size_t factor) {
    TString result;
    TVector<size_t> indicesInUse;
    FactorsAccountant.GetIndicesInUse(&indicesInUse);

    if (BinarySearch(indicesInUse.begin(), indicesInUse.end(), factor)) {
        result += FactorsSet->GetDescription() + ' ' + ToString<size_t>(factor) +
                '\t' + FactorsSet->GetFactorName(factor) +
                "\n\t\t Used in: " + FactorsAccountant.GetFilenames(factor) + '\n';
    }

    return result;
}

TString TFactorsUsageTester::ShowIgnoreFactors() {
    TString result;
    TVector<TString> parts;
    TVector<size_t> usedFactors;
    const size_t factorCount = FactorsSet->GetFactorCount();
    for (size_t idx = 0; idx < factorCount; ++idx)
        if (!(FactorsSet->IsUnusedFactor(idx) || FactorsSet->IsDeprecatedFactor(idx) ||
                FactorsSet->IsRemovedFactor(idx)))
            usedFactors.push_back(idx);

    usedFactors.push_back(factorCount);
    size_t currentUnused = 0;

    for (size_t idx = 0; idx < usedFactors.size(); ++idx) {
        if (usedFactors[idx] == currentUnused + 1)
            parts.push_back(ToString(currentUnused));
        else if (usedFactors[idx] > currentUnused)
            parts.push_back(ToString(currentUnused) + "-" + ToString(usedFactors[idx] - 1));
        currentUnused = usedFactors[idx] + 1;
    }
    result = JoinStrings(parts, ":");

    return result;
}
