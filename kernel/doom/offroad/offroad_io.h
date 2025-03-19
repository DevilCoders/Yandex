#pragma once

#include <library/cpp/offroad/keyinv/null_keyinv_sampler.h>

#include <kernel/doom/standard_models/standard_models.h>

#include "offroad_writer.h"
#include "offroad_reader.h"
#include "offroad_searcher.h"

namespace NDoom {

template<EIndexFormat Format, class Hit, class KeyData, class Vectorizer, class Subtractor, EStandardIoModel DefaultKeyIoModel = NoStandardIoModel, EStandardIoModel DefaultHitIoModel = NoStandardIoModel>
struct TOffroadIo {
    static constexpr auto IndexFormat = Format;

    using THitSampler = TOffroadHitSampler<Hit, KeyData, Vectorizer, Subtractor>;
    using TKeySampler = TOffroadKeySampler<Hit, KeyData, Vectorizer, Subtractor>;
    using TUniSampler = NOffroad::TNullKeyInvSampler;
    using TReader = TOffroadReader<Format, Hit, KeyData, Vectorizer, Subtractor>;
    using TWriter = TOffroadWriter<Format, Hit, KeyData, Vectorizer, Subtractor>;
    using TSearcher = TOffroadSearcher<Format, Hit, KeyData, Vectorizer, Subtractor>;

    using THit = typename TReader::THit;
    using THitModel = typename TReader::THitModel;
    static constexpr auto DefaultHitModel = DefaultHitIoModel;

    using TKeyModel = typename TReader::TKeyModel;
    static constexpr auto DefaultKeyModel = DefaultKeyIoModel;

    enum {
        HasHitSampler = true,
        HasKeySampler = true,
        HasUniSampler = false,
    };
};

} // namespace NDoom
