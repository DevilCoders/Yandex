#pragma once

#include "docactionface.h"
#include <util/generic/ptr.h>
#include <kernel/segnumerator/segnumerator.h>
#include <kernel/hosts/owner/owner.h>

namespace NIndexerCore {

class TDirectTextCreator;

class TSegmentAction :  public IDocumentAction, private TNonCopyable {
private:
    TDirectTextCreator& DTCreator;
    TOwnerCanonizer OwnerCanonizer;
    NSegm::NPrivate::TSegContext SegContext;
    typedef NSegm::TSegmentatorHandler<> TSegHandler;
    THolder<TSegHandler> SegHandler;
public:
    TSegmentAction(TDirectTextCreator& dtc, const TString& dom2Path);
    ~TSegmentAction() override;

    INumeratorHandler* OnDocProcessingStart(const TDocInfoEx* docInfo, bool isFullProcessing) override;
    void OnDocProcessingStop(const IParsedDocProperties* pars, const TDocInfoEx* docInfo, IDocumentDataInserter* inserter, bool isFullProcessing) override;
};

}
