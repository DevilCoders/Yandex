#pragma once

#include "faceint.h"
#include <kernel/indexer/direct_text/dt.h>
#include <kernel/indexer/face/inserter.h>

#include <kernel/keyinv/indexfile/indexstorageface.h>

#include <util/generic/hash_set.h>

class HashSet;
struct IYndexStorageFactory;
class TFullDocAttrs;

namespace NIndexerCore {

namespace NIndexerCorePrivate {
    class TPostingIndex;
    class TDocuments;
}

struct TInvCreatorConfig {
    size_t DocCount;
    size_t MaxMemory;
    bool GroupForms;
    TString StopWordFile;
    bool StripKeysOnIndex = false;
    THashSet<TString> AttrNamesToIgnore; // Attrs with such names won't be stored on call to Store*Attr
    // the following four parameters are calculated in the constructor by DocCount but they can be overridden if necessary
    size_t InternalHashSize;
    size_t KeyBlockSize;
    size_t PosBlockSize;
    size_t DocBlockSize;

    TInvCreatorConfig(size_t docCount, size_t maxMemory = 0, bool stripKeysOnIndex = false);
};

class TInvCreator : public IIndexDataInserter {
private:
    size_t MaxMemory;
    THolder<NIndexerCorePrivate::TPostingIndex> PostingIndex;
    THolder<NIndexerCorePrivate::TDocuments> Documents;
    bool FeedIds;
    size_t CurDocOffset;
    bool StripKeysOnIndex = false;
    THashSet<TString> AttrNamesToIgnore;
public:
    TInvCreator(const TInvCreatorConfig& cfg);
    ~TInvCreator() override;
    void AddDoc();

    void StoreCachedLemma(const TLemmatizedToken& ke, TPosting pos);
    void StoreZones(const char* name, const TPosting* postings, const TPosting* endOfPostings);
    void StoreAttr(ui8 attrType, const char* attrName, const wchar16* attrValue, TPosting pos);

    // IIndexDataInserter
    void StoreLiteralAttr(const char* name, const char* value, TPosting pos) override;
    void StoreLiteralAttr(const char* name, const wchar16* value, TPosting pos) override;
    void StoreDateTimeAttr(const char* name, time_t datetime) override;
    void StoreIntegerAttr(const char* name, const char* value, TPosting pos) override;
    void StoreUrlAttr(const char* name, const wchar16* value, TPosting pos) override;
    void StoreKey(const char* key, TPosting pos) override;
    void StoreZone(const char* name, TPosting beg, TPosting end) override;
    void StoreExternalLemma(const wchar16* lemma, size_t lemmaLen, const wchar16* form, size_t formLen, ui8 flags, ui8 lang, TPosting pos) override;

    void StoreDirectText(const TDirectTextData2& directText, const TDisambMask* masks, const HashSet* stopWords);

    void CommitDoc(ui64 feedId, ui32 docId);
    void MakePortion(IYndexStorageFactory* storageFactory, bool groupForms);
    bool IsFull() const;
 };

class TInvCreatorDTCallback : public IDirectTextCallback4 {
private:
    TInvCreator Creator;
    IYndexStorageFactory* StorageFactory;
    bool GroupForms;
    THolder<HashSet> StopWords;
public:
    TInvCreatorDTCallback(IYndexStorageFactory* sf, const TInvCreatorConfig& cfg);
    ~TInvCreatorDTCallback() override;
    bool ProcessDirectText(const TDirectTextData2& directText, const TDocInfoEx* docInfo, const TFullDocAttrs* docAttrs, ui32 docId, const TDisambMask* masks) override;
    bool ProcessDirectText(const TDirectTextData2& directText, const TDocInfoEx* docInfo, const TFullDocAttrs* docAttrs, ui32 docId) override {
        return ProcessDirectText(directText, docInfo, docAttrs, docId, nullptr);
    }
    void MakePortion() override {
        Creator.MakePortion(StorageFactory, GroupForms);
    }
private:
    void StoreExtSearchData(const TFullDocAttrs* docAttrs);
 };

}
