#pragma once

#include "faceint.h"

#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/hash.h>
#include <util/generic/map.h>
#include <util/memory/segmented_string_pool.h>
#include <util/memory/segpool_alloc.h>

namespace NIndexerCore {

struct TLemmatizedToken;

namespace NIndexerCorePrivate {

class TPostingIndex {
private:
    typedef TPostings::TNode TPosNode;
    typedef TDocPostingsList::TNode TDocNode;
    typedef TMap<TWordKey, TDocPostingsList, TLess<TWordKey>> TWordHash;
    typedef TMap<TAttrKey, TDocPostingsList, TLess<TAttrKey>> TAttrHash;
private:
    segmented_pool<char>     KeyBlocks; // память для хранения строк из ключей
    segmented_pool<TPosNode> PosBlocks; // память для хранения списков позиций
    segmented_pool<TDocNode> DocBlocks; // память для хранения списков документов
    TWordHash                WordHash;
    TAttrHash                AttrHash;

public:
    TPostingIndex(size_t keySize, size_t posSize, size_t docSize);
    ~TPostingIndex();

    void FillValues(TAttrEntries& values) const;
    void FillValues(TWordEntries& values) const;

    size_t MemUsage() const;
    void Restart();

    void StoreWord(size_t docOffset, const TLemmatizedToken& ke, TPosting pos, bool stripKeysOnIndex);
    void StoreExternalLemma(size_t docOffset, const wchar16* lemma, size_t lemmaLen, const wchar16* form, size_t formLen, ui8 flags, ui8 lang, TPosting pos, bool stripKeysOnIndex);
    void StoreAttribute(size_t docOffset, const char* key, TPosting pos);

private:
    void CheckDocList(TDocPostingsList& docPostingsList, size_t docOffset);
    void InsertPosting(TDocPostingsList& docPostingsList, TPosting posting);
};

}}
