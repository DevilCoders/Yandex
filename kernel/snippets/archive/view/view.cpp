#include "view.h"
#include "storage.h"

#include <util/generic/algorithm.h>

namespace NSnippets {
    inline bool LsId(const TArchiveSent* a, const TArchiveSent* b) {
        return a->SentId < b->SentId;
    }
    void TArchiveView::Sort() {
        ::Sort(Sents.begin(), Sents.end(), LsId);
    }
    void TArchiveView::Merge(const TArchiveView& other) {
        Y_ASSERT((!Empty() && !other.Empty()) ? Front()->SourceArc == other.Front()->SourceArc : true);
        TSents res;
        res.resize(Sents.size() + other.Sents.size(), nullptr);
        TSents::iterator end = SetUnion(Sents.begin(), Sents.end(), other.Sents.begin(), other.Sents.end(), res.begin(), LsId);
        Sents.assign(res.begin(), end);
    }
    void DumpResultCopy(const TArchiveView& view, TVector<TUtf16String>& res) {
        res.reserve(view.Size());
        for (size_t i = 0; i < view.Size(); ++i) {
            res.push_back(TUtf16String(view.Get(i)->Sent.data(), view.Get(i)->Sent.size()));
        }
    }
    void DumpIds(const TArchiveView& view, TVector<int>& res) {
        res.reserve(view.Size());
        for (size_t i = 0; i < view.Size(); ++i) {
            res.push_back(view.Get(i)->SentId);
        }
    }
    TArchiveView GetFirstSentences(const TArchiveView& view, int maxSize)
    {
        TArchiveView res;
        int currentSymbolLength = 0;
        for (size_t i = 0; i != view.Size(); ++i) {
            if (currentSymbolLength > maxSize) {
                break;
            }
            res.PushBack(view.Get(i));
            currentSymbolLength += view.Get(i)->Sent.size();
        }

        return res;
    }
}
