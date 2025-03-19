#pragma once

#include "abstract_trie.h"

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

#include <array>

namespace NSaasTrie {
    namespace NPrivate {
        template<size_t Size, typename T>
        void FillArrayTail(std::array<T, Size>& /*arr*/, size_t /*from*/) {
        }
        template<size_t Size, typename T, typename U>
        void FillArrayTail(std::array<T, Size>& arr, size_t from, U&& u) {
            arr[from] = std::forward<U>(u);
        }
        template<size_t Size, typename T, typename U, typename... V>
        void FillArrayTail(std::array<T, Size>& arr, size_t from,  U&& u, V&&... v) {
            arr[from] = std::forward<U>(u);
            FillArrayTail(arr, from + 1, std::forward<V>(v)...);
        }
    }

    template<size_t Size>
    struct TTrieIteratorChain : ITrieStorageIterator {
        template<typename... TIteratorPtr>
        explicit TTrieIteratorChain(TIteratorPtr&&... iterators) {
            static_assert(sizeof... (TIteratorPtr) == Size, "Wrong number of arguments");
            NPrivate::FillArrayTail(Iterators, 0, std::forward<TIteratorPtr>(iterators)...);
            for (auto& iterator : Iterators) {
                Y_ENSURE(iterator);
            }
            FindNotEmptyIterator();
        }

        bool AtEnd() const override {
            return Current >= Size;
        }
        TString GetKey() const override {
            return Iterators[Current]->GetKey();
        }
        ui64 GetValue() const override {
            return Iterators[Current]->GetValue();
        }
        bool Next() override {
            if (Current >= Size) {
                return false;
            }
            if (Iterators[Current]->Next()) {
                return true;
            }
            ++Current;
            return FindNotEmptyIterator();
        }

    private:
        bool FindNotEmptyIterator() {
            for (; Current < Size; ++Current) {
                if (!Iterators[Current]->AtEnd()) {
                    return true;
                }
            }
            return false;
        }

        std::array<THolder<ITrieStorageIterator>, Size> Iterators;
        size_t Current = 0;
    };

    template<typename... TIteratorPtr>
    THolder<ITrieStorageIterator> CreateTrieIteratorChain(TIteratorPtr&&... iterators) {
        constexpr size_t Size = sizeof... (TIteratorPtr);
        return MakeHolder<TTrieIteratorChain<Size>>(std::forward<TIteratorPtr>(iterators)...);
    }

    using TTrieIteratorVector = TVector<THolder<ITrieStorageIterator>>;

    THolder<ITrieStorageIterator> DecorateTrieIteratorVector(TTrieIteratorVector iterators);
}

