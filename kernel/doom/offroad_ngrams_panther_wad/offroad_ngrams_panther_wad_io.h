#pragma once

#include <kernel/doom/offroad_hashed_keyinv_wad/offroad_hashed_keyinv_wad_io.h>
#include <kernel/doom/offroad/panther_hit_adaptors.h>
#include <kernel/doom/wad/wad_index_type.h>

#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>
#include <library/cpp/offroad/custom/ui64_vectorizer.h>

namespace NDoom {
    using TOffroadNgramsPantherIo = TOffroadHashedKeyInvWadIo<
        PantherNgramsIndexType,
        ui32,
        NOffroad::TUi32Vectorizer,
        NOffroad::TD1Subtractor,
        TPantherHit,
        TPantherHitVectorizer,
        TPantherHitSubtractor,
        DefaultPantherNgramsHashModel,
        DefaultPantherNgramsHitModel>;
} // namespace NDoom
