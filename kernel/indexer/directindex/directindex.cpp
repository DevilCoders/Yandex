#include "directindex.h"
#include "directtokenizer.h"

#include <util/system/defaults.h>
#include <util/system/maxlen.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <library/cpp/charset/wide.h>
#include <util/stream/file.h>

#include <kernel/indexer/faceproc/docattrinserter.h>
#include <ysite/directtext/textarchive/createarc.h>
#include <ysite/directtext/textarchive/sastorage.h>

namespace NIndexerCore {

class TDirectIndex::TImpl : public TDirectTokenizer {
public:
    TArchiveCreator* PlainArchiveCreator;
    IDisambDirectText* Disamber;
    IDirectTextCallback4* InvCreator;
    TVector<IDirectTextCallback2*> Callbacks;
    TVector<IDirectTextCallback5*> CallbacksV5;
    IModifyDirectText* RelevMod;
    THolder<TSentAttrStorage> SentAttrStorage;
private:
    const TAttribute* SentAttr;
    size_t SentAttrCount;
public:
    TImpl(TDirectTextCreator& dtc, bool backwardCompatible)
        : TDirectTokenizer(dtc, backwardCompatible)
        , PlainArchiveCreator(nullptr)
        , Disamber(nullptr)
        , InvCreator(nullptr)
        , RelevMod(nullptr)
        , SentAttr(nullptr)
        , SentAttrCount(0)
    {
    }
    ~TImpl() override {
    }
    void Finish() {
        for (size_t i = 0; i < Callbacks.size(); i++) {
            IDirectTextCallback2* cb = Callbacks[i];
            cb->Finish();
        }
        if (RelevMod) {
            RelevMod->Finish();
        }
        if (InvCreator)
            InvCreator->MakePortion();
        Callbacks.clear();
        PlainArchiveCreator = nullptr;
        Disamber = nullptr;
        InvCreator = nullptr;
    }
    void OpenZone(const TString& zoneName) {
        Creator.StoreZone(DTZoneSearch|DTZoneText, zoneName.data(), GetPosition().DocLength(), true);
    }
    template <typename TAttr> void StoreDocAttr(const TAttr& attr, TPosting pos, EDTAttrType type) {
        PrepareAndStoreAttribute(pos, attr, type);
    }
    template <typename TAttr> void OpenZone(const TString& zoneName, const TAttr* attr, size_t attrCount) {
        Y_ASSERT(attr && attrCount);
        OpenZone(zoneName);
        for (size_t i = 0; i < attrCount; ++i) {
            PrepareAndStoreAttribute(GetPosition().DocLength(), attr[i]);
        }
    }
    void CloseZone(const TString& zoneName) {
        Creator.StoreZone(DTZoneSearch|DTZoneText, zoneName.data(), GetPosition().DocLength(), false);
    }
    void StoreText(const wchar16* text, size_t len, RelevLevel relev, const TAttribute* attr, size_t attrCount, const IRelevLevelCallback* callback, const void* callbackV5data) {
        size_t firstEntry = Creator.GetDirectTextEntries().GetEntryCount();
        CheckBreak();
        SentAttr = attr;
        SentAttrCount = attrCount;
        DoStoreText(text, len, relev, callback);
        SentAttr = nullptr;
        SentAttrCount = 0;

        for (IDirectTextCallback5* cb : CallbacksV5)
            Creator.ProcessDirectText(*cb, firstEntry, callbackV5data);
    }

