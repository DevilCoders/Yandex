#pragma once

#include <util/memory/blob.h>
#include <kernel/indexer/face/docinfo.h>

class THostAttrsCache;
class TAttrStacker;
class IDocumentDataInserter;
class TMultilanguageDocData;
struct TAttrProcessorConfig;

class TAttrPortion {
public:
    TAttrPortion(const TAttrProcessorConfig* cfg, TMultilanguageDocData* multilangDocData);
    ~TAttrPortion();

    void ProcessSearchAttr(IDocumentDataInserter* inserter, const TDocInfoEx& docInfo,
        const TAttrProcessorFlags* flags = nullptr) const;

private:
    TAttrProcessorFlags ProcFlags;
    THolder<TAttrStacker> AttrStacker;
    THolder<THostAttrsCache> HostAttrsCache;
    TMultilanguageDocData* MultilanguageDocData;
    TBlob TokenSplitterData;
};
