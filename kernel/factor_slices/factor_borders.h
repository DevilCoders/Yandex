#pragma once

#include "meta_info.h"

#include <kernel/factor_slices/factor_slices_gen.h>
#include <kernel/feature_pool/feature_slice.h>

#include <util/generic/map.h>
#include <util/ysaveload.h>

namespace NFactorSlices {
    using TFactorIndex = i32;

    struct TFullFactorIndex {
        EFactorSlice Slice = EFactorSlice::ALL;
        TFactorIndex Index = Max<TFactorIndex>();

        TFullFactorIndex() = default;
        explicit TFullFactorIndex(TFactorIndex index)
            : Index(index)
        {}
        TFullFactorIndex(EFactorSlice slice, TFactorIndex index)
            : Slice(slice)
            , Index(index)
        {}

        void Save(IOutputStream* rh) const {
            ::Save(rh, ui16(Slice));
            ::Save(rh, Index);
        }
        void Load(IInputStream* rh) {
            ui16 sliceIndex = 0;
            ::Load(rh, sliceIndex);
            Slice = static_cast<EFactorSlice>(sliceIndex);
            ::Load(rh, Index);
        }

        bool operator < (const TFullFactorIndex& other) const {
            return Slice < other.Slice
                || (Slice == other.Slice && Index < other.Index);
        }

        bool operator == (const TFullFactorIndex& other) const {
            return Slice == other.Slice && Index == other.Index;
        }
    };

    class TSliceOffsets {
    public:
        TFactorIndex Begin = 0;
        TFactorIndex End = 0;

    public:
        TSliceOffsets() = default;
        TSliceOffsets(TFactorIndex begin, TFactorIndex end)
            : Begin(begin)
            , End(end)
        {
            Y_ASSERT(0 <= Begin);
            Y_ASSERT(Begin <= End);
        }

        bool operator == (const TSliceOffsets& other) const {
            return other.End == End && other.Begin == Begin;
        }

        bool operator < (const TSliceOffsets& other) const {
            return Begin < other.Begin ||
                (Begin == other.Begin && End < other.End);
        }

        bool operator <= (const TSliceOffsets& other) const {
            return Begin <= other.Begin && End <= other.End;
        }

        size_t Size() const {
            Y_ASSERT(Begin <= End);
            return End - Begin;
        }
        bool Empty() const {
            return Size() == 0;
        }

        bool Contains(TFactorIndex index) const {
            return Begin <= index && End > index;
        }
        bool ContainsRelative(TFactorIndex relIndex) const {
            return 0 <= relIndex && Size() > static_cast<size_t>(relIndex);
        }
        bool Contains(const TSliceOffsets& other) const {
            return Begin <= other.Begin && End >= other.End;
        }
        bool ContainsRelative(const TSliceOffsets& other) const {
            Y_ASSERT(other.End >= other.Begin);
            return 0 <= other.Begin && Size() >= static_cast<size_t>(other.End);
        }
        bool Overlaps(const TSliceOffsets& other) const {
            return (!other.Empty() && Contains(other.Begin)) ||
                (!Empty() && other.Contains(Begin));
        }
        TFactorIndex GetIndex(TFactorIndex relIndex) const {
            Y_ASSERT(ContainsRelative(relIndex));
            return Begin + relIndex;
        }
        TFactorIndex GetRelativeIndex(TFactorIndex index) const {
            Y_ASSERT(Contains(index));
            return Max<TFactorIndex>(0, index - Begin);
        }

        Y_FORCE_INLINE TFactorIndex GetRelativeIndex(TFactorIndex index,
            const TSliceOffsets& other) const
        {
            return GetRelativeIndex(other.GetIndex(index));
        }
        Y_FORCE_INLINE bool Contains(TFactorIndex index,
            const TSliceOffsets& other) const
        {
            return Contains(other.GetIndex(index));
        }

        Y_FORCE_INLINE void Erase(const TSliceOffsets& offsets) {
            if (offsets.End <= End) {
                End -= offsets.Size();
            } else if (offsets.Begin <= End) {
                End = offsets.Begin;
            }

            if (offsets.Begin <= Begin) {
                if (offsets.End <= Begin) {
                    Begin -= offsets.Size();
                } else {
                    Begin = offsets.Begin;
                }
            }
        }

