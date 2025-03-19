#include "duplicate_key_filter.h"

#include "abstract_trie.h"

#include <util/generic/hash_set.h>

namespace NSaasTrie {
    struct TDuplicateKeyFilter : ITrieStorageIterator {
        explicit TDuplicateKeyFilter(THolder<ITrieStorageIterator> source)
            : Source(std::move(source))
        {
            if (!Source->AtEnd()) {
                VisitedKeys.emplace(Source->GetKey());
            }
        }

        bool AtEnd() const override {
            return Source->AtEnd();
        }
        TString GetKey() const override {
            return Source->GetKey();
        }
        ui64 GetValue() const override {
            return Source->GetValue();
        }
        bool Next() override {
            while (Source->Next()) {
                auto key = Source->GetKey();
                auto inserted = VisitedKeys.emplace(std::move(key));
                if (inserted.second) {
                    return true;
                }
            }
            return false;
        }

    private:
        THolder<ITrieStorageIterator> Source;
        THashSet<TString> VisitedKeys;
    };


    THolder<ITrieStorageIterator> FilterDuplicateKeys(THolder<ITrieStorageIterator> source) {
        return MakeHolder<TDuplicateKeyFilter>(std::move(source));
    }
}

