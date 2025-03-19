#pragma once

#include "superlemmer_version.h"

#include <library/cpp/containers/comptrie/comptrie.h>

#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <util/system/yassert.h>

namespace NSuperLemmer {
    using TSuperFlex = std::pair<ui16, TString>;
    using TSuperLemmerTrie = TCompactTrie<char, TSuperFlex>;

    class TSuperLemmer {
    private:
        TBlob Data;
        TSuperLemmerTrie Trie;

    public:
        TSuperLemmer(); // constructs the default superlemmer with built-in dictionary

        TSuperLemmer(TBlob data);

        void ApplyInplace(TString& key) const {
            TSuperFlex value;
            if (Trie.Find(key.c_str(), key.size(), &value)) {
                const auto& cut = value.first;
                const auto& end = value.second;
                Y_ASSERT(cut <= key.size());
                key.resize(key.size() - cut);
                key += end;
            }
        }

        TString Apply(const TStringBuf& key) {
            TString result{key};
            ApplyInplace(result);
            return result;
        }
    };

    inline void ApplyInplace(TString& key, ESuperLemmerVersion version = DefaultSuperLemmer) {
        if (version != NoSuperLemmer) {
            Singleton<TSuperLemmer>()->ApplyInplace(key);
        }
    }

    inline TString Apply(const TStringBuf& key, ESuperLemmerVersion version = DefaultSuperLemmer) {
        if (version != NoSuperLemmer) {
            return Singleton<TSuperLemmer>()->Apply(key);
        } else {
            return TString(key);
        }
    }
}
