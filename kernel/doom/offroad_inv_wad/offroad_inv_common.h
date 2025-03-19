#pragma once

#include <library/cpp/offroad/tuple/limited_tuple_reader.h>
#include <library/cpp/offroad/tuple/limited_tuple_sub_reader.h>

namespace NDoom {


template <class Data, class Vectorizer, class Subtractor, class PrefixVectorizer>
class TOffroadInvCommon {
public:
    using TReader = std::conditional_t<
        PrefixVectorizer::TupleSize == 0,
        NOffroad::TLimitedTupleReader<Data, Vectorizer, Subtractor, NOffroad::TDecoder64, 1, NOffroad::PlainOldBuffer>,
        NOffroad::TLimitedTupleSubReader<PrefixVectorizer, NOffroad::TLimitedTupleReader<Data, Vectorizer, Subtractor, NOffroad::TDecoder64, 1, NOffroad::PlainOldBuffer>>
    >;
};


} // namespace NDoom
