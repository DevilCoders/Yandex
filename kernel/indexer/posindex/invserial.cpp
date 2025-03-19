#include <kernel/search_types/search_types.h>
#include "invcreator.h"
#include "posindex.h"
#include "docdata.h"

#include <kernel/indexer/face/docinfo.h>
#include <kernel/keyinv/indexfile/indexstorageface.h>
#include <kernel/keyinv/invkeypos/keyconv.h>
#include <kernel/keyinv/invkeypos/keycode.h>
#include <library/cpp/wordpos/wordpos.h>
#include <ysite/yandex/common/prepattr.h>

#include <util/generic/noncopyable.h>
#include <util/datetime/base.h>
#include <library/cpp/containers/mh_heap/mh_heap.h>
#include <util/generic/algorithm.h>
#include <util/generic/strfcpy.h>
#include <library/cpp/containers/str_hash/str_hash.h>
#include <util/stream/file.h>

namespace NIndexerCore {

namespace NIndexerCorePrivate {

inline void FillSortedWords(TPostingIndex* postingIndex, TDocuments* documents, TWordEntryWithFeedIds& wordEntries) {
    TWordEntries tempEntries;
    postingIndex->FillValues(tempEntries);
    TVector<ui64> feeds;
    for (TWordEntries::const_iterator it = tempEntries.begin(); it != tempEntries.end(); ++it) {
        const TWordEntry* w = *it;
        const TDocPostingsList& docs = w->second;
        for (TDocPostingsList::TConstIterator d = docs.Begin(); d != docs.End(); ++d) {
            const ui64 feedid = documents->GetFeedId(d->DocOffset);
            feeds.push_back(feedid);
        }
        Sort(feeds.begin(), feeds.end());
        TVector<ui64>::iterator last = Unique(feeds.begin(), feeds.end());
        for (TVector<ui64>::iterator feed = feeds.begin(); feed != last; ++feed)
            wordEntries.push_back(TWordEntryWithFeedId(*feed, w));
        feeds.clear();
    }
}

inline void FillSortedAttrs(TPostingIndex* postingIndex, TDocuments* documents, TAttrEntryWithFeedIds& attrEntries) {
    TAttrEntries tempEntries;
    postingIndex->FillValues(tempEntries);
    TVector<ui64> feeds;
    for (TAttrEntries::const_iterator it = tempEntries.begin(); it != tempEntries.end(); ++it) {
        const TAttrEntry* a = *it;
        if (a->first.Key[0] == ATTR_PREFIX && a->first.Key[1] == ATTR_PREFIX) { // ##_DOC_ID="1 or ##_DOC_IDF_SUM
            attrEntries.push_back(TAttrEntryWithFeedId(0, a));
        } else {
            const TDocPostingsList& docs = a->second;
            for (TDocPostingsList::TConstIterator d = docs.Begin(); d != docs.End(); ++d) {
                const ui64 feedid = documents->GetFeedId(d->DocOffset);
                feeds.push_back(feedid);
            }
            Sort(feeds.begin(), feeds.end());
            TVector<ui64>::iterator last = Unique(feeds.begin(), feeds.end());
            for (TVector<ui64>::iterator feed = feeds.begin(); feed != last; ++feed)
                attrEntries.push_back(TAttrEntryWithFeedId(*feed, a));
            feeds.clear();
        }
    }
}

class TInvSerializer : TNonCopyable {
private:
    typedef TVector<i64> TMergeResult;
private:
    const TDocuments& Documents;
    TMergeResult MergeRes;
    bool NeedSort;
public:
    TInvSerializer(const TDocuments& documents)
        : Documents(documents)
        , NeedSort(false)
    {
    }
    ui32 GetDocId(size_t offset) {
        return Documents.GetDocId(offset);
    }
    ui64 GetDocFeed(size_t offset) {
        return Documents.GetFeedId(offset);
    }

