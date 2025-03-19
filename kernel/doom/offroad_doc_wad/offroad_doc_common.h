#pragma once

#include <type_traits>

#include <library/cpp/offroad/tuple/adaptive_tuple_writer.h>
#include <library/cpp/offroad/tuple/adaptive_tuple_reader.h>

#include "offroad_doc_codec.h"

namespace NDoom {

namespace NPrivate {

template<class Hit, class Vectorizer, class Subtractor>
using TBitTupleReader = NOffroad::TTupleReader<Hit, Vectorizer, Subtractor, NOffroad::TBitDecoder16, 4>;

template<class Hit, class Vectorizer, class Subtractor>
using TBitTupleWriter = NOffroad::TTupleWriter<Hit, Vectorizer, Subtractor, NOffroad::TBitEncoder16, 4>;

template<class Hit, class Vectorizer, class Subtractor, class PrefixVectorizer>
class TBitSubTupleReader : public NOffroad::TSubReader<PrefixVectorizer, TBitTupleReader<Hit, Vectorizer, Subtractor>> {
    using TBase = NOffroad::TSubReader<PrefixVectorizer, TBitTupleReader<Hit, Vectorizer, Subtractor>>;
public:
    using TTable = typename TBase::TTable;

    void Reset() {
        TBase::Reset();
    }

    void Reset(const TTable* table, const TArrayRef<const char>& dataRegion, const TArrayRef<const char>& subRegion) {
        TBase::Reset(subRegion, table, dataRegion);
    }
};

template<class Hit, class Vectorizer, class Subtractor, class PrefixVectorizer>
class TBitSubTupleWriter : public NOffroad::TSubWriter<PrefixVectorizer, TBitTupleWriter<Hit, Vectorizer, Subtractor>> {
    using TBase = NOffroad::TSubWriter<PrefixVectorizer, TBitTupleWriter<Hit, Vectorizer, Subtractor>>;
public:
    using TTable = typename TBase::TTable;

    void Reset(const TTable* table, IOutputStream* dataOutput, IOutputStream* subOutput) {
        TBase::Reset(subOutput, table, dataOutput);
    }
};

} // namespace NPrivate



template<EOffroadDocCodec codec, class Hit, class Vectorizer, class Subtractor, class PrefixVectorizer>
class TOffroadDocCommon;


template<class Hit, class Vectorizer, class Subtractor, class PrefixVectorizer>
class TOffroadDocCommon<BitDocCodec, Hit, Vectorizer, Subtractor, PrefixVectorizer> {
public:
    using TReader = std::conditional_t<PrefixVectorizer::TupleSize == 0, NPrivate::TBitTupleReader<Hit, Vectorizer, Subtractor>, NPrivate::TBitSubTupleReader<Hit, Vectorizer, Subtractor, PrefixVectorizer>>;
    using TWriter = std::conditional_t<PrefixVectorizer::TupleSize == 0, NPrivate::TBitTupleWriter<Hit, Vectorizer, Subtractor>, NPrivate::TBitSubTupleWriter<Hit, Vectorizer, Subtractor, PrefixVectorizer>>;
};


template<class Hit, class Vectorizer, class Subtractor, class PrefixVectorizer>
class TOffroadDocCommon<AdaptiveDocCodec, Hit, Vectorizer, Subtractor, PrefixVectorizer> {
public:
    using TReader = NOffroad::TAdaptiveTupleReader<Hit, Vectorizer, Subtractor, PrefixVectorizer>;
    using TWriter = NOffroad::TAdaptiveTupleWriter<Hit, Vectorizer, Subtractor, PrefixVectorizer>;
};


} // namespace NOffroad
