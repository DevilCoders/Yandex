#include "trie_iterator_chain.h"

#include "abstract_trie.h"

namespace NSaasTrie {
    struct TTrieIteratorVectorDecorator : ITrieStorageIterator {
        using TVectorIterator = TTrieIteratorVector::const_iterator;

        explicit TTrieIteratorVectorDecorator(TTrieIteratorVector iterators)
            : Iterators(std::move(iterators))
        {
            Current = Iterators.cbegin();
            End = Iterators.cend();
            FindNotEmptyIterator();
        }

        bool AtEnd() const override {
            return Current == End;
        }
        TString GetKey() const override {
            return (**Current).GetKey();
        }
        ui64 GetValue() const override {
            return (**Current).GetValue();
        }
        bool Next() override {
            if (Current == End) {
                return false;
            }
            if ((**Current).Next()) {
                return true;
            }
            ++Current;
            return FindNotEmptyIterator();
        }

    private:
        bool FindNotEmptyIterator() {
            for (; Current != End; ++Current) {
                if (!(**Current).AtEnd()) {
                    return true;
                }
            }
            return false;
        }

        TTrieIteratorVector Iterators;
        TVectorIterator Current;
        TVectorIterator End;
    };

    THolder<ITrieStorageIterator> DecorateTrieIteratorVector(TTrieIteratorVector iterators) {
        return MakeHolder<TTrieIteratorVectorDecorator>(std::move(iterators));
    }
}