    template <int N_FORMS_PER_KISHKA, typename TSortedLemmas>
    void WriteLemmatizedWordPoses(IYndexStorage& storage, const TSortedLemmas& sortedLemmas, bool stripKeysOnIndex);
    void WriteAttrs(IYndexStorage& storage, const TAttrEntries& sortedAttrs);
    void WriteAttrs(IYndexStorage& storage, const TAttrEntryWithFeedIds& sortedAttrs);
    void StoreUniqPosting(ui32 docId, TPosting p);
private:
    void StorePosting(ui32 docId, TPosting p);
    void StoreAttrs(ui64 feedId, const TDocPostingsList& dc);
    void StoreMergeRes(IYndexStorage& storage, const char* key);
};

inline void TInvSerializer::StorePosting(ui32 docId, TPosting p) {
    Y_ASSERT(docId != YX_NEWDOCID);
    SUPERLONG newWP = ((SUPERLONG)docId) << DOC_LEVEL_Shift;
    newWP |= p;
    if (!MergeRes.empty() && newWP < MergeRes.back())
        NeedSort = true;
    MergeRes.push_back(newWP);
}

inline void TInvSerializer::StoreUniqPosting(ui32 docId, TPosting p) {
    SUPERLONG newWP = ((SUPERLONG)docId) << DOC_LEVEL_Shift;
    newWP |= p;
    if (!MergeRes.empty()) {
        SUPERLONG oldWP = MergeRes.back();
        if (newWP == oldWP)
            return;
        else if (newWP < oldWP)
            NeedSort = true;
    }
    MergeRes.push_back(newWP);
}

inline void TInvSerializer::StoreAttrs(ui64 feedId, const TDocPostingsList& dc) {
    for (TDocPostingsList::TConstIterator di = dc.Begin(); di != dc.End(); ++di) {
        ui32 docId = Documents.GetDocId((*di).DocOffset);
        if (feedId && Documents.GetFeedId(di->DocOffset) != feedId)
            continue;
        const TPostings& postings = (*di).Postings;
        if (!postings.Empty()) {
            for (TPostings::TConstIterator it = postings.Begin(); it != postings.End(); ++it) {
                StorePosting(docId, *it);
            }
        }
    }
}

inline void TInvSerializer::StoreMergeRes(IYndexStorage& storage, const char* key) {
    if (!MergeRes.empty()) {
        if (NeedSort)
            Sort(MergeRes.begin(), MergeRes.end());
        storage.StorePositions(key, &MergeRes[0], MergeRes.size());
        MergeRes.erase(MergeRes.begin(), MergeRes.end());
    }
    NeedSort = false;
}

void TInvSerializer::WriteAttrs(IYndexStorage& storage, const TAttrEntries& sortedAttrs) {
    for (TAttrEntries::const_iterator k = sortedAttrs.begin(); k != sortedAttrs.end(); ++k) {
        const TAttrEntry* ent = *k;
        StoreAttrs(0, ent->second);
        StoreMergeRes(storage, ent->first.Key);
    }
}

namespace {
    inline bool InsertFeedID(ui64 feedID, const char* key, char* buffer) {
        Y_ASSERT(feedID);
        size_t p = 0;
        buffer[p++] = key[0];
        p += EncodePrefix(feedID, &buffer[p]);
        buffer[p++] = KEY_PREFIX_DELIM;
        Y_ASSERT(p < MAXKEY_BUF);
        const size_t maxChars = MAXKEY_BUF - p;
        const bool hasTruncated = strlcpy(&buffer[p], &key[1], maxChars) >= maxChars;
        return hasTruncated; //false means success
    }

    inline bool MaySkipAttrIfTooLong(const char* key) {
        return !strncmp(key, "#link", 5) || !strncmp(key, "#img", 4);
    }
    inline bool MayBeTruncatedIfTooLong(const char* key) {
        // An attribute may be added here only if it is unique (not more than one value per document)
        return !strncmp(key, "#url", 4);
    }
}

void TInvSerializer::WriteAttrs(IYndexStorage& storage, const TAttrEntryWithFeedIds& sortedAttrs) {
    for (TAttrEntryWithFeedIds::const_iterator k = sortedAttrs.begin(); k != sortedAttrs.end(); ++k) {
        const ui64 feedId = k->FeedId;
        const TAttrEntry* ent = k->AttrEntry;
        if (feedId) {
            char buf[MAXKEY_BUF];
            const bool hasTruncated = InsertFeedID(feedId, ent->first.Key, buf);
            if (Y_UNLIKELY(hasTruncated)) {
                if (MaySkipAttrIfTooLong(ent->first.Key))
                    continue;
                if (!MayBeTruncatedIfTooLong(ent->first.Key)) {
                    Y_VERIFY_DEBUG(0, "attribute is too long (%zu), will be truncated: %s", strlen(ent->first.Key), ent->first.Key);
                    Clog << "WriteAttrs: attribute is too long (" << strlen(ent->first.Key) << "), will be truncated: "
                         << TStringBuf(ent->first.Key, Min<size_t>(strlen(ent->first.Key), 32)) << Endl;
                }
            }
            StoreAttrs(feedId, ent->second);
            StoreMergeRes(storage, buf);
        } else {
            StoreAttrs(0, ent->second);
            StoreMergeRes(storage, ent->first.Key);
        }
    }
}

inline ui64 GetFeedId(const TWordEntry*) {
    return 0;
}

inline ui64 GetFeedId(const TWordEntryWithFeedId& x) {
    return x.FeedId;
}

inline const TWordEntry& GetWordEntry(const TWordEntry* p) {
    return *p;
}

inline const TWordEntry& GetWordEntry(const TWordEntryWithFeedId& p) {
    return *p.WordEntry;
}

template <typename TIter>
struct TPortionOutputKey {
    TIter Beg;
    TIter End;
    char Key[MAXKEY_BUF];

