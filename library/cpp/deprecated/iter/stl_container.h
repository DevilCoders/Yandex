#pragma once

#include "begin_end.h"

#include <util/generic/vector.h>
#include <util/generic/set.h>
#include <util/generic/typetraits.h>

namespace NIter {
    template <class TContainer>
    class TStlContainerTypeTraits {
        using TValueType = typename TContainer::value_type;
        using TNonConstContainer = std::remove_const_t<TContainer>;

    public:
        using TNonConstValueType = std::remove_const_t<TValueType>;
        using TConstValueType = const TNonConstValueType;

        using TConstIter = typename TNonConstContainer::const_iterator;
        using TNonConstIter = typename TNonConstContainer::iterator;

        enum { IsConst = std::is_const<TContainer>::value };

    public:
        using TValue = std::conditional_t<IsConst, TConstValueType, TNonConstValueType>;
        using TIterator = std::conditional_t<IsConst, TConstIter, TNonConstIter>;
    };

    template <class T>
    class TStlContainerSelector {
        enum { IsConst = std::is_const<T>::value };
        using TNonConst = std::remove_const_t<T>;

    public:
        using TVectorType = std::conditional_t<IsConst, const TVector<TNonConst>, TVector<TNonConst>>;
        using TSetType = std::conditional_t<IsConst, const TSet<TNonConst>, TSet<TNonConst>>;
    };

    // If TContainer is const then iterator would return only const reference to iterated items
    template <class TContainer>
    class TStlContainerIterator: public TBeginEndIterator<typename TStlContainerTypeTraits<TContainer>::TIterator,
                                                           typename TStlContainerTypeTraits<TContainer>::TValue> {
    public:
        using TValue = typename TStlContainerTypeTraits<TContainer>::TValue;
        using TStlIter = typename TStlContainerTypeTraits<TContainer>::TIterator;
        using TBase = TBeginEndIterator<TStlIter, TValue>;

        inline TStlContainerIterator()
            : TBase()
        {
        }

        inline explicit TStlContainerIterator(TContainer& container)
            : TBase(container.begin(), container.end())
        {
        }

        inline TStlContainerIterator(TStlIter it_begin, TStlIter it_end)
            : TBase(it_begin, it_end)
        {
        }

        using TBeginEndIterator<TStlIter, TValue>::Reset;

        inline void Reset(TContainer& container) {
            TBase::Reset(container.begin(), container.end());
        }
    };

}
