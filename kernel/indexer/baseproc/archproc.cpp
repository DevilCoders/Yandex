#include "archproc.h"
#include "archconf.h"

#include <kernel/indexer/face/docinfo.h>
#include <kernel/indexer/face/inserter.h>
#include <kernel/tarc/docdescr/docdescr.h>
#include <ysite/directtext/textarchive/sastorage.h>
#include <ysite/directtext/textarchive/createarc.h>
#include <library/cpp/charset/wide.h>
#include <library/cpp/html/entity/htmlentity.h>

#include <util/string/split.h>

namespace NIndexerCore {

class TSentPropHandler : public INumeratorHandler {
public:
    TSentPropHandler()
        : PassageProperties(nullptr)
        , SentAttrStorage(nullptr)
    {}

    void Set(const THashSet<TString>* passageProperties, TSentAttrStorage* sentAttrStorage) {
        PassageProperties = passageProperties;
        SentAttrStorage = sentAttrStorage;
    }

    void OnZoneImpl(const char* name, bool open, const TNumerStat&) {
        if (SentAttrStorage) {
            if (open)
                CurZone = name;
            else
                CurZone.remove();
        }
    }

    void OnAttrImpl(const char* name, ATTR_TYPE , bool , const wchar16* attrText, size_t length, const TNumerStat&) {
        if (SentAttrStorage) {
            TString zattr = CurZone + TString("#") + name;
            if (PassageProperties->contains(zattr))
                SentAttrStorage->AddSentAttr(name, WideToChar(attrText, length, CODES_YANDEX));
        }
    }

    void OnSpacesImpl(TBreakType t, const wchar16 *, unsigned , const TNumerStat& stat) {
        if (SentAttrStorage && IsSentBrk(t)) {
            Y_ASSERT(SentAttrStorage);
            SentAttrStorage->CommitSentAttrs(stat.TokenPos.Break());
        }
    }

private:
    const THashSet<TString>* PassageProperties;
    TSentAttrStorage* SentAttrStorage;
    TString CurZone;

};

TTextArchiveAction::TTextArchiveAction(const TArchiveProcessorConfig* cfg, IOutputStream& outArc) {
    UseArchive = cfg->UseArchive;
    ui32 arcsettings = cfg->SaveHeader ? TArchiveCreator::SaveDocHeader : TArchiveCreator::WriteBlob;
    if (cfg->CompressExtInfo) {
        arcsettings |= TArchiveCreator::CompressExtInfo;
    }
    ArchiveCreator.Reset(new TArchiveCreator(outArc, arcsettings));

    if (UseArchive) {
        Split(cfg->DocProperties, " ,\t\r", DocProperties);
        TVector<TString> psgprop;
        Split(cfg->PassageProperties, " ,\t\r", psgprop);
        for (TVector<TString>::iterator it = psgprop.begin(); it != psgprop.end(); ++it)
            PassageProperties.insert(*it);
        if (!PassageProperties.empty())
            SentAttrStorage.Reset(new TSentAttrStorage);
    }
}

TTextArchiveAction::~TTextArchiveAction() {
}

INumeratorHandler* TTextArchiveAction::OnDocProcessingStart(const TDocInfoEx* /*docInfo*/, bool isFullProcessing) {
    if (!isFullProcessing || !SentAttrStorage)
        return nullptr;
    Handler.Reset(new TSentPropHandler());
    SentAttrStorage->Clear();
    Handler->Set(&PassageProperties, SentAttrStorage.Get());
    return Handler.Get();
}

void TTextArchiveAction::OnDocProcessingStop(const IParsedDocProperties* pars, const TDocInfoEx* /*docInfo*/, IDocumentDataInserter* inserter, bool isFullProcessing) {
    if (!isFullProcessing)
        return;
    // parser properties to archive
    if (!DocProperties.empty()) {
        for (size_t ek = 0; ek < DocProperties.size(); ++ek) {
            const TString& infokey = DocProperties[ek];

            pars->EnumerateValues(infokey.data(), [infokey, pars, inserter] (const char* prop)
                {
                    size_t len = strlen(prop);
                    TCharTemp tempBuf(len);
                    size_t lenbuf = HtEntDecodeToChar(pars->GetCharset(), prop, (unsigned)len, tempBuf.Data());
                    TString value = WideToUTF8(tempBuf.Data(), lenbuf);
                    inserter->StoreTextArchiveDocAttr(infokey, value);
                }
            );
        }
    }

    if (!!SentAttrStorage) {
        const TSentAttrStorage::TSentAttrs& sa = SentAttrStorage->GetSentAttrs();
        ArchiveCreator->SetCurrentSentAttrs(sa.data(), sa.size());
    }
    if (!UseArchive)
        ArchiveCreator->SetCurrentNoText();

    Handler.Destroy();
}

}
