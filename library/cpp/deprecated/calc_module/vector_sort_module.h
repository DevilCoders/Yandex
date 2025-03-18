#pragma once

#include "module_args.h"
#include "vector_buffer.h"

#include <util/str_stl.h>
#include <util/generic/algorithm.h>

namespace NVectorSortModule {
    // This wrapper allows TVectorSortModule to contain microbdb structures which have comparators only for pointers on records.
    template <class T, template <typename> class TCompare, bool useWrapper>
    struct TCompareWrapper {
        inline bool operator()(const T& l, const T& r) {
            return TCompare<T>()(l, r);
        }
    };

    template <class T, template <typename> class TCompare>
    struct TCompareWrapper<T, TCompare, true> {
        inline bool operator()(const T& l, const T& r) {
            return TCompare<T>()(&l, &r);
        }
    };
}

template <typename T, template <typename> class TCompare = TLess, bool useCompareWrapper = false>
class TVectorSortModule: public TVectorBufferModule<T> {
private:
    typedef TVectorBufferModule<T> TBase;
    using TBase::Buffer;

    TSlaveActionPoint SortPoint;

    TVectorSortModule(bool returnPointer)
        : TBase("TVectorSortModule", returnPointer)
        , SortPoint(SortPoint.Bind(this).template To<&TVectorSortModule::Sort>("sort"))
    {
    }

public:
    static TCalcModuleHolder BuildModule(TModuleArgs args = TModuleArgs()) {
        return new TVectorSortModule(
            args.GetArg<bool>("return_pointer", /*default=*/true));
    }

private:
    void Sort() {
        ::Sort(Buffer.begin(), Buffer.end(), NVectorSortModule::TCompareWrapper<T, TCompare, useCompareWrapper>());
    }
};
