#pragma once

#include "attrconf.h"
#include "catwork.h"

#include <kernel/indexer/face/docinfo.h>
#include <kernel/indexer/baseproc/docactionface.h>

class TGroupAttributer;
class TOwnerCanonizer;
class IDocumentDataInserter;

class TAttrDocumentAction : public NIndexerCore::IDocumentAction {
private:
    THolder<const TCatWork> CatWork;
    const TGroupAttributer* Attributer;
    THolder<const TGroupAttributer> MyAttributer;
    TAttrProcessorFlags AttrFlags;
public:
    TAttrDocumentAction(
        const TAttrProcessorConfig* cfg,
        const ICatFilter* filter = nullptr,
        const TGroupAttributer* groupAttributer = nullptr
    );
    ~TAttrDocumentAction() override;
    void OnDocProcessingStop(const IParsedDocProperties* p, const TDocInfoEx* docInfo, IDocumentDataInserter* inserter, bool isFullProcessing) override;
    // This method is used to eliminate TOwnerCanonizer duplication in the rtindexer.
    const TOwnerCanonizer& GetInternalOwnerCanonizer();
};
