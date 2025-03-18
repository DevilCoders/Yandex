#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/iterator.h>
#include <util/generic/ptr.h>

namespace NDomTree {
    template <typename T>
    class IAbstractTraverser {
    public:
        virtual T Next() = 0;
        virtual ~IAbstractTraverser() = default;
    };

    template <typename T>
    class TDomIterator: public TInputRangeAdaptor<TDomIterator<T>> {
    public:
        TDomIterator(TSimpleSharedPtr<T> t)
            : Traverser(t)
        {
        }

        auto Next() {
            return Traverser->Next();
        }

    private:
        TSimpleSharedPtr<T> Traverser;
    };

}
