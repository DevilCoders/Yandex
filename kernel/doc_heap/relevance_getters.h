#pragma once

#include <util/system/compiler.h>

template <class TDoc>
struct TRelevanceAndDocIdGetter {
    using TRelevance = typename TDoc::TRelevanceAndDocId;

    static Y_FORCE_INLINE TRelevance Get(const TDoc& doc) {
        return doc.GetRelevanceAndDocId();
    }

    static bool Compare(const TDoc& doc1, const TDoc& doc2) {
        return Get(doc1) > Get(doc2);
    }
};

template <class TDoc>
struct TRelevanceGetter {
    using TRelevance = typename TDoc::TRelevance;

    static Y_FORCE_INLINE TRelevance Get(const TDoc& doc) {
        return doc.Relevance();
    }

    static bool Compare(const TDoc& doc1, const TDoc& doc2) {
        return Get(doc1) > Get(doc2);
    }
};
