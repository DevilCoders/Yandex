#pragma once

#include "docactionface.h"
#include "groupconf.h"

#include <util/generic/ptr.h>
#include <util/generic/set.h>
#include <kernel/groupattrs/config.h>
#include <kernel/groupattrs/creator/metacreator.h>

namespace NGroupingAttrs {
    class ICreator;
    class TMutableDocAttrs;
}

class IParsedDocProperties;
class TFullDocAttrs;

class TGroupProcessor {
public:
    TGroupProcessor(const TGroupProcessorConfig* cfg, NGroupingAttrs::IMetaInfoCreator* metaInfoCreator = nullptr);
    ~TGroupProcessor();

    void CommitDoc(const IParsedDocProperties* pars, ui32 docId, TFullDocAttrs& extAttrs, bool AddAttrsIntoExt = true);
    void Term();
private:
    void AddDocAttrs(TFullDocAttrs& extAttrs, const IParsedDocProperties* pars);
    THolder<NGroupingAttrs::ICreator> Creator;
    NGroupingAttrs::TConfig* Config;
    TString NewAttrPath;
};

class TGroupDocumentAction : public NIndexerCore::IDocumentAction, private TNonCopyable {
public:
private:
    NGroupingAttrs::TConfig GroupingConfig;
public:
    TGroupDocumentAction(const TGroupProcessorConfig* cfg);
    void OnDocProcessingStop(const IParsedDocProperties* pars, const TDocInfoEx* docInfo, IDocumentDataInserter* inserter, bool isFullProcessing) override;
};
