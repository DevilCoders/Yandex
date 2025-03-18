#pragma once

#include "scalar.h"
#include "empty.h"
#include "adaptor.h"
#include "base.h"

#include <util/generic/ptr.h>

namespace NIter {
    // Iterator with most generic interface (essentially, iterator holder)
    // Could be created from many other types of iterators or stl-containers, or single values.

    // Could be valid or invalid: this is same as holds sub-iterator or not.
    // TBaseIterator methods cannot be used when invalid, first some Reset() method should be called.
    // Could be copied (or assigned), but source iterator is invalidated after copying!

    template <class T>
    class TIterator { /* intentionally not inherited from TBaseIterator<T> */
        typedef TBaseIterator<T> TBase;

    public:
        typedef T TValue;

        // constructors ------------------------------------

        inline TIterator()
            : Iter(nullptr)
        {
            //invalid
        }
        /*
    inline explicit TIterator(const T* single_value)
        : Iter(new TScalarIterator<T>(single_value))
    {
    }
*/

        static inline TIterator<T> FromScalar(const T* value) {
            TIterator<T> res;
            res.Reset(value);
            return res;
        }

        template <class TIter>
        static inline TIterator<T> FromIterator(const TIter& iterator) {
            TIterator<T> res;
            res.Reset<TIter>(iterator);
            return res;
        }

        // Reset methods ----------------------------------

        inline void Reset() {
            Iter.Reset(new TEmptyIterator<T>());
        }

        inline void Reset(const T* single_value) {
            Iter.Reset(new TScalarIterator<T>(single_value));
        }

        template <class TIter>
        inline void Reset(const TIter& iterator) {
            Iter.Reset(new TIteratorAdaptor<TIter, T>(iterator));
        }

        inline void Swap(TIterator<T>& iterator) {
            Iter.Swap(iterator.Iter);
        }

        inline bool IsValid() const {
            return Iter.Get() != nullptr;
        }

        //  implements TBaseIterator -------------------------------

        inline bool Ok() const {
            Y_ASSERT(IsValid());
            return Iter->Ok();
        }

        inline void operator++() {
            ++(*Iter);
        }

        inline const T* operator->() const {
            return Iter->operator->();
        }

        inline const T& operator*() const {
            return Iter->operator*();
        }

    private:
        TAutoPtr<TBase> Iter; // content moved on copying, so old iterator becomes invalid after copying!
    };

}
