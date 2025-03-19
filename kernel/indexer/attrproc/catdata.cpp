#include "catdata.h"
#include "catclosure.h"
#include "multilangdata.h"
#include "attrconf.h"

#include <kernel/indexer/face/inserter.h>
#include <yweb/robot/dbscheeme/mergecfg.h>
#include <kernel/langregion/langregion.h>

TCatData::TCatData(const TAttrProcessorConfig* cfg, const ICatFilter* filter, TMultilanguageDocData* multilangDocData)
    : Attrs("attrs", dbcfg::fbufsize << 6, 0)
    , MultilanguageDocData(multilangDocData)
{
    HasCatData = false;
    HasDatCatData = false;
    HasLocalCatData = false;
    HasMirrorsData = false;

    const TExistenceChecker& ec = cfg->ExistenceChecker;

    if (ec.Check(cfg->MirrorsPath.data())) {

        try {
            Mirrors.Reset(new TMirrorsMappedTrie(cfg->MirrorsPath.data(), 0));
        } catch (...) {
            ythrow yexception() << "Can't load mirrors <" << cfg->MirrorsPath << ">";
        }

        HasMirrorsData = true;
    }

    if (ec.Check(cfg->CatClosurePath.data()))
        CatClosure.Reset(new TCatClosure(cfg->CatClosurePath.data()));

    if (ec.Check(cfg->FilterPath.data())) {

        try {
            Filter.Reset(filter ? filter : GetCatFilter(cfg->FilterPath, cfg->MapFilter, HasMirrorsData ? Mirrors.Get() : nullptr));
        } catch (...) {
            ythrow yexception() << "Can't load filter <" << cfg->FilterPath << ">";
        }

        HasCatData = true;

        if (ec.Check(cfg->LocalFilterPath.data())) {
            try {
                LocalFilter.Reset(GetCatFilter(cfg->LocalFilterPath, cfg->MapFilter, HasMirrorsData ? Mirrors.Get() : nullptr));
            } catch (...) {
                ythrow yexception() << "Can't load local filter <" << cfg->LocalFilterPath << ">";
            }

            HasLocalCatData = true;
        }
    }

    if (ec.Check(cfg->AttrsPath.data())) {
        Attrs.Open(cfg->AttrsPath.data());
        CurAttr = Attrs.Next();
        HasDatCatData = true;
    }
}

TCatData::~TCatData(){
}

void TCatData::ProcessCatAttrs(IDocumentDataInserter* inserter, const TDocInfoEx& docInfo, TVector<ui32>& cats) {
    Y_ASSERT(IsLoaded());
    THashSet<ui32> catSet;
    TString url(docInfo.DocHeader->Url);
    if (!url.StartsWith("https://") && !url.StartsWith("http://")) {
        url = "http://" + url;
    }
    FillCatSet(url.data(), docInfo.DocId, &catSet);
    FillCatVector(catSet, &cats);
    ProcessSearchCatAttr(inserter, catSet);
}

void TCatData::FillCatSet(const char* url, const ui32 docId, TCatSet* res) {
    if(HasLocalCatData){
        TVector<ui32> globalAutoGeo;
        bool hasGlobalUserGeo = false;
        bool hasExactGlobalYacaGeo = false;
        bool hasLocalCats = false;

        TCatAttrsPtr localCats = LocalFilter->Find(url);
        hasLocalCats = localCats->size() > 0;

        TCatAttrsPtr globalCats = Filter->Find(url);

        for (TCatAttrs::const_iterator it = globalCats->begin(); it != globalCats->end(); it++) {
            if (*it >= 31000000 && *it < 32000000){
                hasGlobalUserGeo = true;
            }
            if (*it == 22100011){
                hasExactGlobalYacaGeo = true;
            }
            if ((*it >= 21000000 && *it < 22000000) || (*it >= 121000000 && *it < 122000000)){
                globalAutoGeo.push_back(*it);
            } else {
                res->insert(*it);
            }
        }
        if (hasLocalCats && !hasGlobalUserGeo && !hasExactGlobalYacaGeo){
            for (TCatAttrs::const_iterator it = localCats->begin(); it != localCats->end(); it++)
                res->insert(*it);
        } else {
            TVector<ui32>::const_iterator b = globalAutoGeo.begin(), e = globalAutoGeo.end();
            for (; b != e; ++b){
                res->insert(*b);
            }
        }
        return;
    }

    if (HasCatData) {
        TCatAttrsPtr cats = Filter->Find(url);
        for (TCatAttrs::const_iterator it = cats->begin(); it != cats->end(); it++)
            res->insert(*it);
    } else if (HasDatCatData) {
        while (CurAttr && (CurAttr->DocId < docId))
            CurAttr = Attrs.Next();
        while (CurAttr && (CurAttr->DocId == docId)) {
            res->insert(CurAttr->GroupId);
            CurAttr = Attrs.Next();
        }
    }

    if (MultilanguageDocData) {
        const TDocGroupRec* multilanguageDocRec = MultilanguageDocData->MoveToDoc(docId);
        if (multilanguageDocRec != nullptr && multilanguageDocRec->DocId == docId) {
            res->insert(62000000 + GetAttrValueByLangRegion(static_cast<ELangRegion>(multilanguageDocRec->GroupId)));
        }
    }

}

void TCatData::FillCatVector(const TCatSet& catSet, TVector<ui32>* res) const {
    TCatSet::const_iterator b = catSet.begin(), e = catSet.end();
    for (; b != e; ++b)
        res->push_back(*b);
    Sort(res->begin(), res->end());
}

void TCatData::ProcessSearchCatAttr(IDocumentDataInserter* inserter, const THashSet<ui32>& catSet) const {
    if (!inserter)
        return;
    TCatSet catSetTrans;
    CatClosure->DoClosure(catSet, &catSetTrans);
    char buf[20];
    for (const auto &b : catSetTrans) {
        sprintf(buf, "%u", b);
        inserter->StoreIntegerAttr("cat", buf, 0);
    }
}
