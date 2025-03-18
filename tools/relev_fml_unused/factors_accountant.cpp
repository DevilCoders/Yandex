#include "factors_accountant.h"

#include <kernel/matrixnet/mn_sse.h>
#include <kernel/matrixnet/mn_dynamic.h>
#include <kernel/matrixnet/mn_multi_categ.h>

#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/string/vector.h>
#include <util/ysaveload.h>

template <class T>
void TFactorsAccountant::AddModel(const T& mnFml, const TString& name) {
    if (mnFml.NumFeatures() > int(FactorsSet->GetFactorCount())) {
        // This is fix for formulas with slices, they may have more features
        // than in the factors_gen.in. See SEARCH-1636 for more details.
        // Future fix will be in SEARCH-1482
        Cerr << "Too many features in " << name << Endl;
        return;
    }

    TSet<ui32> factors;
    mnFml.UsedFactors(factors);

    for (TSet<ui32>::const_iterator i = factors.begin(); i != factors.end(); ++i) {
        TFactorsInUse::key_type factor = static_cast<TFactorsInUse::key_type>(*i);
        FactorsInUse[factor].push_back(name);
    }
}

template <class T>
void TFactorsAccountant::LoadModel(const TString& filename) {
    T mnFml;
    TFileInput input(filename);
    ::Load(&input, mnFml);

    AddModel(mnFml, filename);
}

TFactorsAccountant::TFactorsAccountant(const IFactorsSet* factorsSet)
    : FactorsSet(factorsSet)
{
}

void TFactorsAccountant::AddPolynomIndices(const TString& filename) {
    TFileInput in(filename);
    TPolynomDescr polynom;
    ::Load(&in, polynom);

    TVector<bool> alreadyUsedFactors;
    alreadyUsedFactors.resize(FactorsSet->GetFactorCount());

    return AddSingleFmlIndices(filename, polynom.Descr, alreadyUsedFactors);
}


void TFactorsAccountant::AddSingleFmlIndices(const TString& filename, const SRelevanceFormula &formula, TVector<bool>& alreadyUsedFactors) {
    TVector< TVector<size_t> > monoms;
    TVector<float> weights;
    formula.GetFormula(&monoms, &weights);

    for (TVector<TVector<size_t> >::const_iterator itMonom = monoms.begin(); itMonom != monoms.end(); ++itMonom) {
        for (TVector<size_t>::const_iterator itFactor = itMonom->begin(); itFactor != itMonom->end(); ++itFactor) {
            TVector<TString>& value = FactorsInUse[*itFactor];
            if (value.empty() || !alreadyUsedFactors[*itFactor]) {
                value.push_back(filename);
                alreadyUsedFactors[*itFactor] = true;
            }
        }
    }
}

void TFactorsAccountant::AddFmlIndices(const TString& filename) {
    TFileInput in(filename);

    TVector<bool> alreadyUsedFactors;
    alreadyUsedFactors.resize(FactorsSet->GetFactorCount());
    TString line;
    while (in.ReadLine(line)) {

        if (line.empty() || line[0] == '#') {
            continue;
        }

        TVector<TString> parts;
        Split(line, TString(" "), parts);
        if (parts[0] == "!" || parts[0] == "*") {
            parts.erase(parts.begin());
        }

        if (parts.size() < 2) {
            ythrow yexception() << "Wrong formula line in " << filename << ": " << line;
        }

        SRelevanceFormula formula;
        Decode(&formula, parts[1], FactorsSet->GetFactorCount());
        AddSingleFmlIndices(filename, formula, alreadyUsedFactors);
    }
}

void TFactorsAccountant::AddFactorsIndices(const TString& filename) {
    if (!filename.EndsWith(".factors")) {
        ythrow yexception() << filename << " must ends with '.factors'";
    }

    TString sourceFilename = filename.substr(0, filename.size() - 8);

    TFileInput in(filename);

    TString md5;
    if (!in.ReadLine(md5)) {
        ythrow yexception() << filename << " has wrong format";
    }

    char md5buf[33];
    if (!MD5::File(sourceFilename.data(), md5buf)) {
        ythrow yexception() << "Can't calculate md5 of " << sourceFilename;
    }

    if (md5 != TString(md5buf)) {
        ythrow yexception() << "md5 of " << sourceFilename << " is " << md5buf << " while " << md5 << " recorded in " << filename;
    }

    TString factors;
    if (!in.ReadLine(factors)) {
        ythrow yexception() << filename << " has wrong format";
    }

    TVector<TString> factorsStrings;
    Split(factors, TString(" "), factorsStrings);
    for (TVector<TString>::const_iterator it = factorsStrings.begin(); it != factorsStrings.end(); ++it) {
        TFactorsInUse::key_type index = FromString<TFactorsInUse::key_type>(*it);
        FactorsInUse[index].push_back(filename);
    }
}

void TFactorsAccountant::AddInfoModel(const TString& filename) {
    LoadModel<NMatrixnet::TMnSseDynamic>(filename);
}

void TFactorsAccountant::AddMnmcModel(const TString& filename) {
    LoadModel<NMatrixnet::TMnMultiCateg>(filename);
}

void TFactorsAccountant::AddInfoModelFromArchive(const NMatrixnet::TMnSseInfo& info, const TString& name) {
    AddModel<NMatrixnet::TMnSseInfo>(info, name);
}

void TFactorsAccountant::AddFormulaIndices(const TString& filename) {
    if (filename.EndsWith(".fml") || filename.EndsWith(".fml2") || filename.EndsWith(".fml3")) {
        AddFmlIndices(filename);
    } else if (filename.EndsWith(".pln")) {
        AddPolynomIndices(filename);
    } else if (filename.EndsWith(".factors")) {
        AddFactorsIndices(filename);
    } else if (filename.EndsWith(".info")) {
        AddInfoModel(filename);
    } else if (filename.EndsWith(".mnmc")) {
        AddMnmcModel(filename);
    } else {
        ythrow yexception() << "Filename '" << filename <<
            "' doesn't ends with one of next available suffixes: fml, fml2, fml3, factors, pln, info or mnmc";
    }
}

void TFactorsAccountant::GetUnusedIndices(TVector<size_t>* unusedIndices) {
    const size_t factorCount = FactorsSet->GetFactorCount();
    for (size_t i = 0; i < factorCount; ++i)
        if (!FactorsInUse.contains(i))
            unusedIndices->push_back(i);
}

void TFactorsAccountant::GetIndicesInUse(TVector<size_t>* usedIndices) {
    const size_t factorCount = FactorsSet->GetFactorCount();
    for (size_t i = 0; i < factorCount; ++i)
        if (FactorsInUse.contains(i))
            usedIndices->push_back(i);
}

TString TFactorsAccountant::GetFilenames(size_t factor) {
    const auto fileNames = FactorsInUse.FindPtr(factor);
    if (!fileNames)
        return TString();

    return JoinStrings(*fileNames, ", ");
}

void TFactorsAccountant::GetFilenamesList(size_t factor, TVector<TString>& names) {
    names.clear();
    const auto pNames = FactorsInUse.FindPtr(factor);
    if (pNames)
        names = *pNames;
}
