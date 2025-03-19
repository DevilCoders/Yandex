#pragma once

#include "kosher_portion_reader.h"
#include "kosher_portion_sampler.h"
#include "kosher_portion_writer.h"

#include <kernel/doom/standard_models/standard_models.h>

namespace NDoom {


template <class Hit, class HitVectorizer, class HitSubtractor, EStandardIoModel DefaultKeyIoModel = NoStandardIoModel, EStandardIoModel DefaultHitIoModel = NoStandardIoModel>
class TKosherPortionIo {
public:
    using TReader = TKosherPortionReader<Hit, HitVectorizer, HitSubtractor>;
    using TSampler = TKosherPortionSampler<Hit, HitVectorizer, HitSubtractor>;
    using TWriter = TKosherPortionWriter<Hit, HitVectorizer, HitSubtractor>;

    using THit = typename TReader::THit;
    using THitModel = typename TReader::THitModel;
    static constexpr EStandardIoModel DefaultHitModel = DefaultHitIoModel;

    using TKeyModel = typename TReader::TKeyModel;
    static constexpr EStandardIoModel DefaultKeyModel = DefaultKeyIoModel;
};


} // namespace NDoom
