#pragma once

#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/typetraits.h>

namespace NSolveAmbig {
    template <class T>
    struct TOccurrenceTraits {
        static TStringBuf GetId(typename TTypeTraits<T>::TFuncParam o);
        static size_t GetCoverage(typename TTypeTraits<T>::TFuncParam o);
        static size_t GetStart(typename TTypeTraits<T>::TFuncParam o);
        static size_t GetStop(typename TTypeTraits<T>::TFuncParam o);
        static double GetWeight(typename TTypeTraits<T>::TFuncParam o);
    };

    template <class T>
    struct TOccurrenceTraits<TIntrusivePtr<T>> {
        inline static TStringBuf GetId(const TIntrusivePtr<T>& o) {
            Y_ASSERT(o.Get());
            return TOccurrenceTraits<T>::GetId(*o);
        }
        inline static size_t GetCoverage(const TIntrusivePtr<T>& o) {
            Y_ASSERT(o.Get());
            return TOccurrenceTraits<T>::GetCoverage(*o);
        }
        inline static size_t GetStart(const TIntrusivePtr<T>& o) {
            Y_ASSERT(o.Get());
            return TOccurrenceTraits<T>::GetStart(*o);
        }
        inline static size_t GetStop(const TIntrusivePtr<T>& o) {
            Y_ASSERT(o.Get());
            return TOccurrenceTraits<T>::GetStop(*o);
        }
        inline static double GetWeight(const TIntrusivePtr<T>& o) {
            Y_ASSERT(o.Get());
            return TOccurrenceTraits<T>::GetWeight(*o);
        }
    };

}
