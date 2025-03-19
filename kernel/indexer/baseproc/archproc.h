#pragma once

#include "docactionface.h"

#include <util/generic/hash_set.h>
#include <util/generic/string.h>

class IOutputStream;
class TSentAttrStorage;
class TArchiveCreator;
struct TArchiveProcessorConfig;

namespace NIndexerCore{

class TSentPropHandler;

class TTextArchiveAction : public IDocumentAction, private TNonCopyable {
public:
    THolder<TSentPropHandler> Handler;
private:
    THolder<TArchiveCreator> ArchiveCreator;
    bool UseArchive;
    TVector<TString> DocProperties;
    THolder<TSentAttrStorage> SentAttrStorage;
    THashSet<TString> PassageProperties;
public:
    TTextArchiveAction(const TArchiveProcessorConfig* cfg, IOutputStream& outArc);
    ~TTextArchiveAction() override;

    TArchiveCreator* GetArchiveCreator() {
        return ArchiveCreator.Get();
    }

    INumeratorHandler* OnDocProcessingStart(const TDocInfoEx* docInfo, bool isFullProcessing) override;
    void OnDocProcessingStop(const IParsedDocProperties* pars, const TDocInfoEx* docInfo, IDocumentDataInserter* inserter, bool isFullProcessing) override;
};

}
