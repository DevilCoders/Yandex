#pragma once

#include <util/generic/ptr.h>

#include <kernel/indexer/face/docinfo.h>
#include <kernel/catfilter/catfilter_wrapper.h>
#include <library/cpp/microbdb/safeopen.h>
#include <yweb/robot/dbscheeme/baserecords.h>

#include "attrconf.h"

class TAttrPortion;
class IDocumentDataInserter;
class IGeoAttrStorage;
class TGeoData;
class TMultilanguageDocData;
class TCatData;

class TCatWork {
public:
    TCatWork(bool, const TAttrProcessorConfig* cfg, const ICatFilter* filter = nullptr, IGeoAttrStorage* geoAttrStorage = nullptr);
    ~TCatWork();

    template <typename TAttrFlusher>
    void Process(const TDocInfoEx& docInfo, IDocumentDataInserter* inserter, TAttrFlusher& attrFlusher, const TAttrProcessorFlags* flags = nullptr) const {
        TVector<ui32> catsv;
        ProcessCatAttrs(inserter, docInfo, catsv);
        attrFlusher.AddAttrs(docInfo, catsv.data(), catsv.size());
        ProcessSearchAttr(inserter, docInfo, flags);
        ProcessGeoAttrs(inserter, docInfo);
    }

    void Term();

private:
    void ProcessSearchAttr(IDocumentDataInserter* inserter, const TDocInfoEx& docInfo, const TAttrProcessorFlags* flags) const;
    void ProcessGeoAttrs(IDocumentDataInserter* inserter, const TDocInfoEx& docInfo) const;
    void ProcessCatAttrs(IDocumentDataInserter* inserter, const TDocInfoEx& docInfo, TVector<ui32>& cats) const;

private:
    THolder<TAttrPortion> AttrPortion;
    THolder<TCatData> CatData;
    THolder<TGeoData> GeoData;
    THolder<TMultilanguageDocData> MultilanguageDocData;
};
