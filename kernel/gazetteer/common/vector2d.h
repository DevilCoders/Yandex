#pragma once

#include "serialize.h"
#include "bytesize.h"

#include <library/cpp/deprecated/iter/iter.h>

#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>
#include <util/stream/str.h>



namespace NGzt {

// defined in this file:
template <typename T> class TVectorsVector;

template <typename T, typename TSubVectorTraits> class TVectorsVectorBuilder;

typedef ui32 TVectorsVectorIndex;

// Memory optimized version of TVector< TVector<T> >
// (stores all items in single vector)
template <typename T>
class TVectorsVector : TNonCopyable
{
public:
    typedef TVectorsVectorIndex TIndex;

    inline TVectorsVector() {
    }

    template <class TValues>
    TVectorsVector(const TVector<TValues>& source);

    // sub-vectors count
    inline size_t Size() const {
        return (FirstIndex.size() > 0) ? FirstIndex.size() - 1 : 0;
    }

    inline size_t TotalSize() const {
        return AllItems.size();
    }

    typedef NIter::TValueIterator<const T> TConstIter;
    inline TConstIter operator[](size_t index) const {
        Y_ASSERT(index < Size());
        return TConstIter(AllItems.begin() + FirstIndex[index],
                          AllItems.begin() + FirstIndex[index + 1]);
    }

    typedef std::pair<const T*, const T*> TRange;
    inline TRange SubVector(size_t index) const {
        return TRange(AllItems.begin() + FirstIndex[index],
                      AllItems.begin() + FirstIndex[index + 1]);
    }

    void Save(IOutputStream* output) const {
        ::Save(output, AllItems);
        ::Save(output, FirstIndex);
    }

    void Load(IInputStream* input) {
        ::Load(input, AllItems);
        ::Load(input, FirstIndex);
    }

    template <typename TItemSetsProto>
    void SaveToProto(TItemSetsProto& proto) {
        SaveToField(proto.MutableAllItems(), AllItems);
        SaveToField(proto.MutableFirstIndex(), FirstIndex);
    }

    template <typename TItemSetsProto>
    void LoadFromProto(const TItemSetsProto& proto) {
        LoadVectorFromField(proto.GetAllItems(), AllItems);
        LoadVectorFromField(proto.GetFirstIndex(), FirstIndex);
    }

    size_t ByteSize() const {
        return ::ByteSize(AllItems) + ::ByteSize(FirstIndex);
    }

    static void CheckOverflow(ui64 index) {
        if (Y_UNLIKELY(index >= (ui64)Max<TIndex>()))
            ythrow yexception() << "VectorsVector index limit is reached (" << index << ", should be less than Max<ui32>).";
    }

private:
    TVector<T> AllItems;
    TVector<TIndex> FirstIndex;   // first index of each sub-vector in AllItems, plus one index for AllItems.size() (last item)
                                  // AllItems could hold no more than Max<TIndex>() elements, respectively
};


template <typename T>
struct TYVectorTraits {
    typedef TVector<T> TVectorType;
    typedef NIter::TVectorIterator<const T> TIterator;

    static inline TIterator Iterate(const TVectorType& v) {
        return TIterator(v);
    }

    static inline void PushBack(TVectorType& v, const T& t) {
        v.push_back(t);
    }

    // hope RVO works here
    static inline TVectorType MakeSingularVector(const T& t) {
        return TVectorType(1, t);
    }

    static inline void SaveRange(IOutputStream* output, const TVectorType& v) {
        ::SaveRange(output, v.begin(), v.end());
    }

    static inline size_t SaveRangeSize(const TVectorType& v) {
        return v.size();
    }
};


template <typename T, typename TSubVectorTraits = TYVectorTraits<T> >
class TVectorsVectorBuilder : TNonCopyable
{
public:
    typedef TVectorsVectorIndex TIndex;
    typedef typename TSubVectorTraits::TVectorType TSubVector;
    typedef typename TSubVectorTraits::TIterator TSubVectorIter;

    static const TIndex EmptyIndex = 0;

