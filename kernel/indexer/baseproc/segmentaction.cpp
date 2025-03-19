#include "segmentaction.h"
#include "doc_attr_filler.h"
#include <kernel/indexer/dtcreator/dtcreator.h>
#include <kernel/indexer/face/docinfo.h>
#include <kernel/indexer/face/inserter.h>
#include <kernel/indexer/direct_text/dt.h>

namespace NIndexerCore {

TSegmentAction::TSegmentAction(TDirectTextCreator& dtc, const TString& dom2Path)
    : DTCreator(dtc)
{
    if (TExistenceChecker().Check(dom2Path.data()))
        OwnerCanonizer.LoadDom2(dom2Path);
}

TSegmentAction::~TSegmentAction() {
}

INumeratorHandler* TSegmentAction::OnDocProcessingStart(const TDocInfoEx* docInfo, bool /*isFullProcessing*/) {
    // раньше не сохранялись сегментные веса линков если isFullProcessing == false
    SegHandler.Reset(new TSegHandler());
    SegHandler->InitSegmentator(docInfo->DocHeader->Url, &OwnerCanonizer, &SegContext);
    return SegHandler.Get();
}

class TDirectZoneDataInserter : public IDocumentDataInserter {
private:
    IDocumentDataInserter* Other;
    TDirectTextCreator& DTCreator;
public:
    TDirectZoneDataInserter(IDocumentDataInserter* other, TDirectTextCreator& dtc)
        : Other(other)
        , DTCreator(dtc)
    {}
    void StoreLiteralAttr(const char* attrName, const char* attrText, TPosting pos) override {
        Other->StoreLiteralAttr(attrName, attrText, pos);
    }
    void StoreLiteralAttr(const char* attrName, const wchar16* attrText, size_t len, TPosting pos) override {
        Other->StoreLiteralAttr(attrName, attrText, len, pos);
    }
    void StoreDateTimeAttr(const char* attrName, time_t datetime) override {
        Other->StoreDateTimeAttr(attrName, datetime);
    }
    void StoreIntegerAttr(const char* attrName, const char* attrText, TPosting pos) override {
        Other->StoreIntegerAttr(attrName, attrText, pos);
    }
    void StoreKey(const char* key, TPosting pos) override {
        Other->StoreKey(key, pos);
    }
    void StoreZone(const char* zoneName, TPosting begin, TPosting end, bool archiveOnly = false) override {
        DTCreator.StoreZone(archiveOnly ? DTZoneText : DTZoneSearch|DTZoneText, zoneName, begin, true);
        DTCreator.StoreZone(archiveOnly ? DTZoneText : DTZoneSearch|DTZoneText, zoneName, end, false);
    }
    void StoreArchiveZoneAttr(const char* name, const wchar16* value, size_t length, TPosting pos) override {
        DTCreator.StoreZoneAttr(DTAttrText, name, value, length, pos, false, false, false);
    }
    void StoreLemma(const wchar16* lemma, size_t lemmaLen, const wchar16* form, size_t formLen, ui8 flags, TPosting pos, ELanguage lang) override {
        Other->StoreLemma(lemma, lemmaLen, form, formLen, flags, pos, lang);
    }
    void StoreTextArchiveDocAttr(const TString& name, const TString& value) override {
        Other->StoreTextArchiveDocAttr(name, value);
    }
    void StoreFullArchiveDocAttr(const TString& name, const TString& value) override {
        Other->StoreFullArchiveDocAttr(name, value);
    }
    void StoreErfDocAttr(const TString& name, const TString& value) override {
        Other->StoreErfDocAttr(name, value);
    }
    void StoreGrpDocAttr(const TString& name, const TString& value, bool isInt) override {
        Other->StoreGrpDocAttr(name, value, isInt);
    }
};

void TSegmentAction::OnDocProcessingStop(const IParsedDocProperties*, const TDocInfoEx*, IDocumentDataInserter* ins, bool isFullProcessing) {
    TDirectZoneDataInserter myins(ins, DTCreator);
    StoreSegmentatorSpans(*SegHandler, myins);
    if (isFullProcessing)
        Fill(SegHandler->GetSegmentSpans(), &myins);
    SegHandler.Destroy();
}

}
