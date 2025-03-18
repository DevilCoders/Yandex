#pragma once

namespace NOffroad {
    class TNullVectorizer {
    public:
        enum {
            TupleSize = 0
        };

        template <class Slice, class T>
        static void Gather(Slice&&, T*) {
        }

        template <class T, class Slice>
        static void Scatter(T, Slice&&) {
        }
    };

}
