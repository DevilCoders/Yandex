#pragma once

#include <kernel/hosts/clons/clon.h>
#include <kernel/hosts/owner/owner.h>
#include <util/folder/dirut.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

class TCatValueSplitter {
private:
    struct TPart {
        int Bound;
        TString Name;
    };
    TVector<TPart> Parts;
public:
    TCatValueSplitter() {
    }
    TCatValueSplitter(const TString& fileName) {
        LoadConfig(fileName.data());
    }

    void LoadConfig(const char* fName);
    bool SplitCateg(int inCat, int* outCat, TString* catName) const;
};

class IDocumentDataInserter;
struct TDocInfoEx;

class TGroupAttributer {
public:
    TGroupAttributer(const TString& configPath, TExistenceChecker ec = TExistenceChecker());
    ~TGroupAttributer();

    void Store(IDocumentDataInserter* inserter, const TDocInfoEx& docInfo, ui32* catValues, size_t catValuesCount) const;

    // This method is used to eliminate TOwnerCanonizer duplication in the rtindexer.
    const TOwnerCanonizer& GetInternalOwnerCanonizer() const {
        return DomAttrCanonizer;
    }
private:
    const TDomAttrCanonizer DomAttrCanonizer;
    const TClonAttrCanonizer ClonAttrCanonizer;
    const TCatValueSplitter CatValueSplitter;
};

// arguments passes as value due to "to_lower" function
template<class TInserter>
inline void SetDomainAttributes(TString host, TString hostOwner, TInserter& inserter) {
    host.to_lower();
    // h attribute
    inserter.StoreGrpDocAttr("h", host, false);
    // d attribute
    hostOwner.to_lower();
    inserter.StoreGrpDocAttr("d", hostOwner, false);
}

