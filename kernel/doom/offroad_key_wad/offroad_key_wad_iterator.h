#pragma once

#include <kernel/doom/wad/wad_index_type.h>

#include <library/cpp/offroad/key/key_reader.h>
#include <library/cpp/offroad/utility/tagged.h>

#include "combined_key_data_reader.h"

namespace NDoom {


template <class KeyData, class Vectorizer, class Subtractor, class Combiner>
class TOffroadKeyWadIterator: private NOffroad::NPrivate::TTaggedBase {
    using TKeyReader = TCombinedKeyDataReader<Combiner, NOffroad::TKeyReader<KeyData, Vectorizer, Subtractor>>;

public:
    using TKey = TString;
    using TKeyRef = TStringBuf;
    using TKeyData = KeyData;

    bool ReadKey(TKeyRef* key, TKeyData* data) {
        return KeyReader_.ReadKey(key, data);
    }

private:
    template <EWadIndexType anotherIndexType, class AnotherData, class AnotherVectorizer, class AnotherSubtractor, class AnotherSerializer, class AnotherCombiner>
    friend class TOffroadKeyWadSearcher;

    TKeyReader KeyReader_;
};


} // namespace NDoom