        void Save(IOutputStream* rh) const {
            ::Save(rh, Begin);
            ::Save(rh, End);
        }
        void Load(IInputStream* rh) {
            ::Load(rh, Begin);
            ::Load(rh, End);
        }
    };

    class TFactorBorders : public TSliceMap<TSliceOffsets> {
    public:
        TFactorBorders() = default;
        TFactorBorders(const TSlicesMetaInfo& metainfo) {
            metainfo.MakeBorders(*this);
        }

        Y_FORCE_INLINE TFactorIndex GetIndex(TFactorIndex index, EFactorSlice sliceFrom) const
        {
            return (*this)[sliceFrom].GetIndex(index);
        }
        Y_FORCE_INLINE TFactorIndex GetRelativeIndex(EFactorSlice sliceTo, TFactorIndex index,
            EFactorSlice sliceFrom = EFactorSlice::ALL) const
        {
            return (*this)[sliceTo].GetRelativeIndex(index, (*this)[sliceFrom]);
        }

        // Erase all indexes between Begin and End, as if
        // elements from vector
        void Erase(const TSliceOffsets& offsets);

        // Checks that borders are kosher:
        //   - Order and hierarchy of slices
        //   - Sizes of parents and chilren are consistent
        //   - Omitted borders are not allowed
        bool TryToValidate() const;

        // [begin; end) is minimal w.r.t. borders iff
        // for every non-empty slice, [begin; end) doesn't contain borders[slice]
        bool IsMinimal(const TSliceOffsets& offsets) const;

        size_t SizeAll() const;
    };

    enum class ESerializationMode {
        All,
        LeafOnly
    };

    TVector<EFactorSlice> GetFactorSlices(const TFactorBorders& borders,
        ESerializationMode mode = ESerializationMode::LeafOnly);
    void SerializeFactorBorders(IOutputStream& out, const TFactorBorders& borders,
        ESerializationMode mode = ESerializationMode::All);
    TString SerializeFactorBorders(const TFactorBorders& borders,
        ESerializationMode mode = ESerializationMode::All);


    // Deserialize borders for known factor slices.
    // Throws exception in case of unknown slice names and invalid slice borders syntax.
    void DeserializeFactorBorders(const TStringBuf buf, TFactorBorders& res);
    bool TryToDeserializeFactorBorders(const TStringBuf buf, TFactorBorders& res);

    // Deserialize borders for both known and unknown slices.
    // Throws exception in case of invalid slice borders syntax and unknown slice names, if name validation wasn't skipped.
    void DeserializeFeatureSlices(const TStringBuf buf, const bool skipNameValidation, NMLPool::TFeatureSlices& res);
    bool TryToDeserializeFeatureSlices(const TStringBuf buf, const bool skipNameValidation, NMLPool::TFeatureSlices& res);

    // Read slices from raw vector and init borders with them.
    // Throws exception in case of unknown slice name.
    void ParseSlicesVector(const NMLPool::TFeatureSlices& rawSlices, TFactorBorders& res);

    // Checks if all names are known
    bool ValidateBordersNames(const TVector<TString>& names, TVector<TString>& unknownSliceNames);
    bool IsBordersInSubsetRelation(const TFactorBorders& subsetCandidate, const TFactorBorders& supersetCandidate);

    // Custom compare to simplify lookups
    // Parent (containing slice) always preceeds
    // its children (contained slices)
    // All empty slices are at the beginning
    struct TCompareSliceOffsets {
        bool operator () (const TSliceOffsets& x, const TSliceOffsets& y) const {
            return (x.Empty() && !y.Empty()) ||
                (x.Empty() == y.Empty() && (x.Begin < y.Begin ||
                    (x.Begin == y.Begin && x.End > y.End)));
        }
    };

    namespace NDetail {
        class TDeserializeError : public yexception {};
        class TSliceNameError : public TDeserializeError {
        public:
            TMap<TString, TSliceOffsets> UnknownSlices;
            size_t TotalNamesCount = 0;

