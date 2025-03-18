#pragma once

namespace NIter {
    template <class TDirectAccessContainer, class TValue = typename TDirectAccessContainer::value_type>
    class TRangedForIteratorAdapter {
    public:
        TRangedForIteratorAdapter(const TDirectAccessContainer* container, size_t index)
            : Container(container)
            , Index(index)
        {
        }

        inline const TValue& operator*() const {
            return (*Container)[Index];
        }

        inline TRangedForIteratorAdapter<TDirectAccessContainer, TValue>& operator++() {
            ++Index;
            return *this;
        }

        inline bool operator!=(const TRangedForIteratorAdapter<TDirectAccessContainer, TValue>& iter) const {
            return !(iter.Index == Index && iter.Container == Container);
        }

    private:
        const TDirectAccessContainer* Container;
        size_t Index;
    };

}
