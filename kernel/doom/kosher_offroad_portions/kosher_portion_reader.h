#pragma once

#include <kernel/doom/progress/progress.h>

#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>
#include <library/cpp/offroad/key/key_reader.h>
#include <library/cpp/offroad/tuple/tuple_reader.h>
#include <library/cpp/offroad/offset/data_offset.h>

namespace NDoom {

template <class Hit, class HitVectorizer, class HitSubtractor>
class TKosherPortionReader {
public:
    using THit = Hit;
    using THitReader = NOffroad::TTupleReader<THit, HitVectorizer, HitSubtractor, NOffroad::TDecoder64, 1, NOffroad::PlainOldBuffer>;
    using THitModel = typename THitReader::TModel;
    using THitTable = typename THitReader::TTable;

    using TKey = TString;
    using TKeyRef = TStringBuf;
    using TKeyData = ui32;
    using TKeyReader = NOffroad::TKeyReader<TKeyData, NOffroad::TUi32Vectorizer, NOffroad::TI1Subtractor>;
    using TKeyModel = typename TKeyReader::TModel;
    using TKeyTable = typename TKeyReader::TTable;

    TKosherPortionReader() = default;

    TKosherPortionReader(const THitTable* hitTable, const TArrayRef<const char>& hitSource, const TKeyTable* keyTable, const TArrayRef<const char>& keySource)
        : HitReader_(hitTable, hitSource)
        , KeyReader_(keyTable, keySource)
    {

    }

    void Reset(const THitTable* hitTable, const TArrayRef<const char>& hitSource, const TKeyTable* keyTable, const TArrayRef<const char>& keySource) {
        HitReader_.Reset(hitTable, hitSource);
        KeyReader_.Reset(keyTable, keySource);

        HitsLeft_ = 0;
    }

    void Restart() {
        HitReader_.Restart();
        KeyReader_.Restart();
        HitsLeft_ = 0;
    }

    bool ReadKey(TKeyRef* key) {
        TKeyData data;
        return ReadKey(key, &data);
    }

    bool ReadKey(TKeyRef* key, TKeyData* data) {
        Y_ENSURE(HitsLeft_ == 0);
        if (KeyReader_.ReadKey(key, data)) {
            HitsLeft_ = *data;
            return HitReader_.Seek(HitReader_.Position(), THit(), NOffroad::SeekPointSeek);
        } else {
            return false;
        }
    }

    bool ReadHit(THit* hit) {
        if (HitsLeft_ > 0) {
            --HitsLeft_;
            return HitReader_.ReadHit(hit);
        }
        return false;
    }

    TProgress Progress() const {
        return TProgress(); // TODO
    }

private:
    ui32 HitsLeft_ = 0;
    THitReader HitReader_;
    TKeyReader KeyReader_;
};

} // namespace NDoom
