#pragma once

#include <util/generic/array_ref.h>
#include <util/generic/iterator_range.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>
#include <util/system/yassert.h>

#include <algorithm>

template <class TOrderedContainer, class TKey, class TIterator>
auto LowerBoundHinted(TOrderedContainer&& container, const TKey& key, TIterator begin) {
    if constexpr (std::is_same<typename std::iterator_traits<TIterator>::iterator_category, std::random_access_iterator_tag>::value) {
        Y_ASSERT(std::is_sorted(begin, container.end()));
        return std::lower_bound(begin, container.end(), key);
    } else {
        return container.lower_bound(key);
    }
}

template <class TOrderedContainer, class TKey>
auto LowerBound(TOrderedContainer&& container, const TKey& key) {
    return LowerBoundHinted(container, key, container.begin());
}

template <class TOrderedContainer0, class TOrderedContainer1>
bool IsIntersectionEmpty(const TOrderedContainer0& first, const TOrderedContainer1& second) {
    auto i = first.begin();
    auto j = second.begin();
    while (i != first.end() && j != second.end()) {
        if (*i < *j) {
            i = LowerBoundHinted(first, *j, i);
        } else if (*j < *i) {
            j = LowerBoundHinted(second, *i, j);
        } else {
            return false;
        }
    }
    return true;
}

template <class TOrderedContainer0, class TOrderedContainer1>
bool HasIntersection(const TOrderedContainer0& first, const TOrderedContainer1& second) {
    return !IsIntersectionEmpty(first, second);
}

template <class T, class TContainer>
TSet<T> MakeSet(const TContainer& container) {
    return { container.begin(), container.end() };
}

template <class TContainer>
TSet<typename TContainer::value_type> MakeSet(const TContainer& container) {
    return { container.begin(), container.end() };
}

template <class T, class TContainer>
TVector<T> MakeVector(const TContainer& container) {
    return { container.begin(), container.end() };
}

template <class TContainer>
TVector<typename TContainer::value_type> MakeVector(const TContainer& container) {
    return { container.begin(), container.end() };
}

namespace NContainer {
    namespace NPrivate {
        template <size_t N, class TIterator>
        class TTupleIteratorWrapper {
        public:
            using iterator_category = typename TIterator::iterator_category;
            using direct_value_type = typename std::tuple_element<N, typename TIterator::value_type>::type;
            using value_type = typename std::remove_cv<direct_value_type>::type;
            using difference_type = typename TIterator::difference_type;

            using pointer = direct_value_type*;
            using reference = direct_value_type&;

        public:
            TTupleIteratorWrapper(TIterator iterator)
                : Iterator(iterator)
            {
            }

            auto operator*() const noexcept {
                return Dereference();
            }
            auto operator->() const noexcept {
                return &Dereference();
            }
            auto operator++() {
                ++Iterator;
                return *this;
            }
            bool operator==(const TTupleIteratorWrapper<N, TIterator>& other) const {
                return Iterator == other.Iterator;
            }
            bool operator!=(const TTupleIteratorWrapper<N, TIterator>& other) const {
                return Iterator != other.Iterator;
            }

        private:
            auto Dereference() const noexcept {
                return std::get<N>(*Iterator);
            }

        private:
            TIterator Iterator;
        };

        template <size_t N, class TIterator>
        auto Wrap(TIterator iterator) {
            return TTupleIteratorWrapper<N, TIterator>(std::move(iterator));
        }
    }

    template <class TKey, class TValue>
    const TValue* GetMapValuePtr(const TMap<TKey, TValue>& kv, const TKey& key) {
        auto it = kv.find(key);
        return (it == kv.end()) ? nullptr : &it->second;
    }

    template <size_t N, class TContainer>
    auto Element(TContainer&& container) {
        return MakeIteratorRange(NPrivate::Wrap<N>(container.begin()), NPrivate::Wrap<N>(container.end()));
    }

    template <class TContainer>
    auto Keys(TContainer&& container) {
        return Element<0>(std::forward<TContainer>(container));
    }

    template <class TContainer>
    auto Values(TContainer&& container) {
        return Element<1>(std::forward<TContainer>(container));
    }

    template <class T>
    auto Scalar(T& value) {
        return MakeArrayRef(&value, 1);
    }
}
