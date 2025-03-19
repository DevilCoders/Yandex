#include <util/string/cast.h>

#include "attrproc.h"
#include "attributer.h"

class TFlusherToDocAttrs {
public:
    TFlusherToDocAttrs(const TGroupAttributer& attributer, IDocumentDataInserter* inserter)
        : Attributer(attributer)
        , Inserter(inserter)
    {}
    void AddAttrs(const TDocInfoEx& docInfo, ui32* catValues, size_t catValuesCount) {
        Attributer.Store(Inserter, docInfo, catValues, catValuesCount);
    }
private:
    const TGroupAttributer& Attributer;
    IDocumentDataInserter* Inserter;
};

class TFakeFlusher {
public:
    TFakeFlusher()
    {}
    void AddAttrs(const TDocInfoEx&, ui32*, size_t) {
    }
};

TAttrDocumentAction::TAttrDocumentAction(const TAttrProcessorConfig* cfg, const ICatFilter* filter, const TGroupAttributer* groupAttributer)
    : Attributer(nullptr)
    , AttrFlags(*cfg)
{
    CatWork.Reset(new TCatWork(true, cfg, filter));
    if (!!cfg->AttributerDir) {
        if (!!groupAttributer)
            Attributer = groupAttributer;
        else
            MyAttributer.Reset(Attributer = new TGroupAttributer(cfg->AttributerDir, cfg->ExistenceChecker));
    }
}

TAttrDocumentAction::~TAttrDocumentAction() {
}

const TOwnerCanonizer& TAttrDocumentAction::GetInternalOwnerCanonizer() {
    return Attributer->GetInternalOwnerCanonizer();
}

static bool ParseOneProcFlag(bool& result, const IParsedDocProperties* props, const char* name) {
    const char* strValue = nullptr;
    if (!props->GetProperty(name, &strValue) && TryFromString<bool>(strValue, result)) {
        return true;
    }
    return false;
}

static bool ParseProcFlags(TAttrProcessorFlags& flags, const IParsedDocProperties* props) {
    bool changed = false;

    changed = ParseOneProcFlag(flags.IndexUrl, props, "IndexUrl") || changed;
    changed = ParseOneProcFlag(flags.IndexUrlAttributes, props, "IndexUrlAttributes") || changed;
    changed = ParseOneProcFlag(flags.SplitUrl, props, "SplitUrl") || changed;
    changed = ParseOneProcFlag(flags.CutScheeme, props, "CutScheeme") || changed;
    changed = ParseOneProcFlag(flags.IgnoreDateAttrs, props, "IgnoreDateAttrs") || changed;

    return changed;
}

void TAttrDocumentAction::OnDocProcessingStop(const IParsedDocProperties* props, const TDocInfoEx* docInfo, IDocumentDataInserter* inserter, bool isFullProcessing) {
    if (isFullProcessing) {
        TAttrProcessorFlags customFlags = AttrFlags;
        TAttrProcessorFlags* customFlagsPtr = ParseProcFlags(customFlags, props)
            ? &customFlags
            : nullptr;
        if (!!Attributer) {
            TFlusherToDocAttrs fl(*Attributer, inserter);
            CatWork->Process(*docInfo, inserter, fl, customFlagsPtr);
        } else {
            TFakeFlusher fl;
            CatWork->Process(*docInfo, inserter, fl, customFlagsPtr);
        }
    }
}
