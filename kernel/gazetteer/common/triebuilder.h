#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/map.h>
#include <util/generic/buffer.h>
#include <util/stream/buffer.h>
#include <util/memory/blob.h>
#include <util/memory/pool.h>

#include <util/ysaveload.h>

#include <library/cpp/containers/comptrie/comptrie.h>

#include "tools.h"
#include "traits.h"
#include "bytesize.h"


// This class is more optimized version of TCompactTrieBuilder.
// It is populated with key,value pairs and eventually serialized
// into blob compatible with TCompactTrie.
//
// It is generally faster and uses substantially less memory,
// compared to TCompactTrieBuilder. In addition, it provides
// an opportunity for trading-off between
// compilation speed and consumed memory (see @maxSize in constructor).
//
template <typename TKeyChar, typename TValue, typename TPacker = TCompactTriePacker<TValue> >
class TMoreCompactTrieBuilder
{
    typedef typename NGzt::TCharStringTraits<TKeyChar>::TStringType TKeyString;
    typedef TBasicStringBuf<TKeyChar> TKey;
    typedef TCompactTrie<TKeyChar, TValue, TPacker> TTrie;
    typedef TCompactTrieBuilder<TKeyChar, TValue, TPacker> TTrieBuilder;
public:
    TMoreCompactTrieBuilder(size_t maxSize = 64*1000000)    // 64M
        : MaxInsertedSize(maxSize)
        , InsertedKeysPool(1024)
    {
    }

    bool Find(const TKeyChar* key, size_t keylen, TValue* res) const;
    bool Insert(const TKeyChar* key, size_t keylen, const TValue& value, TValue** existing);

    void Save(IOutputStream& out) const {
        TBuffer buf;
        Merge(buf);
        // NOTE: saved trie does not know its size, the caller should take care about.
        ::SavePodArray(&out, buf.data(), buf.size());
    }

    size_t ByteSize() const {
        return ::ByteSize(InsertedKeysPool)
             + ::ByteSize(Inserted)
             + ::ByteSize(Compacted.Data());
    }

private:
    void Merge(TBuffer& buffer) const;

    void Compact() {
        TBuffer buf;
        Merge(buf);
        Compacted.Init(TBlob::FromBuffer(buf));
        Inserted.clear();
        InsertedKeysPool.Clear();
    }

    size_t MaxInsertedSize;
    TMemoryPool InsertedKeysPool;
    TMap<TKey, TValue> Inserted;
    TTrie Compacted;
};



// ============================================================================
// Template implementations ===================================================
// ============================================================================

template <typename TKeyChar, typename TValue, typename TPacker>
bool TMoreCompactTrieBuilder<TKeyChar, TValue, TPacker>::Find(const TKeyChar* key, size_t keylen, TValue* res) const {
    typename TMap<TKey, TValue>::const_iterator ins = Inserted.find(TKey(key, keylen));
    if (ins != Inserted.end()) {
        *res = ins->second;
        return true;
    } else
        return Compacted.Find(key, keylen, res);
}

template <typename TKeyChar, typename TValue, typename TPacker>
bool TMoreCompactTrieBuilder<TKeyChar, TValue, TPacker>::Insert(const TKeyChar* keyp, size_t keylen, const TValue& value, TValue** existing) {
    if (Inserted.size() >= MaxInsertedSize)
        Compact();

    typedef TMap<TKey, TValue> TMapType;
    TKey key(keyp, keylen);

    std::pair<typename TMapType::iterator, bool> ins = Inserted.insert(std::make_pair(key, value));
    *existing = &(ins.first->second);

    if (!ins.second)
        return false;       // already have this key

    // dirty hack: we must replace already inserted key with its pool-stored version
    // (the alternative will be a second insert which is slower)
    const_cast<TKey&>(ins.first->first) = TKey(InsertedKeysPool.Append(key.data(), key.size()), key.size());

    TValue compacted;
    if (Compacted.Find(key.data(), key.size(), &compacted)) {
        **existing = compacted;  // restore previous value from @Compacted
        return false;
    } else
        return true;
}

template <typename TKeyChar, typename TValue, typename TPacker>
void TMoreCompactTrieBuilder<TKeyChar, TValue, TPacker>::Merge(TBuffer& buffer) const {

    TTrieBuilder builder(CTBF_PREFIX_GROUPED);

    typedef typename TMap<TKey, TValue>::const_iterator TMapIter;
    typedef typename TTrie::TConstIterator TTrieIter;
    TMapIter mapIt = Inserted.begin();
    TTrieIter trieIt = Compacted.Begin();

    if (trieIt != Compacted.End()) {
        TKeyString trieKey = trieIt.GetKey();

        while (mapIt != Inserted.end()) {
            const TKey& mapKey = mapIt->first;
            int cmp = mapKey.compare(TBasicStringBuf<TKeyChar>{trieKey.data(), trieKey.size()});
            if (cmp < 0) {          // mapKey < trieKey
                builder.Add(mapKey.data(), mapKey.size(), mapIt->second);
                ++mapIt;
                continue;

            } else if (cmp > 0) {   // trieKey < mapKey
                builder.Add(trieKey.data(), trieKey.size(), trieIt.GetValue());
                ++trieIt;

            } else {                // equal
                // @inserted overrides @compacted
                builder.Add(mapKey.data(), mapKey.size(), mapIt->second);
                ++mapIt;
                ++trieIt;
            }

            if (trieIt == Compacted.End())
                break;
            trieKey = trieIt.GetKey();
        }
    }

    // remainings
    for (; trieIt != Compacted.End(); ++trieIt) {
        builder.Add(trieIt.GetKey(), trieIt.GetValue());
    }

    for (; mapIt != Inserted.end(); ++mapIt) {
        const TKey& mapKey = mapIt->first;
        builder.Add(mapKey.data(), mapKey.size(), mapIt->second);
    }

    TBufferOutput bufout(buffer);
    builder.Save(bufout);
}

