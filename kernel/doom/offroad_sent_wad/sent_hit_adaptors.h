#pragma once

#include <kernel/doom/hits/sent_hit.h>
#include <kernel/doom/hits/sent_len_hit.h>

namespace NDoom {
    struct TSentHitVectorizer {
        enum {
            TupleSize = 1
        };

        template <typename Slice>
        static inline void Scatter(const TSentHit& hit, Slice&& slices) {
            slices[0] = hit.Offset() + 1;
        }

        template <typename Slice>
        static inline void Gather(Slice&& slices, TSentHit* hit) {
            hit->SetOffset(slices[0] - 1);
        }
    };

    struct TSentLenHitVectorizer {
        enum {
            TupleSize = 1
        };

        template <typename Slice>
        static inline void Scatter(const TSentLenHit& hit, Slice&& slices) {
            slices[0] = hit.Length() + 1;
        }

        template <typename Slice>
        static inline void Gather(Slice&& slices, TSentLenHit* hit) {
            hit->SetLength(slices[0] - 1);
        }
    };


    class TSentHitSubtractor {
    public:
        enum {
            TupleSize = 1,
            PrefixSize = 0,
        };

        template<class Storage>
        inline static void Integrate(Storage&&) {}

        template<class Value, class Delta, class Next>
        inline static void Integrate(Value&& value, Delta&& delta, Next&& next) {
            next[0] = delta[0] + value[0] - 1;
        }

        template<class Value, class Delta, class Next>
        inline static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
            delta[0] = next[0] - value[0] + 1;
        }
    };
} // NDoom
