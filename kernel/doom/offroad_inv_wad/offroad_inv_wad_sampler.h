#pragma once

#include <library/cpp/offroad/sub/sub_sampler.h>
#include <library/cpp/offroad/tuple/tuple_sampler.h>

namespace NDoom {

template <class Data, class Vectorizer, class Subtractor, class PrefixVectorizer>
using TOffroadInvWadSampler = std::conditional_t<
    PrefixVectorizer::TupleSize == 0,
    NOffroad::TTupleSampler<Data, Vectorizer, Subtractor, NOffroad::TSampler64, NOffroad::PlainOldBuffer>,
    NOffroad::TSubSampler<PrefixVectorizer, NOffroad::TTupleSampler<Data, Vectorizer, Subtractor, NOffroad::TSampler64, NOffroad::PlainOldBuffer>>
>;

} // namespace NDoom
