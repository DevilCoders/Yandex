#pragma once

#include "stl_container.h"

namespace NIter {
    template <class T>
    class TVectorIterator: public TStlContainerIterator<typename TStlContainerSelector<T>::TVectorType> {
        typedef typename TStlContainerSelector<T>::TVectorType TVectorType;
        typedef TStlContainerIterator<TVectorType> TBase;

    public:
        inline TVectorIterator()
            : TBase()
        {
        }

        inline explicit TVectorIterator(TVectorType& vector)
            : TBase(vector)
        {
        }

        inline TVectorIterator(typename TBase::TStlIter it_begin, typename TBase::TStlIter it_end)
            : TBase(it_begin, it_end)
        {
        }

        inline explicit TVectorIterator(T* value, size_t count)
            : TBase(value, value + count)
        {
        }

        inline T* operator->() const {
            return TBase::Cur; // should be more efficient than &*IterCur (?)
        }

        inline size_t Size() const {
            return TBase::End - TBase::Cur;
        }
    };

}
