#pragma once

#include "factor_borders.h"
#include "slices_info.h"
#include "slice_iterator.h"

#include <kernel/factors_info/one_factor_info.h>

#include <util/generic/maybe.h>
#include <util/generic/array_ref.h>

namespace NFactorSlices {
    // Trivial destructor is required.
    // Main use case: store in memory pool as part of TFactorStorage,
    // where dtor is never invoked.
    class TFactorDomain {
        TFactorBorders Borders;
        bool Normal = false;
        size_t NumFactors = 0;
        TMaybe<EFactorUniverse> Universe;

        static EFactorUniverse GlobalUniverse;

    public:
        class TIterator;

    public:
        TFactorDomain(const TFactorDomain&) = default;
        TFactorDomain& operator=(const TFactorDomain&) = default;

        // NOTE. TFactorDomain ctor of the form TFactorDomain(EFactorSlice::FOO)
        // will *fail*, if static information about slice FOO is not available
        // at runtime. To make it work, your binary must be linked (via PEERDIR)
        // with the codegen module (factors_gen.in) that describes
        // contents of EFactorSlice::FOO. For example see kernel/web_factors_info.

        explicit TFactorDomain(const TSlicesMetaInfo& metaInfo, TMaybe<EFactorUniverse> universe = Nothing())
            : Borders(metaInfo.GetBorders())
            , Normal(true)
            , Universe(universe)
        {
            InitNumFactors();
        }

        explicit TFactorDomain(const TSlicesMetaInfo& metaInfo, EFactorUniverse universe)
            : TFactorDomain(metaInfo, TMaybe<EFactorUniverse>{ universe })
        {
        }

        template <class... Args>
        explicit TFactorDomain(const TSlicesMetaInfo& metaInfo, Args... args)
            : TFactorDomain(CreateMetaInfo(metaInfo, args...))
        {
        }

        template <class... Args>
        explicit TFactorDomain(Args... args)
            : TFactorDomain(CreateMetaInfo(NFactorSlices::TGlobalSlicesMetaInfo::Instance(), args...))
        {
        }

        explicit TFactorDomain(const TFactorBorders& borders, TMaybe<EFactorUniverse> universe = Nothing())
            : Borders(borders)
            , Normal(Borders.TryToValidate())
            , Universe(universe)
        {
            InitNumFactors();
        }

        explicit TFactorDomain(const TFactorBorders& borders, EFactorUniverse universe)
            : TFactorDomain(borders, TMaybe<EFactorUniverse>{ universe })
        {
        }

        explicit TFactorDomain(ui32 count) {
            InitDefaultWebBorders(count);
            InitNumFactors();
        }

        explicit TFactorDomain(int count)
            : TFactorDomain(static_cast<ui32>(count))
        {
        }

        explicit TFactorDomain(size_t count)
            : TFactorDomain(static_cast<ui32>(count))
        {
        }

        explicit TFactorDomain(ui32 count, EFactorUniverse universe)
            : Universe(universe)
        {
            InitDefaultWebBorders(count);
            InitNumFactors();
        }

        static void SetGlobalUniverse(EFactorUniverse universe);
        // resetting global universe may lead to race conditions, ForceSetGlobalUniverse contains no checks of multiple settings of global universe
        static void ForceSetGlobalUniverse(EFactorUniverse universe);
        static EFactorUniverse GetGlobalUniverse();
        static EFactorSlice GetSliceFor(EFactorUniverse universe, ESliceRole role);

        EFactorUniverse GetUniverse() const;
        EFactorSlice GetSliceFor(ESliceRole role) const;

        // Helper overload to uniformly handle slices and roles
        static EFactorSlice GetSliceFor(EFactorSlice slice) {
            return slice;
        }

        TMaybe<TFullFactorIndex> GetL3ModelValueIndex() const;

        bool IsNormal() const {
            return Normal;
        }
        size_t Size() const {
            return NumFactors;
        }
        const TFactorBorders& GetBorders() const {
            return Borders;
        }
        const TSliceOffsets& operator[] (EFactorSlice slice) const {
            return Borders[slice];
        }

        // Iterate over domain
        TIterator Begin(EFactorSlice slice = EFactorSlice::ALL) const;
        TIterator End() const;

        // Following methods correctly work
        // with iterator from another TFactorDomain
        bool HasIndex(const TFullFactorIndex& iter) const;
        TFactorIndex GetIndex(const TFullFactorIndex& iter) const;
        TFactorIndex GetRelativeIndex(EFactorSlice slice, const TFullFactorIndex& iter) const;

        // Get factor index by name; requires leaf slice
        bool TryGetRelativeIndexByName(EFactorSlice slice, const TString& factorName, TFactorIndex& index) const;

        // Get factors info
        const IFactorsInfo* GetSliceFactorsInfo(EFactorSlice slice) const;
        TOneFactorInfo GetFactorInfo(TFactorIndex index, EFactorSlice slice = EFactorSlice::ALL) const;

        // Return leaf slice that contains factor index
        EFactorSlice GetLeafByFactorIndex(TFactorIndex index, EFactorSlice slice = EFactorSlice::ALL) const;

        EFactorSlice GetReplacementSliceUglyHack(EFactorSlice slice) const;

        bool operator==(const TFactorDomain& rhs) const;

        void Save(IOutputStream* rh) const {
            ::Save(rh, Borders);
            ::Save(rh, Normal);
            ::Save(rh, NumFactors);
        }
        void Load(IInputStream* rh) {
            ::Load(rh, Borders);
            ::Load(rh, Normal);
            ::Load(rh, NumFactors);
        }

