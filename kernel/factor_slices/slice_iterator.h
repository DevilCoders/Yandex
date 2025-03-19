#pragma once

#include <kernel/factor_slices/factor_slices_gen.h>

// Iterate over slice hierarchy from factor_slices_gen.in

namespace NFactorSlices {
    class TSlicesInfo;

    template <typename Iter>
    class TSliceIteratorOps : public Iter {
    private:
        using TThis = TSliceIteratorOps<Iter>;

    public:
        TSliceIteratorOps(EFactorSlice parent = EFactorSlice::ALL)
            : Iter(parent)
        {}

        TThis& operator ++ () {
            Iter::Next();
            return *this;
        }

        TThis operator ++ (int) {
            TThis iter = *this;
            Iter::Next();
            return iter;
        }

        static TThis Begin(EFactorSlice parent) {
            return TThis(parent);
        }

        static const TThis& End() {
            static TThis endIter(EFactorSlice::COUNT);
            return endIter;
        }

        // Support range-for
        const TThis& begin() const {
            return *this;
        }

        const TThis& end() const {
            return End();
        }
    };

    // For all slices contained in X (including X)
    class TSliceIteratorBase {
    protected:
        EFactorSlice EndSlice = EFactorSlice::COUNT;
        EFactorSlice Slice = EFactorSlice::ALL;
        static const TSlicesInfo& Info;

    public:
        TSliceIteratorBase() = default;
        TSliceIteratorBase(EFactorSlice parent);

        void Next();

        bool Valid() const {
            return Slice != EFactorSlice::COUNT;
        }

        EFactorSlice Get() const {
            return Slice;
        }

        EFactorSlice operator * () const {
            return Slice;
        }

        bool operator == (const TSliceIteratorBase& other) const {
            return Slice == other.Slice;
        }

        bool operator != (const TSliceIteratorBase& other) const {
            return Slice != other.Slice;
        }
    };

    using TSliceIterator = TSliceIteratorOps<TSliceIteratorBase>;

    // For all slices contained in X (excluding X)
    class TChildIteratorBase : public TSliceIteratorBase {
    public:
        TChildIteratorBase(EFactorSlice parent);
    };

    using TChildIterator = TSliceIteratorOps<TChildIteratorBase>;

    // For all siblings directly under X
    class TSiblingIteratorBase : public TSliceIteratorBase {
    public:
        TSiblingIteratorBase(EFactorSlice parent);

        void Next();
    };

    using TSiblingIterator = TSliceIteratorOps<TSiblingIteratorBase>;

    // For all leaves contained in X (including X, if it's a leaf)
    class TLeafIteratorBase : public TSliceIteratorBase {
    public:
        TLeafIteratorBase(EFactorSlice parent = EFactorSlice::ALL);

        void Next();
    };

    using TLeafIterator = TSliceIteratorOps<TLeafIteratorBase>;
} // NFactorSlices
