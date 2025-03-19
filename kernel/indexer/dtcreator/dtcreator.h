#pragma once

#include "dtconfig.h"

#include <kernel/indexer/direct_text/dt.h>
#include <kernel/lemmer/core/language.h>
#include <library/cpp/token/nlptypes.h>
#include <library/cpp/langmask/langmask.h>
#include <library/cpp/wordpos/wordpos.h>

#include <library/cpp/langs/langs.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>

class IDocumentDataInserter;
class TIndexLanguageOptions;
struct TDocInfoEx;
struct THtmlChunk;
class TFullDocAttrs;
class TCompatiblePureContainer;

namespace NIndexerCore {

class IDirectTextCallback2;
class IDirectTextCallback3;
class TInvCreatorDTCallback;
struct TLemmatizedTokens;
struct TDirectTextData2;
struct TDisambMask;
class TDefaultDisamber; //todo probably need to replace with interface if more disambers needed to make;

namespace NIndexerCorePrivate {
    class TLemmatizationCache;
    class TPool;
    class TDirectText2;
    struct TDirectData;
}

class TDirectTextCreator : private TNonCopyable {
private:
    TSimpleSharedPtr<TCompatiblePureContainer> Pure;
    THolder<NIndexerCorePrivate::TLemmatizationCache> LemmatizationCache; // кеширует результаты лемматизации
    THolder<NIndexerCorePrivate::TDirectText2> DirectText;                // формы и пробелы текста по-порядку
    TVector<TDisambMask> DisambMasks;
    ui32 CurDocId;
public:
    //! @param pure    can be empty pointer
    TDirectTextCreator(const TDTCreatorConfig& cfg, const TSimpleSharedPtr<TCompatiblePureContainer>& pure);
    TDirectTextCreator(const TDTCreatorConfig& cfg, const TLangMask& langMask, ELanguage lang,
        const NLemmer::TAnalyzeWordOpt* lemmerOptions = &NLemmer::TAnalyzeWordOpt::IndexerOpt(), size_t lemmerOptionsCount = 1, const TSimpleSharedPtr<TCompatiblePureContainer>* pure = nullptr);
    ~TDirectTextCreator();

    void StoreForm(const TWideToken& tok, ui32 origOffset, TPosting pos, ui8 lemmOptions = 0);
    ui32 StoreForm(const TWideToken& tok, ui32 origOffset, TPosting pos, TLemmatizedTokens** tokens, ui8 lemmOptions = 0);
    void StoreSpaces(const wchar16* spaces, ui32 len, TBreakType type);
    void StoreZoneAttr(ui8 type, const char* name, const wchar16* value, size_t length, TPosting pos, bool noFollow, bool sponsored, bool ugc);
    void StoreZone(ui8 type, const char* key, TPosting pos, bool isOpen);

    void AddDoc(ui32 docId, ELanguage lang);
    void CommitDoc();
    void CreateDisambMasks(IDisambDirectText& disamber, const TDirectTextData2* extraDirectText = nullptr);
    void ApplyRelevanceModification(IModifyDirectText& callback, IDocumentDataInserter* inserter) const;
    void AddDocToCallback(IDirectTextCallback5& callback) const;
    void ProcessDirectText(IDirectTextCallback2& callback, IDocumentDataInserter* inserter,
        const TDirectTextData2* extraDirectText = nullptr) const;
    void ProcessDirectText(IDirectTextCallback3& callback, const TDocInfoEx* docInfo, const TFullDocAttrs* docAttrs,
        const TDirectTextData2* extraDirectText = nullptr) const;
    void ProcessDirectText(IDirectTextCallback5& cb, size_t firstEntry, const void* callbackV5Data) const;
    void CreateInvertIndex(IDirectTextCallback4& callback, const TDocInfoEx* docInfo, const TFullDocAttrs* docAttrs,
        const TDirectTextData2* extraDirectText = nullptr) const;
    const NIndexerCorePrivate::TDirectText2& GetDirectText() const;
    void ClearDirectText() const;
    void ClearLemmatizationCache() const;
    void RestartLemmatizationCache();

    TIndexLanguageOptions SetLanguageOptions(const TIndexLanguageOptions& newOptions);
    void SetLemmaNormalization(bool lemmaNormalization);
    bool GetLemmaNormalization() const;

    //! returns old direct text and creates a new one
    const NIndexerCorePrivate::TDirectText2* DetachDirectText();
    TDirectTextEntries GetDirectTextEntries() const;
    const TVector<TDisambMask>& GetDisambMasks() const;

private:
    static NIndexerCorePrivate::TDirectText2* CreateDirectText(bool kiwiTrigger);
};

} // NIndexerCore
