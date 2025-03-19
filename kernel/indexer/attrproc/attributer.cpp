#include "attributer.h"

#include <kernel/indexer/face/inserter.h>
#include <kernel/indexer/face/docinfo.h>

#include <library/cpp/string_utils/url/url.h>
#include <util/string/cast.h>
#include <library/cpp/deprecated/fgood/fgood.h>

void TCatValueSplitter::LoadConfig(const char* fName) {
    if (!fName)
        return;
    Parts.reserve(100);
    TIFStream cfgFile(fName);
    char s[1024];
    TString buffer;
    int bound;
    while (cfgFile.ReadLine(buffer) && sscanf(buffer.data(), "%d %1000s", &bound, s) == 2) {
        Parts.resize(Parts.size() + 1);
        Parts.back().Bound = bound;
        if (strcmp(s, "-") != 0) {
            char* p = strrchr(s, '.');
            if (p) {
                Y_ASSERT(strcmp(p, ".d2c") == 0);
                *p = 0;
            }
            Parts.back().Name = s;
        }
    }
}

bool TCatValueSplitter::SplitCateg(int inCat, int* outCat, TString* catName) const {
    if (Parts.empty())
        return false;
    for (size_t i = Parts.size() - 1; i > 0; i--) {
        if (inCat >= Parts[i].Bound) {
            if (!!Parts[i].Name) {
                *catName = Parts[i].Name;
                *outCat = inCat - Parts[i].Bound;
                return true;
            }
            return false;
        }
    }
    return false;
}

TGroupAttributer::TGroupAttributer(const TString& configPath, TExistenceChecker ec)
    : DomAttrCanonizer(configPath)
    , ClonAttrCanonizer(ec.Check((configPath + "/clon.h2g.trie.idx").data()), ec.Check((configPath + "/clon.h2g.trie.dat").data()))
    , CatValueSplitter(ec.Check((configPath + "/spl-grp.cfg").data()))
{
}

TGroupAttributer::~TGroupAttributer() {
}


void TGroupAttributer::Store(IDocumentDataInserter* inserter, const TDocInfoEx& docInfo, ui32* catValues, size_t catValuesCount) const {
    TString host = TString{GetHost(CutHttpPrefix(docInfo.DocHeader->Url))};
    SetDomainAttributes(host, ToString(DomAttrCanonizer.GetHostOwner(host)), *inserter);

    // clon attribute
    TVector<ui32> clonGroups;
    ClonAttrCanonizer.GetHostClonGroups(host.data(), clonGroups);
    for (size_t i = 0; i < clonGroups.size(); ++i)
        inserter->StoreGrpDocAttr("clon", ToString(clonGroups[i]), true);
    // cat*, geo* (and others from filter.obj) attributes
    int outCat;
    TString catName;
    for (size_t i = 0; i < catValuesCount; i++) {
        if (CatValueSplitter.SplitCateg(catValues[i], &outCat, &catName))
            inserter->StoreGrpDocAttr(catName, ToString(outCat), true);
    }
}
