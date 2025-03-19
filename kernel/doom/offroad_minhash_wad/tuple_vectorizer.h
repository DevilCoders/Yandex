#pragma once

#include <tuple>

namespace NDoom {

template <typename ...Vectorizers>
class TTupleVectorizer {
public:
    enum {
        TupleSize = (Vectorizers::TupleSize + ...),
    };

    template <typename Slice, typename Tuple>
    static void Gather(Slice&& slice, Tuple* tuple) {
        GatherImpl<0, 0, Vectorizers...>(std::forward<Slice>(slice), tuple);
    }

    template <typename Slice, typename Tuple>
    static void Scatter(const Tuple& tuple, Slice&& slice) {
        ScatterImpl<0, 0, Vectorizers...>(tuple, std::forward<Slice>(slice));
    }

private:
    template <size_t I, size_t Offset, typename Tuple, typename Slice, typename Vectorizer, typename ...RestVectorizers>
    static void GatherImpl(Slice&& slice, Tuple* tuple) {
        Vectorizer::Gather(slice.template SubSlice<Offset>(), &std::get<I>(*tuple));
        if constexpr (sizeof...(RestVectorizers)) {
            GatherImpl<I + 1, Offset + Vectorizer::TupleSize, RestVectorizers...>();
        }
    }

    template <size_t I, size_t Offset, typename Tuple, typename Slice, typename Vectorizer, typename ...RestVectorizers>
    static void ScatterImpl(const Tuple& tuple, Slice&& slice) {
        Vectorizer::Scatter(std::get<I>(tuple), slice.template SubSlice<Offset>());
        if constexpr (sizeof...(RestVectorizers)) {
            ScatterImpl<I + 1, Offset + Vectorizer::TupleSize, RestVectorizers...>();
        }
    }
};

} // namespace NDoom