    TPortionOutputKey() {
        Key[0] = 0;
    }

    TPortionOutputKey(TIter beg, TIter end, const char* psz)
        : Beg(beg)
        , End(end)
    {
        strcpy(Key, psz);
    }
};

template <typename TIter>
struct TPortionOutputKeyCmp {
    bool operator()(const TPortionOutputKey<TIter> &a, const TPortionOutputKey<TIter> &b) const {
        return strcmp(a.Key, b.Key) < 0;
    }
};

class TWordFormEnumerator {
private:
typedef TRestartableIlistIterator<TDocPostings> TDocIter;
struct TDocIterLess {
    bool operator()(const TDocIter* x, const TDocIter* y) const {
        return x->operator *().DocOffset < y->operator *().DocOffset;
    }
};
typedef TRestartableIlistIterator<TPosting> TPosIter;
struct TPosIterLess {
    bool operator()(const TPosIter* x, const TPosIter* y) const {
        return (x->operator*()) < (y->operator*());
    }
};
private:
    TDocIter DocIters[N_MAX_FORMS_PER_KISHKA];
    TDocIter *PDocIters[N_MAX_FORMS_PER_KISHKA];
    TPosIter PosIters[N_MAX_FORMS_PER_KISHKA];
    TPosIter *PPosIters[N_MAX_FORMS_PER_KISHKA];
    int WordFormForIter[N_MAX_FORMS_PER_KISHKA]; // map iterator number to word form

    TInvSerializer& InvSerializer;

private:
    void WalkOneDocPosting(ui32 formCount, ui32 curDocId) {
        if (formCount > 1) {
            MultHitHeap<TPosIter, TPosIterLess> posHeap(PPosIters, formCount);
            DoWalk(posHeap, curDocId);
        } else if (formCount == 1) {
            DoWalk(PosIters[0], curDocId);
        }
    }
    template <class Iter>
    void DoWalk(Iter& heap, ui32 curDocId) {
        heap.Restart();
        for (; heap.Valid(); ++heap) {
            TPosIter* pTopIter = heap.TopIter();
            TPosting p = **pTopIter;
            int nIter = int(pTopIter - PosIters);
            int nWordForm = WordFormForIter[nIter];
            SetPostingNForm(&p, nWordForm);
            InvSerializer.StoreUniqPosting(curDocId, p);
        }
    }
public:
    TWordFormEnumerator(TInvSerializer& is)
        : InvSerializer(is)
    {
    }

