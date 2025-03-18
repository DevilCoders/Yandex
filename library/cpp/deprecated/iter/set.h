#pragma once

#include "stl_container.h"

namespace NIter {
    template <class T>
    class TSetIterator: public TStlContainerIterator<typename TStlContainerSelector<T>::TSetType> {
        typedef typename TStlContainerSelector<T>::TSetType TSetType;
        typedef TStlContainerIterator<TSetType> TBase;

    public:
        inline TSetIterator()
            : TBase()
        {
        }

        inline explicit TSetIterator(TSetType& set)
            : TBase(set)
        {
        }

        inline TSetIterator(typename TBase::TStlIter it_begin, typename TBase::TStlIter it_end)
            : TBase(it_begin, it_end)
        {
        }
    };

}