    void AddDoc(ui32 docId, ELanguage lang);
    void CommitDoc(const TDocInfoEx* docInfo, TFullDocAttrs* docAttrs);
private:
    void OnBreak() override {
        if (!SentAttrStorage || !SentAttr || !SentAttrCount)
            return;
        for (size_t i = 0; i < SentAttrCount; ++i)
            SentAttrStorage->AddSentAttr(SentAttr[i].Name, SentAttr[i].Value);
        SentAttrStorage->CommitSentAttrs(GetPosition().Break());
    }
    // TODO: порефакторить, протащить тип в TAttribute
    void PrepareAndStoreAttribute(TPosting pos, const TAttribute& attr, EDTAttrType type = DTAttrSearchLiteral) {
        TUtf16String s = CharToWide(attr.Value, csYandex);
        Creator.StoreZoneAttr(type, attr.Name.data(), s.data(), s.size(), pos, false, false, false);
    }
    void PrepareAndStoreAttribute(TPosting pos, const TWAttribute& attr, EDTAttrType type = DTAttrSearchLiteral) {
        Creator.StoreZoneAttr(type, attr.Name.data(), attr.Value.data(), attr.Value.size(), pos, false, false, false);
    }
};

void TDirectIndex::TImpl::AddDoc(ui32 docId, ELanguage lang) {
    TDirectTokenizer::AddDoc(docId, lang);
    if (PlainArchiveCreator) {
        SentAttrStorage->Clear();
    }
    for (IDirectTextCallback5* cb : CallbacksV5)
        Creator.AddDocToCallback(*cb);
}

void TDirectIndex::TImpl::CommitDoc(const TDocInfoEx* docInfo, TFullDocAttrs* extDocAttrs) {
    TDirectTokenizer::CommitDoc();
    TFullDocAttrs tempAttrs;
    TFullDocAttrs* docAttrs = extDocAttrs ? extDocAttrs : &tempAttrs;
    if (Disamber) {
        Creator.CreateDisambMasks(*Disamber);
    }
    {
        TDocAttrInserter inserter(docAttrs);
        for (size_t i = 0; i < Callbacks.size(); i++) {
            IDirectTextCallback2* cb = Callbacks[i];
            if (docInfo)
                cb->SetCurrentDoc(*docInfo);
            Creator.ProcessDirectText(*cb, &inserter);
        }
        if (RelevMod) {
            if (docInfo)
                RelevMod->SetCurrentDoc(*docInfo);
            Creator.ApplyRelevanceModification(*RelevMod, &inserter);
        }
    }
    if (PlainArchiveCreator) {
        Y_ASSERT(docInfo);
        const TSentAttrStorage::TSentAttrs& sa = SentAttrStorage->GetSentAttrs();
        PlainArchiveCreator->SetCurrentSentAttrs(sa.data(), sa.size());
        Creator.ProcessDirectText(*PlainArchiveCreator, docInfo, docAttrs);
    }
    if (InvCreator) {
        Creator.CreateInvertIndex(*InvCreator, docInfo, docAttrs);
    }
}

TDirectIndex::TDirectIndex(TDirectTextCreator& dtc, bool backwardCompatible)
    : Impl(new TImpl(dtc, backwardCompatible))
{
}

TDirectIndex::~TDirectIndex() {
    Finish();
}

void TDirectIndex::Finish() {
    Impl->Finish();
}

void TDirectIndex::SetDisamber(IDisambDirectText* obj) {
    Y_ASSERT(obj);
    Impl->Disamber = obj;
}

void TDirectIndex::SetRelevanceModificator(IModifyDirectText* obj) {
    Y_ASSERT(obj);
    Impl->RelevMod = obj;
}

void TDirectIndex::SetInvCreator(IDirectTextCallback4* obj) {
    Y_ASSERT(obj);
    Impl->InvCreator = obj;
}

void TDirectIndex::SetArcCreator(TArchiveCreator* obj) {
    Y_ASSERT(obj);
    Impl->PlainArchiveCreator = obj;
    Impl->SentAttrStorage.Reset(new TSentAttrStorage);
}

void TDirectIndex::AddDirectTextCallback(IDirectTextCallback2* obj) {
    Y_ASSERT(obj);
    Impl->Callbacks.push_back(obj);
}

void TDirectIndex::AddDirectTextCallback(IDirectTextCallback5* obj) {
    Y_ASSERT(obj);
    Impl->CallbacksV5.push_back(obj);
}

void TDirectIndex::AddDoc(ui32 docId, ELanguage lang) {
    Impl->AddDoc(docId, lang);
}

void TDirectIndex::StoreDocAttr(const TAttribute& attr, EDTAttrType type /*= DTAttrSearchLiteral*/) {
    Impl->StoreDocAttr(attr, 0, type);
}

void TDirectIndex::StoreDocAttr(const TWAttribute& attr, EDTAttrType type /*= DTAttrSearchLiteral*/) {
    Impl->StoreDocAttr(attr, 0, type);
}

void TDirectIndex::StoreDocIntegerAttr(const TWAttribute& attr) {
    Impl->StoreDocAttr(attr, 0, DTAttrSearchInteger);
}

void TDirectIndex::StoreDocIntegerAttr(const TWAttribute& attr, TPosting pos) {
    Impl->StoreDocAttr(attr, pos, DTAttrSearchInteger);
}

void TDirectIndex::StoreDocDateTimeAttr(const TString& name, time_t value) {
    TUtf16String strvalue = ASCIIToWide(ToString(value));
    Impl->StoreDocAttr(TWAttribute(name, strvalue), 0, DTAttrSearchDate);
}

void TDirectIndex::OpenZone(const TString& zoneName) {
    Impl->OpenZone(zoneName);
}

void TDirectIndex::OpenZone(const TString& zoneName, const TAttribute& zoneAttr) {
    Impl->OpenZone(zoneName, &zoneAttr, 1);
}

void TDirectIndex::OpenZone(const TString& zoneName, const TAttributes& zoneAttrs) {
    if (zoneAttrs.empty())
        Impl->OpenZone(zoneName);
    else
        Impl->OpenZone(zoneName, zoneAttrs.data(), zoneAttrs.size());
}

void TDirectIndex::OpenZone(const TString& zoneName, const TWAttribute& zoneAttr) {
    Impl->OpenZone(zoneName, &zoneAttr, 1);
}

void TDirectIndex::OpenZone(const TString& zoneName, const TWAttributes& zoneAttrs) {
    if (zoneAttrs.empty())
        Impl->OpenZone(zoneName);
    else
        Impl->OpenZone(zoneName, zoneAttrs.data(), zoneAttrs.size());
}

void TDirectIndex::CloseZone(const TString& zoneName) {
    Impl->CloseZone(zoneName);
}

void TDirectIndex::StoreUtf8Text(const TString& text, RelevLevel relev) {
    TUtf16String seq = UTF8ToWide(text);
    Impl->StoreText(seq.data(), seq.size(), relev, nullptr, 0, nullptr, nullptr);
}

void TDirectIndex::StoreUtf8Text(const TString& text, RelevLevel relev, const TAttribute& sentAttr) {
    TUtf16String seq = UTF8ToWide(text);
    Impl->StoreText(seq.data(), seq.size(), relev, &sentAttr, 1, nullptr, nullptr);
}

void TDirectIndex::StoreUtf8Text(const TString& text, RelevLevel relev, const TAttributes& sentAttrs) {
    TUtf16String seq = UTF8ToWide(text);
    Impl->StoreText(seq.data(), seq.size(), relev, sentAttrs.data(), sentAttrs.size(), nullptr, nullptr);
}

void TDirectIndex::StoreText(const wchar16* text, size_t len, RelevLevel relev) {
    Impl->StoreText(text, len, relev, nullptr, 0, nullptr, nullptr);
}

void TDirectIndex::StoreText(const wchar16* text, size_t len, RelevLevel relev, const TAttribute& sentAttr) {
    Impl->StoreText(text, len, relev, &sentAttr, 1, nullptr, nullptr);
}

void TDirectIndex::StoreText(const wchar16* text, size_t len, RelevLevel relev, const TAttributes& sentAttrs) {
    Impl->ResetLangOptions();
    Impl->StoreText(text, len, relev, sentAttrs.data(), sentAttrs.size(), nullptr, nullptr);
}

void TDirectIndex::StoreText(const wchar16* text, size_t len, RelevLevel relev, const ::TLangMask& langMask, ELanguage lang, ui8 lemmerOptions, const TAttributes& sentAttrs, const IRelevLevelCallback* callback, const void* callbackV5data) {
    Impl->SetLangOptions(langMask, lang, lemmerOptions);
    Impl->StoreText(text, len, relev, sentAttrs.data(), sentAttrs.size(), callback, callbackV5data);
}

void TDirectIndex::IncBreak(ui32 k) {
    Impl->IncBreak(k);
}

ui16 TDirectIndex::CurrentBreak() const {
    return GetPosition().Break();
}

void TDirectIndex::NextWord() {
    Impl->NextWord();
}

TWordPosition TDirectIndex::GetPosition() const {
    return Impl->GetPosition();
}

void TDirectIndex::CommitDoc(const TDocInfoEx* docInfo, TFullDocAttrs* docAttrs) {
    Impl->CommitDoc(docInfo, docAttrs);
}

ui32 TDirectIndex::GetWordCount() const {
    return Impl->GetWordCount();
}

void TDirectIndex::SetIgnoreStoreTextBreaks(bool val) {
    Impl->IgnoreStoreTextBreaks = val;
}

bool TDirectIndex::GetIgnoreStoreTextBreaks() const {
    return Impl->IgnoreStoreTextBreaks;
}

void TDirectIndex::SetNoImplicitBreaks(bool val) {
    Impl->NoImplicitBreaks = val;
}

void TDirectIndex::SetIgnoreStoreTextNextWord(bool val) {
    Impl->IgnoreStoreTextNextWord = val;
}

bool TDirectIndex::GetIgnoreStoreTextNextWord() const {
    return Impl->IgnoreStoreTextNextWord;
}

void TDirectIndex::SetStoreTextMaxBreaks(ui32 val) {
    Impl->StoreTextMaxBreaks = val;
}

TDirectIndexWithArchive::TDirectIndexWithArchive(TDirectTextCreator& dtc, const TString& textArchive, ui32 archiveVersion, bool backwardCompatible)
    : TDirectIndex(dtc, backwardCompatible)
{
    Y_ASSERT(!!textArchive);
    OutArc.Reset(new TFixedBufferFileOutput(textArchive));
    WriteTextArchiveHeader(*OutArc, archiveVersion);

    Initialize(OutArc.Get(), archiveVersion);
}

TDirectIndexWithArchive::TDirectIndexWithArchive(TDirectTextCreator& dtc, IOutputStream* textArchive, bool useArchiveHeader, ui32 archiveVersion)
    : TDirectIndex(dtc, false)
{
    if (useArchiveHeader) {
        WriteTextArchiveHeader(*textArchive, archiveVersion);
    }

    Initialize(textArchive, archiveVersion);
}

TDirectIndexWithArchive::~TDirectIndexWithArchive() {
    Finish();
}

void TDirectIndexWithArchive::Initialize(IOutputStream* archive, ui32 archiveVersion) {
    ui32 archiveCreatorSettings = GetArchiveCreatorSetting(archiveVersion);

    SetupArcCreator(archive, archiveCreatorSettings);
}

ui32 TDirectIndexWithArchive::GetArchiveCreatorSetting(ui32 archiveVersion) {
    ui32 archiveCreatorSettings = TArchiveCreator::SaveDocHeader;

    if (archiveVersion == ARC_COMPRESSED_EXT_INFO) {
        archiveCreatorSettings |= TArchiveCreator::CompressExtInfo;
    }

    return archiveCreatorSettings;
}

void TDirectIndexWithArchive::SetupArcCreator(IOutputStream* archive, ui32 archiveCreatorSettings) {
    PlainArchiveCreator.Reset(new TArchiveCreator(*archive, archiveCreatorSettings));
    SetArcCreator(PlainArchiveCreator.Get());
}

}