    template <typename TLemmaIterator>
    void EnumWordForms(ui64 feedId, const TLemmaIterator& beg, const TLemmaIterator& end) {
        int keyFormCount = 0;
        for (TLemmaIterator it = beg; it != end; ++it) {
            const TDocPostingsList& formPostings = GetWordEntry(*it).second;
            if (formPostings.Empty())
                continue;
            PDocIters[keyFormCount] = &DocIters[keyFormCount];
            DocIters[keyFormCount].SetContainer(&formPostings);
            ++keyFormCount;
        }
        Y_ASSERT(keyFormCount <= (int)N_MAX_FORMS_PER_KISHKA);
        // enumerate all forms for each document
        MultHitHeap<TDocIter, TDocIterLess> docHeap(PDocIters, keyFormCount);
        docHeap.Restart();
        size_t curOffset = 0xffffffff;
        bool entryAllowed = false;
        ui32 formCount = 0;
        while (docHeap.Valid()) {
            TDocIter* pTopIter = docHeap.TopIter();
            const TDocPostings& de = **pTopIter;
            if (de.DocOffset != curOffset) {
                if (curOffset != 0xffffffff) {
                    Y_ASSERT(curOffset < de.DocOffset);
                    WalkOneDocPosting(formCount, InvSerializer.GetDocId(curOffset));
                }
                formCount = 0;
                curOffset = de.DocOffset;
                entryAllowed = (InvSerializer.GetDocFeed(curOffset) == feedId);
            }
            if (entryAllowed) {
                const int nForm = int(pTopIter - DocIters);
                PPosIters[formCount] = &PosIters[formCount];
                PosIters[formCount].SetContainer(&de.Postings);
                WordFormForIter[formCount] = nForm;
                ++formCount;
            }
            ++docHeap;
        }
        WalkOneDocPosting(formCount, InvSerializer.GetDocId(curOffset));
    }
};

// TODO: optimize this for N_FORMS_PER_KISHKA==1
template <int N_FORMS_PER_KISHKA, typename TSortedLemmas>
void TInvSerializer::WriteLemmatizedWordPoses(IYndexStorage& storage, const TSortedLemmas& sortedLemmas, bool stripKeysOnIndex) {
    typedef typename TSortedLemmas::const_iterator TLemmaIterator;
    TVector<TPortionOutputKey<TLemmaIterator> > outputKeys;

    const int N_KEY_BUF_SIZE = MAXKEY_BUF * 2;
    const int N_MAX_KEY_LENGTH = MAXKEY_BUF - MAXATTRNAME_BUF - 2;
    char currentKey[N_KEY_BUF_SIZE];
    char formTexts[N_FORMS_PER_KISHKA][MAXKEY_BUF];
    const char* formTextPtrs[N_FORMS_PER_KISHKA];
    for (int i = 0; i < N_FORMS_PER_KISHKA; ++i)
        formTextPtrs[i] = formTexts[i];

    TLemmaIterator it = sortedLemmas.begin();
    TLemmaIterator beg = it;
    unsigned formInKeyCount = 0;
    while (it != sortedLemmas.end()) {
        const ui64 feedId = GetFeedId(*it);
        const TWordKey& wordKey = GetWordEntry(*it).first;

        TKeyLemmaInfo keyLemma;
        if (feedId) {
            EncodePrefix(feedId, keyLemma.szPrefix);
        }

        ui8 lemmaLang = wordKey.Lang;
        if (wordKey.LemmaText) {
            strcpy(keyLemma.szLemma, wordKey.LemmaText);
            keyLemma.Lang = wordKey.Lang;
        }

        while (it != sortedLemmas.end()) {
            const TWordKey& word = GetWordEntry(*it).first;
            if (feedId != GetFeedId(*it) || lemmaLang != word.Lang || strcmp(keyLemma.szLemma, word.LemmaText) != 0) {
                break;
            }
            const TDocPostingsList& formPostings = GetWordEntry(*it).second;

            if (formPostings.Empty()) {
                ++it;
            } else {
                char* pszForm = formTexts[formInKeyCount];
                Y_ASSERT(strlen(word.FormaText) < (size_t)N_MAX_KEY_LENGTH);
                strcpy(pszForm, word.FormaText);
                if (word.Flags) {
                    int length = strlen(pszForm);
                    ui8 lang = stripKeysOnIndex ? word.Lang : static_cast<ui8>(LANG_UNK);
                    AppendFormFlags(pszForm, &length, MAXKEY_BUF, word.Flags, word.Joins, lang);
                }
                if (stripKeysOnIndex) {
                    keyLemma.Lang = LANG_UNK;
                }

                ++formInKeyCount;

                int currentKeyLength = ConstructKeyWithForms(currentKey, N_KEY_BUF_SIZE, keyLemma, formInKeyCount, formTextPtrs);
                if (currentKeyLength >= N_MAX_KEY_LENGTH) {
                    if (formInKeyCount > 1)
                        currentKeyLength = ConstructKeyWithForms(currentKey, N_KEY_BUF_SIZE, keyLemma, formInKeyCount - 1, formTextPtrs);
                    if (currentKeyLength >= N_MAX_KEY_LENGTH) {
                        // major problem, too long form encountered, skip this lemma
                        outputKeys.clear();
                        formInKeyCount = 0;
                        ++it;
                        beg = it;
                        break;
                    }
                    outputKeys.push_back(TPortionOutputKey<TLemmaIterator>(beg, it, currentKey));
                    beg = it;
                    formInKeyCount = 0;
                } else {
                    ++it;
                    if (formInKeyCount == N_FORMS_PER_KISHKA) {
                        outputKeys.push_back(TPortionOutputKey<TLemmaIterator>(beg, it, currentKey));
                        beg = it;
                        formInKeyCount = 0;
                    }
                }
            }
        }

        if (formInKeyCount) {
            outputKeys.push_back(TPortionOutputKey<TLemmaIterator>(beg, it, currentKey));
            beg = it;
            formInKeyCount = 0;
        }

        if (!outputKeys.empty()) {
            Sort(outputKeys.begin(), outputKeys.end(), TPortionOutputKeyCmp<TLemmaIterator>());
            for (size_t i = 0; i < outputKeys.size(); ++i) {
                const TPortionOutputKey<TLemmaIterator>& k = outputKeys[i];
                TWordFormEnumerator wordEnum(*this);
                wordEnum.EnumWordForms(feedId, k.Beg, k.End);
                StoreMergeRes(storage, k.Key);
            }
            outputKeys.clear();
        }
    }
}

struct TWordLess {
    bool operator()(const TWordEntry* x, const TWordEntry* y) const {
        const int n = strcmp(x->first.LemmaText, y->first.LemmaText);
        if (n < 0)
            return true;
        if (n > 0)
            return false;
        return (x->first.Lang < y->first.Lang);
    }
    bool operator()(const TWordEntryWithFeedId& x, const TWordEntryWithFeedId& y) const {
        Y_ASSERT((x.FeedId && y.FeedId) || (!x.FeedId && !y.FeedId));
        if (x.FeedId < y.FeedId)
            return true;
        if (x.FeedId != y.FeedId)
            return false;
        return operator()(x.WordEntry, y.WordEntry);
    }
};

struct TAttrLess {
    // Sorting by: 1. Key[0] 2. FeedID 3. &Key[1]
    bool operator()(const TAttrEntry* x, const TAttrEntry* y) const {
        return (strcmp(x->first.Key, y->first.Key) < 0);
    }
    bool operator()(const TAttrEntryWithFeedId& x, const TAttrEntryWithFeedId& y) const {
        return (x.AttrEntry->first.Key[0] < y.AttrEntry->first.Key[0]) ||
               (x.AttrEntry->first.Key[0] == y.AttrEntry->first.Key[0] && (x.FeedId < y.FeedId ||
               (x.FeedId == y.FeedId && strcmp(&x.AttrEntry->first.Key[1], &y.AttrEntry->first.Key[1]) < 0)));
    }
};

}

using namespace NIndexerCorePrivate;

void TInvCreator::MakePortion(IYndexStorageFactory* storageFactory, bool groupForms) {
    if (Documents->Size() == 0)
        return;
    TInvSerializer serializer(*Documents);
    if (FeedIds) {
        TWordEntryWithFeedIds sortedWords;
        FillSortedWords(PostingIndex.Get(), Documents.Get(), sortedWords);
        if (!sortedWords.empty()) {
            Sort(sortedWords.begin(), sortedWords.end(), TWordLess());
            y_auto_storage storage(storageFactory, IYndexStorage::PORTION_FORMAT_LEMM);
            if (groupForms)
                serializer.WriteLemmatizedWordPoses<N_MAX_FORMS_PER_KISHKA>(*storage, sortedWords, StripKeysOnIndex);
            else
                serializer.WriteLemmatizedWordPoses<1>(*storage, sortedWords, StripKeysOnIndex);
        }

        TAttrEntryWithFeedIds attrEntries;
        FillSortedAttrs(PostingIndex.Get(), Documents.Get(), attrEntries);
        if (!attrEntries.empty()) {
            Sort(attrEntries.begin(), attrEntries.end(), TAttrLess());
            y_auto_storage storage(storageFactory, IYndexStorage::PORTION_FORMAT_ATTR);
            serializer.WriteAttrs(*storage, attrEntries);
        }
    } else {
        TWordEntries sortedWords;
        PostingIndex->FillValues(sortedWords);
        if (!sortedWords.empty()) {
            y_auto_storage storage(storageFactory, IYndexStorage::PORTION_FORMAT_LEMM);
            if (groupForms)
                serializer.WriteLemmatizedWordPoses<N_MAX_FORMS_PER_KISHKA>(*storage, sortedWords, StripKeysOnIndex);
            else
                serializer.WriteLemmatizedWordPoses<1>(*storage, sortedWords, StripKeysOnIndex);
        }

        TAttrEntries attrEntries;
        PostingIndex->FillValues(attrEntries);
        if (!attrEntries.empty()) {
            y_auto_storage storage(storageFactory, IYndexStorage::PORTION_FORMAT_ATTR);
            serializer.WriteAttrs(*storage, attrEntries);
        }
    }

    PostingIndex->Restart();
    Documents->Clear();
    FeedIds = false;
}

}
