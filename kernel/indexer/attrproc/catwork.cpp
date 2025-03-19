#include "catwork.h"
#include "catdata.h"
#include "attrportion.h"
#include "geodata.h"
#include "multilangdata.h"

TCatWork::TCatWork(bool useAS, const TAttrProcessorConfig* cfg, const ICatFilter* filter, IGeoAttrStorage* geoAttrStorage) {
    if (!!cfg->GeoTrieName) {
        GeoData.Reset(new TGeoData(cfg, geoAttrStorage));
    }

    if (cfg->CreateMultilanguageDocAttrs)
        MultilanguageDocData.Reset(new TMultilanguageDocData(cfg));

    if (useAS)
        AttrPortion.Reset(new TAttrPortion(cfg, MultilanguageDocData.Get()));

    CatData.Reset(new TCatData(cfg, filter, MultilanguageDocData.Get()));
}

void TCatWork::Term() {
}

TCatWork::~TCatWork() {
}

void TCatWork::ProcessCatAttrs(IDocumentDataInserter* inserter, const TDocInfoEx& docInfo, TVector<ui32>& cats) const {
    if (CatData->IsLoaded())
        CatData->ProcessCatAttrs(inserter, docInfo, cats);
}

void TCatWork::ProcessGeoAttrs(IDocumentDataInserter* inserter, const TDocInfoEx& docInfo) const {
    if (inserter && GeoData.Get())
        GeoData->StoreGeoAttrs(*inserter, docInfo);
}

void TCatWork::ProcessSearchAttr(IDocumentDataInserter* inserter, const TDocInfoEx& docInfo, const TAttrProcessorFlags* flags) const {
    if (!AttrPortion)
        return;
    AttrPortion->ProcessSearchAttr(inserter, docInfo, flags);
}
