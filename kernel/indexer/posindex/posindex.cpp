#include "posindex.h"
#include <kernel/indexer/direct_text/fl.h>
#include <library/cpp/token/charfilter.h>
#include <kernel/keyinv/invkeypos/keyconv.h>
#include <kernel/keyinv/invkeypos/keycode.h>
#include <kernel/search_types/search_types.h>
#include <library/cpp/token/nlptypes.h>
#include <ysite/yandex/common/prepattr.h>
#include <util/system/maxlen.h>

namespace NIndexerCore {
namespace NIndexerCorePrivate {

TPostingIndex::TPostingIndex(size_t keySize, size_t posSize, size_t docSize)
    : KeyBlocks(keySize/*, "TPostingIndex::KeyBlocks"*/)
    , PosBlocks(posSize/*, "TPostingIndex::PosBlocks"*/)
    , DocBlocks(docSize/*, "TPostingIndex::DocBlocks"*/)
    , WordHash()
    , AttrHash()
{
    KeyBlocks.alloc_first_seg();
    PosBlocks.alloc_first_seg();
    DocBlocks.alloc_first_seg();
}

TPostingIndex::~TPostingIndex() {
}

void TPostingIndex::FillValues(TAttrEntries& values) const {
    if (!AttrHash.empty()) {
        values.resize(AttrHash.size());
        size_t index = 0;
        for (TAttrHash::const_iterator ei = AttrHash.begin(); ei != AttrHash.end(); ++ei)
            values[index++] = &(*ei);
    }
}

void TPostingIndex::FillValues(TWordEntries& values) const {
    if (!WordHash.empty()) {
        values.resize(WordHash.size());
        size_t index = 0;
        for (TWordHash::const_iterator ei = WordHash.begin(); ei != WordHash.end(); ++ei)
            values[index++] = &(*ei);
    }
}

size_t TPostingIndex::MemUsage() const {
    return KeyBlocks.size() + PosBlocks.size() + DocBlocks.size()
        + WordHash.size() * sizeof(TWordHash::value_type)
        + AttrHash.size() * sizeof(TAttrHash::value_type);
}

void TPostingIndex::Restart() { // память накопленная до предыдущего сброса на диск, используется повторно
    KeyBlocks.restart();
    PosBlocks.restart();
    DocBlocks.restart();

    WordHash.clear();
    AttrHash.clear();
}

inline
void TPostingIndex::CheckDocList(TDocPostingsList& docPostingsList, size_t docOffset) {
    if (docPostingsList.Empty() || docPostingsList.Back().DocOffset != docOffset) {
        TDocNode* node = DocBlocks.get_raw();
        new (&node->Item) TDocPostings;
        node->Item.DocOffset = docOffset;
        node->Item.Postings = TPostings();
        docPostingsList.PushBack(node);
    }
}

inline
void TPostingIndex::InsertPosting(TDocPostingsList& docPostingsList, TPosting posting) {
    TDocPostings& de = docPostingsList.Back();
    TPosNode* node = PosBlocks.get_raw();
    node->Item = posting;
    de.Postings.PushBack(node);
}

void TPostingIndex::StoreAttribute(size_t docOffset, const char* key, TPosting pos) {
    std::pair<TAttrKey, TDocPostingsList> value;
    value.first.Key = key;
    std::pair<TAttrHash::iterator, bool> ins = AttrHash.insert(value);
    if (ins.second) {
        TAttrKey& insertedKey = const_cast<TAttrKey&>(ins.first->first);
        insertedKey.Key = KeyBlocks.append(key, strlen(key) + 1);
    }
    TDocPostingsList& docPostingsList = ins.first->second;
    CheckDocList(docPostingsList, docOffset);
    InsertPosting(docPostingsList, pos);
}

void TPostingIndex::StoreWord(size_t docOffset, const TLemmatizedToken& tok, TPosting pos, bool stripKeysOnIndex) {
    std::pair<TWordKey, TDocPostingsList> value;
    TWordKey& ke1 = value.first;
    ke1.LemmaText = tok.LemmaText;
    ke1.FormaText = tok.FormaText;
    ke1.Flags = tok.Flags;
    ke1.Joins = tok.Joins;
    ke1.Lang = tok.Lang;
    if (stripKeysOnIndex) {
        ConvertFlagsToStrippedFormat(&ke1.Flags, &ke1.Joins, &ke1.Lang);
    }
    std::pair<TWordHash::iterator, bool> ins1 = WordHash.insert(value);
    // эти строки сохранены в кеше лемматизации
    //if (ins.second) {
    //    TWordKey& insertedKey = const_cast<TWordKey&>(ins.first->first);
    //    insertedKey.LemmaText = KeyBlocks.append(ke.LemmaText, strlen(ke.LemmaText) + 1);
    //    insertedKey.FormaText = KeyBlocks.append(ke.FormaText, strlen(ke.FormaText) + 1);
    //}
    TDocPostingsList& docPostingsList = ins1.first->second;
    CheckDocList(docPostingsList, docOffset);
    const TPosting curPos = PostingInc(pos, tok.FormOffset);
    Y_ASSERT(TWordPosition::Break(pos) == TWordPosition::Break(curPos)); // all subtokens of multitoken must be in the same sentence
    InsertPosting(docPostingsList, curPos);

    if (tok.Prefix) {
        std::pair<TWordKey, TDocPostingsList> prefixValue;
        TWordKey& ke2 = prefixValue.first;
        char buf[PUNCT_PREFIX_BUF];
        EncodeWordPrefix(tok.Prefix, buf);
        ke2.LemmaText = buf;
        ke2.FormaText = buf;
        ke2.Flags = FORM_HAS_JOINS;
        ke2.Joins = FORM_RIGHT_JOIN;
        if (stripKeysOnIndex) {
            ConvertFlagsToStrippedFormat(&ke2.Flags, &ke2.Joins, &ke2.Lang);
        }
        std::pair<TWordHash::iterator, bool> ins2 = WordHash.insert(prefixValue);
        if (ins2.second) {
            TWordKey& insertedKey = const_cast<TWordKey&>(ins2.first->first);
            insertedKey.LemmaText = KeyBlocks.append(ke2.LemmaText, strlen(ke2.LemmaText) + 1);
            insertedKey.FormaText = KeyBlocks.append(ke2.FormaText, strlen(ke2.FormaText) + 1);
        }
        TDocPostingsList& prefixPostings = ins2.first->second;
        CheckDocList(prefixPostings, docOffset);
        InsertPosting(prefixPostings, curPos);
    }
}

inline bool CharactersValid(const wchar16* p, size_t n) {
    const wchar16* e = p + n;
    while (p != e) {
        const ui32 c = *p;
        if (c <= 0x20 || c == 0x7F)
            return false;
        ++p;
    }
    return true;
}

void TPostingIndex::StoreExternalLemma(size_t docOffset, const wchar16* lemma, size_t lemmaLen, const wchar16* form, size_t formLen, ui8 flags, ui8 lang, TPosting pos, bool stripKeysOnIndex) {
    wchar16 buf[MAXKEY_BUF];
    size_t n = NormalizeUnicode(lemma, lemmaLen, buf, MAXKEY_LEN);
    buf[n] = 0;
    if(ui64 num = 0; TryFromString(TWtringBuf{lemma, n}, num)) {
        wchar16 buf2[MAXWORD_LEN];
        n = PrepareNonLemmerToken(NLP_TYPE::NLP_INTEGER, buf, buf2);
        buf2[n] = 0;
        MemCopy(buf, buf2, MAXWORD_LEN);
    }

    if (!CharactersValid(buf, n) || !CharactersValid(form, formLen))
        return; // throw?

    char lemmabuf[MAXKEY_BUF];
    TFormToKeyConvertor lemmaConv(lemmabuf, MAXKEY_BUF);
    lemmaConv.Convert(buf, n);

    char formbuf[MAXWORD_BUF];
    TFormToKeyConvertor formConv(formbuf, MAXWORD_BUF);
    formConv.Convert(form, formLen);

    std::pair<TWordKey, TDocPostingsList> value;
    TWordKey& ke = value.first;
    ke.LemmaText = lemmabuf;
    ke.FormaText = formbuf;
    ke.Flags = flags;
    ke.Joins = 0;
    ke.Lang = lang;
    if (stripKeysOnIndex) {
        ConvertFlagsToStrippedFormat(&ke.Flags, &ke.Joins, &ke.Lang);
    }
    std::pair<TWordHash::iterator, bool> ins = WordHash.insert(value);
    if (ins.second) {
        TWordKey& insertedKey = const_cast<TWordKey&>(ins.first->first);
        insertedKey.LemmaText = KeyBlocks.append(ke.LemmaText, strlen(ke.LemmaText) + 1);
        insertedKey.FormaText = KeyBlocks.append(ke.FormaText, strlen(ke.FormaText) + 1);
    }
    TDocPostingsList& docPostingsList = ins.first->second;
    CheckDocList(docPostingsList, docOffset);
    InsertPosting(docPostingsList, pos);
}

}}