    TVectorsVectorBuilder()
        : TotalSize_(0)
    {
        Vectors.emplace_back();    // the only empty vector, corresponds to EmptyIndex
    }

    // sub-vectors count
    inline size_t Size() const {
        return Vectors.size();
    }

    inline size_t TotalSize() const {
        return TotalSize_;
    }

    const TSubVector& SubVector(TIndex index) const {
        return Vectors[index];
    }

    inline TSubVectorIter Iter(TIndex index) const {
        return (index == EmptyIndex) ? TSubVectorIter() : TSubVectorTraits::Iterate(Vectors[index]);
    }

    inline TSubVectorIter operator[](TIndex index) const {
        return Iter(index);
    }

    bool HasItemAt(TIndex index, const T& item) const {
        for (TSubVectorIter it = Iter(index); it.Ok(); ++it)
            if (*it == item)
                return true;
        return false;
    }

    inline TIndex NextIndex() const {
        return Vectors.size();
    }

    TIndex Add(const T& item) {
        TIndex ret = NextIndex();
        Vectors.push_back(TSubVectorTraits::MakeSingularVector(item));
        ++TotalSize_;
        return ret;
    }

    TIndex AddTo(TIndex index, const T& item) {
        if (index == EmptyIndex)
            return Add(item);
        TSubVectorTraits::PushBack(Vectors[index], item);
        ++TotalSize_;
        return index;
    }

    // Serialized to format, compatible with TVectorsVector.
    void Save(IOutputStream* output) const;

    TString DebugString() const;

    size_t ByteSize() const {
        return sizeof(*this) + ::OuterByteSize(Vectors);
    }

private:
    TVector<TSubVector> Vectors;
    size_t TotalSize_;
};


// template impl

template <class T>
template <class TValues>
TVectorsVector<T>::TVectorsVector(const TVector<TValues>& source)
{
    // TValues must be a stl-container of T, e.g. TVector<T>;
    size_t total_size = 0;
    for (size_t i = 0; i < source.size(); ++i)
        total_size += source[i].size();

    AllItems.reserve(total_size);
    FirstIndex.resize(source.size() + 1);

    size_t cur_size = 0;
    for (size_t i = 0; i < source.size(); ++i) {
        // overflow check
        CheckOverflow(cur_size);

        FirstIndex[i] = cur_size;
        const TValues& values = source[i];
        cur_size += values.size();
        AllItems.insert(AllItems.end(), values.begin(), values.end());
    }
    FirstIndex.back() = AllItems.size();
}


template <typename T, typename TTraits>
void TVectorsVectorBuilder<T, TTraits>::Save(IOutputStream* output) const {
    size_t totalRangeSize = 0;
    // skip first empty vector (as it might be not empty really, see PackedIntVector specialization)
    for (size_t i = 1; i < Vectors.size(); ++i)
        totalRangeSize += TTraits::SaveRangeSize(Vectors[i]);

    TVectorsVector<T>::CheckOverflow(totalRangeSize);

    // TVectorsVector::AllItems
    ::SaveSize(output, totalRangeSize);
    // skip first empty vector (as it might be not empty really, see PackedIntVector specialization)
    for (size_t i = 1; i < Vectors.size(); ++i)
        TTraits::SaveRange(output, Vectors[i]);

    // TVectorsVector::FirstIndex
    ::SaveSize(output, Vectors.size() + 1);
    ui32 next = 0;
    ::Save<TIndex>(output, next);
    for (size_t i = 1; i < Vectors.size(); ++i) {
        ::Save<TIndex>(output, next);
        next += TTraits::SaveRangeSize(Vectors[i]);
    }
    Y_ASSERT(next == totalRangeSize);
    ::Save<TIndex>(output, next);       // vector's end
}

template <typename T, typename TTraits>
TString TVectorsVectorBuilder<T, TTraits>::DebugString() const {
    TStringStream str;
    str << "TVectorsVector: " << Vectors.size() << " sets, ";
    str << TotalSize_ << " total in sets, ";
    str << sizeof(T) << " bytes per item";
    return str.Str();
}

}   // namespace NGzt

