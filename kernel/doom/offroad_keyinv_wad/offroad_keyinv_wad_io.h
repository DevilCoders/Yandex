#pragma once

#include <kernel/doom/standard_models/standard_models.h>

#include "offroad_keyinv_wad_reader.h"
#include "offroad_keyinv_wad_sampler.h"
#include "offroad_keyinv_wad_writer.h"

namespace NDoom {


template <class KeyIo, class HitIo, class KeyDataAccessor>
struct TOffroadKeyInvWadIo {
    using TKeyIo = KeyIo;
    using TKeySampler = typename TKeyIo::TSampler;
    using TKeyWriter = typename TKeyIo::TWriter;
    using TKeyReader = typename TKeyIo::TReader;
    using TKeySearcher = typename TKeyIo::TSearcher;
    using TKeyIterator = typename TKeySearcher::TIterator;
    using TKeyModel = typename TKeyWriter::TModel;
    static constexpr EStandardIoModel DefaultKeyModel = KeyIo::DefaultModel;

    using THitIo = HitIo;
    using THitSampler = typename THitIo::TSampler;
    using THitWriter = typename THitIo::TWriter;
    using THitReader = typename THitIo::TReader;
    using THitSearcher = typename THitIo::TSearcher;
    using THitIterator = typename THitSearcher::TIterator;
    using THitModel = typename THitWriter::TModel;
    static constexpr EStandardIoModel DefaultHitModel = HitIo::DefaultModel;

    using TSampler = TOffroadKeyInvWadSampler<TKeySampler, THitSampler, THitWriter, KeyDataAccessor>;
    using TWriter = TOffroadKeyInvWadWriter<TKeyWriter, THitWriter, KeyDataAccessor>;
    using TReader = TOffroadKeyInvWadReader<TKeyReader, THitReader, KeyDataAccessor>;

    // Compatibility

    enum {
        HasUniSampler = true,
        HasKeySampler = false,
        HasHitSampler = false
    };

    using TUniSampler = TSampler;
};


} // namespace NDoom
