#pragma once

#include <kernel/doom/wad/wad_index_type.h>

#include <library/cpp/offroad/utility/tagged.h>

#include "offroad_inv_common.h"

namespace NDoom {


template <class Data, class Vectorizer, class Subtractor, class PrefixVectorizer>
class TOffroadInvWadIterator: private NOffroad::NPrivate::TTaggedBase {
    using TReader = typename TOffroadInvCommon<Data, Vectorizer, Subtractor, PrefixVectorizer>::TReader;

public:
    using THit = Data;

    Y_FORCE_INLINE bool ReadHit(THit* data) {
        return Reader_.ReadHit(data);
    }

    template <class Consumer>
    Y_FORCE_INLINE bool ReadHits(const Consumer& consumer) {
        return Reader_.ReadHits(consumer);
    }

    Y_FORCE_INLINE bool LowerBound(const THit& prefix, THit* first) {
        return Reader_.LowerBound(prefix, first);
    }

private:
    template <EWadIndexType anotherIndexType, class AnotherData, class AnotherVectorizer, class AnotherSubtractor, class AnotherPrefixVectorizer>
    friend class TOffroadInvWadSearcher;

    TReader Reader_;
};


} // namespace NDoom
