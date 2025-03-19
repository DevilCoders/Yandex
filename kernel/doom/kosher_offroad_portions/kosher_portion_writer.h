#pragma once

#include <library/cpp/offroad/tuple/tuple_writer.h>
#include <library/cpp/offroad/key/key_writer.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>
#include <library/cpp/offroad/custom/subtractors.h>

namespace NDoom {

template <class Hit, class HitVectorizer, class HitSubtractor>
class TKosherPortionWriter {
public:
    using THit = Hit;
    using THitWriter = NOffroad::TTupleWriter<THit, HitVectorizer, HitSubtractor>;
    using THitModel = typename THitWriter::TModel;
    using THitTable = typename THitWriter::TTable;

    using TKey = TString;
    using TKeyRef = TStringBuf;
    using TKeyWriter = NOffroad::TKeyWriter<ui32, NOffroad::TUi32Vectorizer, NOffroad::TI1Subtractor>;
    using TKeyModel = typename TKeyWriter::TModel;
    using TKeyTable = typename TKeyWriter::TTable;

    static_assert(THitWriter::Stages == 1, "Expected 1 stage for hit writer");
    static_assert(TKeyWriter::Stages == 1, "Expected 1 stage for key writer");

    enum {
        Stages = 1
    };

    TKosherPortionWriter() = default;

    TKosherPortionWriter(const THitTable* hitTable, IOutputStream* hitOutput, const TKeyTable* keyTable, IOutputStream* keyOutput)
        : HitWriter_(hitTable, hitOutput)
        , KeyWriter_(keyTable, keyOutput)
    {

    }

    void Reset(const THitTable* hitTable, IOutputStream* hitOutput, const TKeyTable* keyTable, IOutputStream* keyOutput) {
        HitsCount_ = 0;
        HitWriter_.Reset(hitTable, hitOutput);
        KeyWriter_.Reset(keyTable, keyOutput);
    }

    void WriteHit(const THit& hit) {
        HitWriter_.WriteHit(hit);
        ++HitsCount_;
    }

    void WriteKey(const TKeyRef& key) {
        if (HitsCount_ > 0) {
            HitWriter_.WriteSeekPoint();
            KeyWriter_.WriteKey(key, HitsCount_);
            HitsCount_ = 0;
        }
    }

    void WriteLayer() {

    }

    void Finish() {
        HitWriter_.Finish();
        KeyWriter_.Finish();
    }

private:
    ui32 HitsCount_ = 0;
    THitWriter HitWriter_;
    TKeyWriter KeyWriter_;
};

} // namespace NDoom
