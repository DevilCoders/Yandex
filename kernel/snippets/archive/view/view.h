#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSnippets {
    class TSentsOrder;
    class TArchiveSent;
    class TArchiveView {
    public:
        typedef const TArchiveSent* TSentPtr;
    private:
        typedef TVector<TSentPtr> TSents;
        TSents Sents;
    public:
        TArchiveView() {
        }
        void PushBack(TSentPtr p) {
            Y_ASSERT(!!p);
            Sents.push_back(p);
        }
        void Clear() {
            Sents.clear();
        }
        void Sort();
        bool Empty() const {
            return Sents.empty();
        }
        size_t Size() const {
            return Sents.size();
        }
        TSentPtr Get(size_t i) const {
            return Sents[i];
        }
        void Reset(size_t i, TSentPtr p) {
            Sents[i] = p;
        }
        void Resize(size_t n) {
            Sents.resize(n);
        }
        TSentPtr Front() const {
            return Sents.front();
        }
        TSentPtr Back() const {
            return Sents.back();
        }
        void InsertAllBack(const TArchiveView& other) {
            Sents.insert(Sents.end(), other.Sents.begin(), other.Sents.end());
        }
        void Swap(TArchiveView& other) {
            Sents.swap(other.Sents);
        }
        void Merge(const TArchiveView& other);
    };
    void DumpResultCopy(const TArchiveView& view, TVector<TUtf16String>& res);
    void DumpIds(const TArchiveView& view, TVector<int>& res);
    class TArchiveViews {
    public:
        typedef TVector<TArchiveView> TViews;
        TViews Views;
    };
    TArchiveView GetFirstSentences(const TArchiveView& view, int maxSize);
}
