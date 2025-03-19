#pragma once

namespace NDoom {

template<class HitEqual, class Base>
class TUniqueIndexReader: public Base {
public:
    using TKey = typename Base::TKey;
    using TKeyRef = typename Base::TKeyRef;
    using TKeyData = typename Base::TKeyData;
    using THit = typename Base::THit;

    using Base::Base;

    template<class... Args>
    TUniqueIndexReader(const HitEqual& comparator, Args&&... args)
        : Base(std::forward<Args>(args)...)
        , HitEqual_(comparator)
    {
    }

    bool ReadKey(TKeyRef* key, TKeyData* data = NULL) {
        LastHit_.Clear();
        return Base::ReadKey(key, data);
    }

    bool ReadHit(THit* hit) {
        while (Base::ReadHit(hit)) {
            if (!LastHit_.Empty() && HitEqual_(*hit, *LastHit_))
                continue;

            LastHit_ = *hit;
            return true;
        }

        return false;
    }

private:
    TMaybe<THit> LastHit_;
    HitEqual HitEqual_;
};

struct TDocBreakWordHitEqual {
    template <class THit>
    bool operator() (const THit& l, const THit& r) const {
        return l.DocId() == r.DocId() && l.Break() == r.Break() && l.Word() == r.Word();
    }
};

} // namespace NDoom
