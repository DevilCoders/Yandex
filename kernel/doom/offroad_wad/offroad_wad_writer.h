#pragma once

#include <util/stream/file.h>
#include <util/string/cast.h>

#include <library/cpp/offroad/key/key_writer.h>
#include <library/cpp/offroad/key/fat_key_writer.h>
#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>
#include <library/cpp/offroad/custom/null_serializer.h>
#include <library/cpp/offroad/custom/ui64_varint_serializer.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/tuple/adaptive_tuple_writer.h>

#include <kernel/doom/adaptors/transforming_key_writer.h>
#include <kernel/doom/info/index_format.h>
#include <kernel/doom/offroad_common/accumulating_output.h>
#include <kernel/doom/wad/mega_wad_writer.h>


#include "offroad_wad_buffer.h"
#include "offroad_wad_key_buffer.h"

namespace NDoom {


/**
 * Converting writer for offroad keyinv wad files.
 */
template<EWadIndexType indexType, class Hit, class Vectorizer, class Subtractor, class PrefixVectorizer, class KeyPrefixGetter>
class TOffroadWadWriter {
private:
    using TKeyWriter = NOffroad::TFatKeyWriter<
        NOffroad::TTransformingKeyWriter<
            KeyPrefixGetter,
            NOffroad::TFatOffsetDataWriter<ui32, NOffroad::TNullSerializer>
        >,
        NOffroad::TKeyWriter<ui32, NOffroad::TUi32Vectorizer, NOffroad::TI1Subtractor>
    >;
    using THitWriter = NOffroad::TAdaptiveTupleWriter<Hit, Vectorizer, Subtractor, PrefixVectorizer>;
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

    static_assert(THitWriter::Stages == 1, "Expected 1 stage for hit writer");
    static_assert(TKeyWriter::Stages == 1, "Expected 1 stage for key writer");

    enum {
        Stages = 1
    };

    TOffroadWadWriter() {}

    TOffroadWadWriter(const THitModel& hitModel, const TKeyModel& keyModel, const TString& path) {
        Reset(hitModel, keyModel, path);
    }

    TOffroadWadWriter(const THitModel& hitModel, const TKeyModel& keyModel, IOutputStream* output) {
        Reset(hitModel, keyModel, output);
    }

    void Reset(const THitModel& hitModel, const TKeyModel& keyModel, const TString& path) {
        Output_.Reset(new TOFStream(path));
        Reset(hitModel, keyModel, Output_.Get());
    }

    void Reset(const THitModel& hitModel, const TKeyModel& keyModel, IOutputStream* output) {
        HitTable_->Reset(hitModel);
        KeyTable_->Reset(keyModel);
        WadOutput_.Reset(output);

        /* Register wad types. */
        WadOutput_.RegisterDocLumpType(TWadLumpId(indexType, EWadLumpRole::Hits));
        WadOutput_.RegisterDocLumpType(TWadLumpId(indexType, EWadLumpRole::HitSub));

        /* Write out models right away. */
        IOutputStream* hitModelOutput = WadOutput_.StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::HitsModel));
        hitModel.Save(hitModelOutput);

        IOutputStream* keyModelOutput = WadOutput_.StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeysModel));
        keyModel.Save(keyModelOutput);

        /* Set up key i/o, key data will be written next. */
        FatOutput_.Reset();
        FatSubOutput_.Reset();
        IOutputStream* keyOutput = WadOutput_.StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::Keys));
        KeyWriter_.Reset(&FatOutput_, &FatSubOutput_, KeyTable_.Get(), keyOutput);

        /* Hits go to buffer first as we need to uninvert the index. */
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

    void Finish() {
        if (IsFinished())
            return;

        KeyBuffer_.PrepareKeys(true);
        HitBuffer_.RemapTermIds(KeyBuffer_.KeysMapping());
        TStringBuf lastKey;

        for (ui32 i = 0; i < KeyBuffer_.Keys().size(); ++i) {
            if (i > 0) {
                Y_ENSURE(lastKey < KeyBuffer_.Keys()[i]);
            }
            KeyWriter_.WriteKey(KeyBuffer_.Keys()[i], KeyBuffer_.SortedKeysMapping()[i]);
            lastKey = KeyBuffer_.Keys()[i];
        }

        KeyWriter_.Finish();

        IOutputStream* keyFatOutput = WadOutput_.StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeyFat));
        FatOutput_.Flush(keyFatOutput);

        IOutputStream* keyIdxOutput = WadOutput_.StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeyIdx));
        FatSubOutput_.Flush(keyIdxOutput);

        THitWriter hitWriter;
        for (size_t docId = 0; docId < HitBuffer_.Size(); docId++) {
            auto hits = HitBuffer_.Hits(docId);

            if (!hits.empty()) {
                hitWriter.Reset(
                    HitTable_.Get(),
                    WadOutput_.StartDocLump(docId, TWadLumpId(indexType, EWadLumpRole::Hits)),
                    WadOutput_.StartDocLump(docId, TWadLumpId(indexType, EWadLumpRole::HitSub))
                );
                for (const THit& hit : hits)
                    hitWriter.WriteHit(hit);
                hitWriter.Finish();
            }
        }
        WadOutput_.Finish();

        Output_.Reset();
    }

    bool IsFinished() const {
        return KeyWriter_.IsFinished();
    }

private:
    THolder<IOutputStream> Output_;
    THolder<THitTable> HitTable_ = MakeHolder<THitTable>();
    THolder<TKeyTable> KeyTable_ = MakeHolder<TKeyTable>();
    TMegaWadWriter WadOutput_;

    TAccumulatingOutput FatOutput_;
    TAccumulatingOutput FatSubOutput_;

    TKeyWriter KeyWriter_;
    THitBuffer HitBuffer_;
    TKeyBuffer KeyBuffer_;
};

} // namespace NDoom
