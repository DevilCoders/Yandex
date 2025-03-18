#pragma once

#include <util/system/defaults.h>
#include <util/stream/str.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <util/generic/algorithm.h>

namespace NGraph {
    struct TArcadiaTraits {
        template <typename E>
        struct TVectorT {
            typedef TVector<E> T;
        };

        template <typename K, typename V>
        struct TMapT {
            typedef THashMap<K, V> T;
        };

        typedef TString TStringT;
        typedef IInputStream TInputStreamT;
        typedef IOutputStream TOutputStreamT;
        typedef TStringOutput TStringOutputT;
        typedef TStringInput TStringInputT;

        template <class T, class C>
        static inline void Sort(T f, T l, C c) {
            ::Sort(f, l, c);
        }
    };

    template <typename VertexProps,
              typename EdgeProps>
    struct TDefaultTraits: public TArcadiaTraits {
        typedef unsigned int TCount;
        typedef VertexProps TVertexProps;
        typedef EdgeProps TEdgeProps;
    };

}