        [[nodiscard]] TFactorDomain MakeDomainWithIncreasedSlice(EFactorSlice slice, size_t newFeatsNum) const;

    private:
        void InitDefaultWebBorders(ui32 count);
        void InitNumFactors();

        EFactorSlice GetChildOfSameSize(EFactorSlice slice) const;

        template <class... Args>
        static TSlicesMetaInfo CreateMetaInfo(const TSlicesMetaInfo& metaInfo, Args... args) {
            TSlicesMetaInfo tempMetaInfo = metaInfo;
            EnableSlices(tempMetaInfo, args...);
            return tempMetaInfo;
        }
    };

    class TFactorDomain::TIterator {
        friend class TFactorDomain;

        // Domain-specific
        const TFactorDomain* Domain = nullptr;
        TFactorIndex Index = Max<TFactorIndex>();

        // Doesn't depend on domain
        EFactorSlice Slice = EFactorSlice::ALL;
        TLeafIterator LeafIter;
        const IFactorsInfo* Info = nullptr;

    private:
        TIterator(const TFactorDomain& domain)
            : Domain(&domain)
            , Index(domain.Size())
        {
        }

        TIterator(const TFactorDomain& domain, EFactorSlice slice);

        void UpdateCurrentLeaf();
        void UpdateCurrentInfo();

    public:
        TIterator() = default;
        TIterator(const TIterator&) = default;

        bool Valid() const {
            return Index < (*Domain)[Slice].End;
        }
        void Next() {
            Advance(1);
        }
        void NextLeaf() {
            Y_ASSERT(Valid());
            LeafIter.Next();
            UpdateCurrentLeaf();
            if (!LeafIter.Valid()) {
                return;
            }
            Index = (*Domain)[*LeafIter].Begin;
        }
        void Advance(size_t count) {
            Y_ASSERT(Valid());
            Index += count;
            if (Index < (*Domain)[*LeafIter].End) {
                return;
            }
            if (!Valid()) {
                Index = Domain->Size();
                LeafIter = TLeafIterator::End();
                Info = nullptr;
                return;
            }
            LeafIter.Next();
            while (LeafIter.Valid() && Index >= (*Domain)[*LeafIter].End) {
                LeafIter.Next();
            }
            Y_ASSERT(LeafIter.Valid());
            Y_ASSERT(count > 1 || Index == (*Domain)[*LeafIter].Begin);
            UpdateCurrentInfo();
        }

        // Finds largest contiguous chunk that exists both
        // in parent domain of this iterator and another domain (arg).
        // Chunk starts from current iterator position.
        // Assigns size = 0, if current factor doesn't exist
        // in another domain, and skips to first common factor.
        void NextChunk(const TFactorDomain& domain, size_t& size);
        TIterator To(const TFactorDomain& domain) const;

        const TFactorDomain& GetDomain() const {
            return *Domain;
        }
        EFactorSlice GetSlice() const {
            return Slice;
        }

        EFactorSlice GetLeaf() const {
            Y_ASSERT(Valid());
            return *LeafIter;
        }
        size_t GetLeafSize() const {
            Y_ASSERT(Valid());
            return (*Domain)[*LeafIter].Size();
        }
        TFactorIndex GetIndex() const {
            Y_ASSERT(Valid());
            return Index;
        }
        TFactorIndex GetIndexInLeaf() const {
            Y_ASSERT(Valid());
            return (*Domain)[*LeafIter].GetRelativeIndex(Index);
        }
        TFactorIndex GetIndexInSlice() const {
            Y_ASSERT(Valid());
            return (*Domain)[Slice].GetRelativeIndex(Index);
        }
        TFactorIndex GetIndexInSlice(EFactorSlice slice) const {
            Y_ASSERT(Valid());
            return (*Domain)[slice].GetRelativeIndex(Index);
        }
        TOneFactorInfo GetFactorInfo() const {
            Y_ASSERT(Valid());
            return TOneFactorInfo(GetIndexInLeaf(), Info);
        }

        bool operator == (const TIterator& other) const {
            return Domain == other.Domain &&
                Index == other.Index;
        }
        bool operator != (const TIterator& other) const {
            return !(*this == other);
        }
        bool operator < (const TIterator& other) const {
            return Domain == other.Domain &&
                Index < other.Index;
        }
        ptrdiff_t operator - (const TIterator& other) const {
            Y_ASSERT(Domain == other.Domain);
            return Index - other.Index;
        }
        TIterator& operator ++ () {
            Next();
            return *this;
        }
        TIterator operator ++ (int) {
            TIterator tmp(*this);
            Y_UNUSED(tmp);
            Next();
            return *this;
        }
        Y_FORCE_INLINE operator TFullFactorIndex () const {
            Y_ASSERT(Valid());
            return TFullFactorIndex(*LeafIter, (*Domain)[*LeafIter].GetRelativeIndex(Index));
        }
    };

    Y_FORCE_INLINE bool TFactorDomain::HasIndex(const TFullFactorIndex& index) const {
        return Borders[index.Slice].ContainsRelative(index.Index);
    }
    Y_FORCE_INLINE TFactorIndex TFactorDomain::GetIndex(const TFullFactorIndex& index) const {
        return Borders.GetIndex(index.Index, index.Slice);
    }
    Y_FORCE_INLINE TFactorIndex TFactorDomain::GetRelativeIndex(EFactorSlice slice, const TFullFactorIndex& index) const {
        return Borders.GetRelativeIndex(slice, index.Index, index.Slice);
    }

    using TFactorIterator = TFactorDomain::TIterator;

} // NFactorSlices
