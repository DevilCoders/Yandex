#include <kernel/search_types/search_types.h>
#include "dtcreator.h"
#include "lemcache.h"
#include <kernel/indexer/face/directtext.h>
#include <ysite/yandex/pure/pure_container.h>

namespace NIndexerCore {

using namespace NIndexerCorePrivate;

TDirectTextCreator::TDirectTextCreator(const TDTCreatorConfig& cfg, const TSimpleSharedPtr<TCompatiblePureContainer>& pure)
    : Pure(!pure ? new TCompatiblePureContainer(cfg.PureLangConfigFile, cfg.PureTrieFile) : pure)
    , LemmatizationCache(new TLemmatizationCache(cfg, Pure.Get(), TIndexLanguageOptions(LANG_UNK),
        &NLemmer::TAnalyzeWordOpt::IndexerOpt(), 1, cfg.CacheBlockSize))
    , DirectText(CreateDirectText(cfg.KiwiTrigger))
    , CurDocId(YX_NEWDOCID)
{
}

TDirectTextCreator::TDirectTextCreator(const TDTCreatorConfig& cfg, const TLangMask& langMask, ELanguage lang,
    const NLemmer::TAnalyzeWordOpt* lemmerOptions, size_t lemmerOptionsCount, const TSimpleSharedPtr<TCompatiblePureContainer>* pure)
    : Pure(pure ? *pure : new TCompatiblePureContainer(cfg.PureLangConfigFile, cfg.PureTrieFile))
    , LemmatizationCache(new TLemmatizationCache(cfg, Pure.Get(), TIndexLanguageOptions(langMask, lang),
        lemmerOptions, lemmerOptionsCount, cfg.CacheBlockSize))
    , DirectText(CreateDirectText(false))
    , CurDocId(YX_NEWDOCID)
{
}

TDirectTextCreator::~TDirectTextCreator()
{ }

void TDirectTextCreator::AddDoc(ui32 docId, ELanguage lang) {
    CurDocId = docId;
    LemmatizationCache->SetLang(lang);
    DisambMasks.clear();
}

TIndexLanguageOptions TDirectTextCreator::SetLanguageOptions(const TIndexLanguageOptions& newOptions) {
    return LemmatizationCache->SetLanguageOptions(newOptions);
}

void TDirectTextCreator::SetLemmaNormalization(bool lemmaNormalization) {
    LemmatizationCache->SetLemmaNormalization(lemmaNormalization, lemmaNormalization ?
        &NLemmer::TAnalyzeWordOpt::IndexerOpt() : &NLemmer::TAnalyzeWordOpt::IndexerLemmatizeAllOpt(), 1);
}

bool TDirectTextCreator::GetLemmaNormalization() const {
    return LemmatizationCache->GetLemmaNormalization();
}

void TDirectTextCreator::StoreForm(const TWideToken& tok, ui32 origOffset, TPosting pos, ui8 lemmOptions) {
    TTempArray<TLemmatizedTokens*> forms(tok.SubTokens.size());
    StoreForm(tok, origOffset, pos, forms.Data(), lemmOptions);
}

ui32 TDirectTextCreator::StoreForm(const TWideToken& tok, ui32 origOffset, TPosting pos, TLemmatizedTokens** forms, ui8 lemmOptions) {
    TTempArray<TTokenText> buf(tok.SubTokens.size());
    TTokenText* tokens = buf.Data();
    const size_t n = LemmatizationCache->StoreMultiToken(tok, lemmOptions, tokens, forms);
    for (size_t i = 0; i < n; ++i) {
        const TTokenText& token = tokens[i];
        const TLemmatizedTokens* form = forms[i];
        if (i) {
            const TTokenText& prevToken = tokens[i - 1];
            const ui32 spacePos = prevToken.Offset + prevToken.Len;
            const ui32 spaceLen = token.Offset - spacePos;
            if (spaceLen)
                DirectText->InsertSpace(tok.Token + spacePos, spaceLen, ST_NOBRK);
        }
        DirectText->InsertForm(token.Text, form->Tokens, form->TokenCount, pos, origOffset + token.Offset);
        if (i < n - 1)
            pos = PostingInc(pos, token.Subtoks);
    }
    return n;
}

static const wchar16 Space = 0x0020;

void TDirectTextCreator::StoreSpaces(const wchar16* spaces, ui32 len, TBreakType type) {
    if (!IsSentBrk(type) && spaces && *spaces == 0) {
        DirectText->InsertSpace(&Space, 1, type);
        return;
    }
    DirectText->InsertSpace(spaces, len, type);
}

void TDirectTextCreator::StoreZoneAttr(ui8 type, const char *name, const wchar16* value, size_t length, TPosting pos, bool noFollow, bool sponsored, bool ugc) {
    DirectText->StoreZoneAttr(type, name, TWtringBuf(value, length), pos, noFollow, sponsored, ugc);
}

void TDirectTextCreator::StoreZone(ui8 type, const char* key, TPosting pos, bool isOpen) {
    char zone[MAXKEY_BUF];
    size_t p = 0;
    zone[p++] = (isOpen ? OPEN_ZONE_PREFIX : CLOSE_ZONE_PREFIX);
    p += strlcpy(&zone[p], key, MAXKEY_BUF - p);
    Y_ASSERT(p < MAXKEY_BUF);
    CodePageByCharset(CODES_YANDEX)->ToLower(zone, zone);
    DirectText->StoreZone(type, zone, pos);
}

void TDirectTextCreator::CommitDoc() {
    DirectText->OnCommitDoc();
}

void TDirectTextCreator::AddDocToCallback(IDirectTextCallback5& callback) const {
    callback.AddDoc(CurDocId);
}

void TDirectTextCreator::ProcessDirectText(IDirectTextCallback2& callback,
    IDocumentDataInserter* inserter, const TDirectTextData2* extraDirectText) const
{
    TDirectData data;
    DirectText->FillDirectData(&data, extraDirectText);
    callback.ProcessDirectText2(inserter, data.DirectText, CurDocId);
}

void TDirectTextCreator::ProcessDirectText(IDirectTextCallback5& callback, size_t firstEntry, const void* callbackV5data) const {
    TDirectTextEntries entries = GetDirectTextEntries();
    size_t entryCount = entries.GetEntryCount() - firstEntry;
    callback.ProcessDirectText(entries.GetEntries() + firstEntry, entryCount, callbackV5data);
}

const TDirectText2& TDirectTextCreator::GetDirectText() const {
    return *DirectText;
}

void TDirectTextCreator::CreateDisambMasks(IDisambDirectText& disamber, const TDirectTextData2* extraDirectText)
{
    TDirectData data;
    DirectText->FillDirectData(&data, extraDirectText);
    DisambMasks.clear();//left here in case of crazy double call of masks creation
    disamber.ProcessText(data.DirectText.Entries, data.DirectText.EntryCount, &DisambMasks);
}

void TDirectTextCreator::ApplyRelevanceModification(IModifyDirectText& callback, IDocumentDataInserter* inserter) const
{
    TDirectTextEntries entries = DirectText->GetEntries();
    Y_VERIFY(DisambMasks.empty() || DisambMasks.size() == entries.GetEntryCount(), "wrong length of DisambMask : interface or memory fault");
    callback.ProcessDirectText2(inserter, entries, DisambMasks.empty() ? nullptr : &DisambMasks[0], CurDocId);
}

void TDirectTextCreator::ProcessDirectText(IDirectTextCallback3& callback, const TDocInfoEx* docInfo,
    const TFullDocAttrs* docAttrs, const TDirectTextData2* extraDirectText) const
{
    TDirectData data;
    DirectText->FillDirectData(&data, extraDirectText);
    callback.ProcessDirectText(data.DirectText, docInfo, docAttrs, CurDocId);
}

void TDirectTextCreator::CreateInvertIndex(IDirectTextCallback4& callback, const TDocInfoEx* docInfo,
    const TFullDocAttrs* docAttrs, const TDirectTextData2* extraDirectText) const
{
    TDirectData data;
    DirectText->FillDirectData(&data, extraDirectText);
    Y_VERIFY(DisambMasks.empty() || DisambMasks.size() == data.DirectText.EntryCount, "wrong lenght of DisambMask : interface or memory fault");
    callback.ProcessDirectText(data.DirectText, docInfo, docAttrs, CurDocId, DisambMasks.empty() ? nullptr: &DisambMasks[0]);
}

void TDirectTextCreator::ClearDirectText() const {
    DirectText->Clear();
    //CurDocId = YX_NEWDOCID;
}

void TDirectTextCreator::ClearLemmatizationCache() const {
    LemmatizationCache->Restart();
}

void TDirectTextCreator::RestartLemmatizationCache() {
    LemmatizationCache->Restart();
}

TDirectText2* TDirectTextCreator::CreateDirectText(bool kiwiTrigger) {
    return new TDirectText2(100, kiwiTrigger);
}

const TDirectText2* TDirectTextCreator::DetachDirectText() {
    THolder<const TDirectText2> p(DirectText.Release());
    DirectText.Reset(CreateDirectText(p->KiwiTrigger));
    return p.Release();
}

TDirectTextEntries TDirectTextCreator::GetDirectTextEntries() const {
    return DirectText->GetEntries();
}

}