            TSliceNameError(const TMap<TString, TSliceOffsets>& unknownSlices, size_t totalNamesCount)
                : UnknownSlices(unknownSlices)
                , TotalNamesCount(totalNamesCount)
            {}
        };
        class TParseError : public TDeserializeError {};

        struct TReConstructOptions {
            bool IgnoreHierarchicalBorders = false; // Do not fail on incorrect borders for hier slices;
                                                    // instead always infer from child slices

            static TReConstructOptions GetRobust() {
                TReConstructOptions r;
                r.IgnoreHierarchicalBorders = true;
                return r;
            }
        };

        // Fill all missing offsets
        // according to static slice hierarchy.
        // Non-empty offsets are not modified.
        // Return false, if it cannot be done, or there is more than one
        // possibly solution.
        [[nodiscard]] bool ReConstructMetaInfo(const TFactorBorders& fromBorders, TSlicesMetaInfo& toMetainfo,
            const TReConstructOptions& options = TReConstructOptions());
        [[nodiscard]] bool ReConstructBorders(const TFactorBorders& fromBorders, TFactorBorders& toBorders,
            const TReConstructOptions& options = TReConstructOptions());
        [[nodiscard]] bool ReConstruct(TFactorBorders& borders,
            const TReConstructOptions& options = TReConstructOptions());
        //throws on error
        void EnsuredReConstructMetaInfo(const TFactorBorders& fromBorders, TSlicesMetaInfo& toMetainfo,
            const TReConstructOptions& options = TReConstructOptions());
        void EnsuredReConstructBorders(const TFactorBorders& fromBorders, TFactorBorders& toBorders,
            const TReConstructOptions& options = TReConstructOptions());
        void EnsuredReConstruct(TFactorBorders& borders,
            const TReConstructOptions& options = TReConstructOptions());

        class TSortedFactorBorders {
        private:

            using TSortedSlices = TMultiMap<TSliceOffsets, EFactorSlice, TCompareSliceOffsets>;

            TFactorBorders Borders;
            TSortedSlices Sorted;

        public:
            struct TIterator {
                friend class TSortedFactorBorders;

                using TIter = TSortedSlices::const_iterator;
                TIter Cur;
                TIter End;

            private:
                TIterator(TIter begin, TIter end)
                    : Cur(begin)
                    , End(end)
                {}

            public:
                void Next() {
                    if (Y_LIKELY(Cur != End)) {
                        ++Cur;
                    }
                }
                bool Valid() const {
                    return Cur != End;
                }
                EFactorSlice GetSlice() const {
                    return Cur->second;
                }
                const TSliceOffsets& GetOffsets() const {
                    return Cur->first;
                }
            };

            TSortedFactorBorders() {
                Prepare();
            }

            TSortedFactorBorders(const TFactorBorders& borders)
                : Borders(borders)
            {
                Prepare();
            }

            TSortedFactorBorders& operator = (const TSortedFactorBorders&) = default;

            TSortedFactorBorders& operator = (const TFactorBorders& borders) {
                Borders = borders;
                Sorted.clear();
                Prepare();
                return *this;
            }

            // Checks basic properties:
            //   - No overlaps, containment is ok
            //   - Size(parent) = Sum of Size(child)
            bool TryToValidate() const;

            TIterator Begin() const {
                return TIterator(Sorted.begin(), Sorted.end());
            }

            TIterator BeginSkipEmpty() const {
                auto begin = Sorted.lower_bound(TSliceOffsets(0, Max<TFactorIndex>()));
                return TIterator(begin, Sorted.end());
            }

            const TFactorBorders& GetBorders() const {
                return Borders;
            }

            // Returns any minimal slice
            // that contains index.
            // EFactorSlice::COUNT if none.
            EFactorSlice GetSliceByFactorIndex(TFactorIndex index) const;

        private:
            void Prepare();
        };
    }
} // NFactorSlices

template<>
struct THash<NFactorSlices::TFullFactorIndex> {
    inline size_t operator() (const NFactorSlices::TFullFactorIndex& fullFactorIndex) {
        TStringStream serialized;
        fullFactorIndex.Save(&serialized);
        return THash<TString>()(serialized.Str());
    }
};
