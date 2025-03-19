#pragma once

#include <type_traits>

#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>

#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/tuple/tuple_sampler.h>
#include <library/cpp/offroad/key/key_sampler.h>

#include "offroad_wad_buffer.h"
#include "offroad_wad_key_buffer.h"
#include <util/string/cast.h>


namespace NDoom {

template<class Hit, class Vectorizer, class Subtractor>
class TOffroadWadSampler {
    using TKeyWriter = NOffroad::TKeySampler<ui32, NOffroad::TUi32Vectorizer, NOffroad::TI1Subtractor>;

    using THitWriter = NOffroad::TTupleSampler<Hit, Vectorizer, Subtractor>;
    using THitBuffer = TOffroadWadBuffer<Hit>;

public:
    using THit = Hit;
    using TKey = TString;
    using TKeyRef = TStringBuf;

    using THitTable = typename THitWriter::TTable;
    using TKeyTable = typename TKeyWriter::TTable;
    using THitModel = typename THitWriter::TModel;
    using TKeyModel = typename TKeyWriter::TModel;
    using TKeyBuffer = TOffroadKeyBuffer<TKey>;

    static_assert(THitWriter::Stages == TKeyWriter::Stages, "Expected same stages for key and hit samplers");

    enum {
        Stages = THitWriter::Stages,
    };

    TOffroadWadSampler() {}

    void Reset() {
        HitBuffer_.Reset();
        KeyBuffer_.Reset();
    }

    void WriteHit(const THit& hit) {
        HitBuffer_.Write(hit);
        KeyBuffer_.AddHit();
    }

    void WriteKey(const TKeyRef& key) {
        KeyBuffer_.AddKey(ToString(key));
        HitBuffer_.WriteSeekPoint();
    }

    std::pair<THitModel, TKeyModel> Finish() {
        KeyBuffer_.PrepareKeys(true);
        HitBuffer_.RemapTermIds(KeyBuffer_.KeysMapping());
        for (ui32 i = 0; i < KeyBuffer_.Keys().size(); ++i) {
            DoSample(KeyBuffer_.Keys()[i], KeyBuffer_.SortedKeysMapping()[i]);
        }

        for (size_t docId = 0; docId < HitBuffer_.Size(); docId++) {
            for (const THit& hit : HitBuffer_.Hits(docId))
                HitWriter_.WriteHit(hit);

            HitWriter_.FinishBlock();
        }

        return { HitWriter_.Finish(), KeyWriter_.Finish() };
    }

private:
    void DoSample(const TKeyRef& key, ui32 id) {
        KeyWriter_.WriteKey(key, id);
    }

private:
    TKeyWriter KeyWriter_;
    THitBuffer HitBuffer_;
    THitWriter HitWriter_;
    TKeyBuffer KeyBuffer_;
};


} // namespace NDoom
