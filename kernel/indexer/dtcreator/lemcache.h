#pragma once

#include "dtconfig.h"

#include <kernel/lemmer/core/language.h>
#include <library/cpp/langmask/index_language_chooser.h>
#include <library/cpp/token/nlptypes.h>
#include <ysite/yandex/pure/pure_container.h>

#include <util/memory/pool.h>

class TCompatiblePureContainer;

namespace NIndexerCore {

struct TLemmatizedToken;

struct TLemmatizedTokens {
    TLemmatizedToken* Tokens;
    ui32 TokenCount;

    explicit TLemmatizedTokens(TLemmatizedToken* p = nullptr, ui32 n = 0)
        : Tokens(p)
        , TokenCount(n)
    {
    }
};

namespace NIndexerCorePrivate {

struct TWideTokenInfo;
class ITokenProcessor;

struct TTokenText {
    const wchar16* Text;
    ui32 Len;
    ui32 Index; // index of subtoken in composite multitoken
    ui32 Offset; // char offset of Text from the original TWideToken::Token
    ui32 Subtoks;
};

class TLemmatizationCache {
private:
    class THashedStringStorage;
    class TTokenHash;

    TMemoryPool Pool;
    size_t CfgHashSize;
    THolder<THashedStringStorage> StringStorage;
    THolder<TTokenHash> TokenHash;
    const TCompatiblePureContainer* PureContainer;
    TCompatiblePure CurrentPure;
    TVector<NLemmer::TAnalyzeWordOpt> LemmerOptions;
    TIndexLanguageOptions LanguageOptions;
    ITokenProcessor* TokenProc;
    bool LemmaNormalization;

public:
    TLemmatizationCache(const TDTCreatorConfig& cfg, const TCompatiblePureContainer* pure, const TIndexLanguageOptions& langOptions,
        const NLemmer::TAnalyzeWordOpt* lOpt, size_t lOptCount, size_t blockSize);
    ~TLemmatizationCache();
    void SetLang(ELanguage lang);
    TIndexLanguageOptions SetLanguageOptions(const TIndexLanguageOptions& newOptions);
    void SetTokenProc(ITokenProcessor* proc);

    //! when lemmer options or lemma normalization are changed the cache must be cleared
    //! number of options must be the same value that was passed to the constructor
    void SetLemmaNormalization(bool value, const NLemmer::TAnalyzeWordOpt* options, size_t count);
    bool GetLemmaNormalization() const {
        return LemmaNormalization;
    }

    //! buffers for tokens and forms must have size equal to or greater than tok.SubTokens.size()
    ui32 StoreMultiToken(const TWideToken& tok, ui8 opt, TTokenText* tokens, TLemmatizedTokens** forms);
    //! for unit-testing only
    ui32 StoreMultiToken(const TWideToken& tok, ui8 opt, TLemmatizedTokens** forms);

    size_t MemUsage() const;
    void Restart();
private:
    bool CheckHashTable(const TWideTokenInfo& tokInfo, NLP_TYPE nlpType, TTokenText& token, TLemmatizedTokens*& forms, ui8 opt = 0);
    void StoreNonLemmerToken(const TWideTokenInfo& tokInfo, TTokenText& token, TLemmatizedTokens*& forms);
    void StoreLemmerToken(const TWideTokenInfo& tokInfo, ui8 opt, TTokenText& token, TLemmatizedTokens*& forms);
    ui32 StoreCompositeMultitoken(const TWideToken& tok, ui8 opt, TTokenText* tokens, TLemmatizedTokens** forms);
    void FillNonLemmerLematizedToken(const TWideTokenInfo& tokInfo, NLP_TYPE nlpType, TLemmatizedToken& lemToken);
    void FillLemmerLematizedToken(const TWideTokenInfo& tokInfo, const TWLemmaArray& origLemmas, TLemmatizedToken* tokens);
    void LemmatizeToken(const TWideToken& tok, const TWideTokenInfo& tokInfo, const ui8 opt, TLemmatizedTokens*& tokForms);
};

} // NIndexerCorePrivate
} // NIndexerCore
