#pragma once

#include <kernel/doom/wad/wad.h>
#include <library/cpp/offroad/utility/tagged.h>

#include "struct_diff_reader.h"


namespace NDoom {


template <class Key, class KeyVectorizer, class KeySubtractor, class Data>
class TOffroadStructDiffWadIterator: private NOffroad::NPrivate::TTaggedBase {
    using TReader = TStructDiffReader<Key, KeyVectorizer, KeySubtractor, Data>;
public:
    using TKey = Key;
    using TData = Data;

    inline bool Next(TKey* key, const TData** data) {
        return Reader_.Read(key, data);
    }

private:
    template <EWadIndexType indexType, class OtherKey, class OtherKeyVectorizer, class OtherKeySubtractor, class OtherKeyPrefixVectorizer, class OtherData>
    friend class TOffroadStructDiffWadSearcher;

    TReader Reader_;
};


} // namespace NDoom
