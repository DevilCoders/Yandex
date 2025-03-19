#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>

#include <kernel/indexer_iface/yandind.h>

namespace NIndexerCore {
namespace NIndexerCorePrivate {

class TDocument : TNonCopyable {
public:
    TDocument()
        : FeedId(0)
        , DocId(YX_NEWDOCID)
    {}
    bool operator < (const TDocument& x) const {
        return FeedId < x.FeedId || (FeedId == x.FeedId && DocId < x.DocId);
    }
    void Clear() {
        FeedId = 0;
        DocId = YX_NEWDOCID;
    }
public:
    ui64 FeedId;
    ui32 DocId;
};

class TDocuments {
private:
    TVector<TDocument> DocArray;
    size_t Count;
public:
    TDocuments(size_t docCount)
        : DocArray(docCount)
        , Count(0)
    {
    }

    void AddDoc() {
        Y_ASSERT(Count < DocArray.size());
        Count++;
    }

    size_t GetLastOffset() const {
        Y_ASSERT(Count > 0);
        return Count - 1;
    }

    void CommitDoc(ui32 docid) {
        CommitDoc(0, docid);
    }
    void CommitDoc(ui64 feedId, ui32 docId) {
        Y_ASSERT(docId != YX_NEWDOCID);
        Y_ASSERT(Count > 0);
        TDocument& doc = DocArray[Count - 1];
        doc.FeedId = feedId;
        doc.DocId = docId;
    }
    size_t Size() const {
        return Count;
    }
    bool IsFull() const {
        return Count >= DocArray.size();
    }
    size_t MemUsage() const {
        return DocArray.size()*sizeof(TDocument);
    }
    ui64 GetFeedId(size_t i) const {
        Y_ASSERT(i < Count);
        return DocArray[i].FeedId;
    }
    ui32 GetDocId(size_t i) const {
        Y_ASSERT(i < Count);
        return DocArray[i].DocId;
    }
    void Clear() {
        for (size_t i = 0; i < Count; i++) {
            DocArray[i].Clear();
        }
        Count = 0;
    }
};

}}
