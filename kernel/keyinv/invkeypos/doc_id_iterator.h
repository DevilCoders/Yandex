#pragma once

#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/algorithm.h>
#include <util/generic/maybe.h>
#include <util/generic/vector.h>
#include <util/generic/array_ref.h>

class TDocIdIterator {
private:
    TVector<ui32> DocIds = { Max<ui32>() };
    size_t Index = 0;

public:

    TArrayRef<const ui32> GetDocIds() const {
        return TArrayRef<const ui32>(DocIds.data(), DocIds.size() ? DocIds.size() - 1 : 0);
    }

    template<typename T>
    void Init(T begin, T end) {
        if (begin == end)
            return;
        TVector<ui32> docIds;
        for (T toDocId = begin; toDocId != end; ++toDocId) {
            docIds.push_back(*toDocId);
        }
        Sort(docIds.begin(), docIds.end());
        docIds.erase(Unique(docIds.begin(), docIds.end()), docIds.end());
        for (size_t i = 0; i < docIds.size(); ++i) {
            Add(docIds[i]);
        }
    }

    TDocIdIterator() = default;

    template<typename T>
    TDocIdIterator(T begin, T end) {
        Init<T>(begin, end);
    }

    template<typename T>
    void MergeDocs(T begin, T end) {
        if (begin == end)
            return;

        /* For correct moving current position after merge */
        TMaybe<ui32> valueBeforeIndex;
        if (Index > 0) {
            valueBeforeIndex = DocIds[Index - 1];
        }

        /* Merge and Sort all docs */
        DocIds.reserve(DocIds.size() + end - begin);
        for (T toDocId = begin; toDocId != end; ++toDocId) {
            DocIds.push_back(*toDocId);
        }
        Sort(DocIds.begin(), DocIds.end());
        DocIds.resize(std::unique(DocIds.begin(), DocIds.end()) - DocIds.begin());

        /* Moving current position to the right place */
        if (valueBeforeIndex.Defined()) {
            while (DocIds[Index] <= valueBeforeIndex.GetRef()) {
                ++Index;
            }
        }
    }

    inline bool Eof() const {
        return EofDoc();
    }

    inline bool EofDoc() const {
        return Max<ui32>() == DocIds[Index];
    }

    SUPERLONG Last() const {
        if (DocIds.size() < 2)
            return 0;
        else
            return SUPERLONG((DocIds[DocIds.size() - 2]) + 1) << DOC_LEVEL_Shift;
    }

    ui32 LastDoc() const {
        if (DocIds.size() < 2)
            return 0;
        else
            return ((DocIds[DocIds.size() - 2]) + 1);
    }

    inline bool GetNextPosDoc(SUPERLONG& pos) {
        if ( (pos == FINAL_DOC_BITS && !Eof()) || (pos >= (SUPERLONG(DocIds[Index]) << DOC_LEVEL_Shift)) ) {
            pos = SUPERLONG(DocIds[Index]) << DOC_LEVEL_Shift;
            ++Index;
            return true;
        } else {
            return false;
        }
    }

    bool Contains(ui32 docId) {
        auto i = LowerBound(DocIds.begin(), DocIds.end(), docId);
        if (i == DocIds.end() || *i != docId) {
            return false;
        } else {
            return true;
        }
    }

    SUPERLONG Next(SUPERLONG pos) {
        while (pos > (SUPERLONG(DocIds[Index]) << DOC_LEVEL_Shift)) {
            ++Index;
        }
        return (SUPERLONG(DocIds[Index]) << DOC_LEVEL_Shift);
    }

    ui32 NextDoc(ui32 pos) {
        while (pos > DocIds[Index]) {
            ++Index;
        }
        return DocIds[Index];
    }

    inline void Advance() {
        Y_ASSERT(!Eof());
        ++Index;
    }

    inline SUPERLONG Peek() const {
        Y_ASSERT(!Eof());
        return SUPERLONG(DocIds[Index]) << DOC_LEVEL_Shift;
    }

    inline ui32 PeekDoc() const {
        return DocIds[Index];
    }

    bool Miss(ui32 docId) {
        if (!EofDoc() && PeekDoc() == docId)
            ++Index;
        return !EofDoc() && PeekDoc() < docId;
    }

    enum EPosType {
        ExpectedSorted = 0,
        ExpectedUnsorted = 1,
    };

    bool IsNotAcceptedDoc(ui32 pos, EPosType expectedUnsorted = ExpectedSorted) {
        // Should use per-item (not logarithmic LowerBound) compare in normal case, as it's expected to be faster
        if (expectedUnsorted) {
            auto i = LowerBound(DocIds.begin(), DocIds.end(), pos);
            return i == DocIds.end() || *i != pos;
        }
        
        while (DocIds[Index] < pos)
            ++Index;
        return DocIds[Index] != pos;
    }

    bool IsNotAccepted(SUPERLONG pos) {
        while ((SUPERLONG(DocIds[Index]) << DOC_LEVEL_Shift) < pos)
            ++Index;
        return DocIds[Index] != TWordPosition::Doc(pos);
    }

    inline void Add(ui32 docId) {
        DocIds.pop_back();
        Y_ASSERT(DocIds.empty() || (DocIds.back() < docId));
        DocIds.push_back(docId);
        DocIds.push_back(Max<ui32>());
    }

    void Reset() {
        Index = 0;
    }
};

extern TDocIdIterator EMPTY_IDS;
