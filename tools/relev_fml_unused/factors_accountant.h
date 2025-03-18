#pragma once

#include "factors.h"

#include <kernel/relevfml/relev_fml.h>
#include <kernel/matrixnet/mn_sse.h>

#include <library/cpp/digest/md5/md5.h>
#include <util/generic/algorithm.h>
#include <util/generic/map.h>

// TFactorsAccountant class is used to collect infromation
// about factors used in the bunch of formulas. This information
// is used by TFactorsUsageTester.
class TFactorsAccountant {
private:
    typedef TMap<size_t, TVector<TString> > TFactorsInUse;

    const IFactorsSet* FactorsSet;
    TFactorsInUse FactorsInUse;

    void AddSingleFmlIndices(const TString& filename, const SRelevanceFormula &formula, TVector<bool>& alreadyUsedFactors);
    void AddFmlIndices(const TString& filename);
    void AddPolynomIndices(const TString& filename);
    void AddInfoModel(const TString& filename);
    void AddMnmcModel(const TString& filename);
    void AddFactorsIndices(const TString& filename);

    template <class T>
    void AddModel(const T& mnFml, const TString& name);
    template <class T>
    void LoadModel(const TString& filename);

public:
    TFactorsAccountant(const IFactorsSet* FactorsSet);

    // Add information from .fml[2-3]?/.factors file
    void AddFormulaIndices(const TString& filename);
    // Add single info formula from .archive file (models.archive) already mapped to the memory
    void AddInfoModelFromArchive(const NMatrixnet::TMnSseInfo& info, const TString& name);
    // Get the list of factors not encountered in any formula
    void GetUnusedIndices(TVector<size_t>* unusedIndices);
    // Get the list of factors encountered in some formula
    void GetIndicesInUse(TVector<size_t>* usedIndices);
    // Get names of files with formulas where the 'factor' is used
    TString GetFilenames(size_t factor);
    // Get vector with names of files with formulas where the 'factor' is used
    void GetFilenamesList(size_t factor, TVector<TString>& names);
};
