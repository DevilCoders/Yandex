#pragma once

#include <utility>                  /* For std::forward. */

namespace NDoom {


template<class HitFilter, class Base>
class THitFilteringIndexReader : public Base {
public:
    using THit = typename Base::THit;

    using Base::Base; /* For default-constructible hit filter. */

    template<class... Args>
    THitFilteringIndexReader(const HitFilter& hitFilter, Args&&... args) : Base(std::forward<Args>(args)...), HitFilter_(hitFilter) {}

    bool ReadHit(THit* hit) {
        while (Base::ReadHit(hit)) {
            if (HitFilter_(*hit)) {
                return true;
            } else {
                continue;
            }
        }

        return false;
    }

private:
    HitFilter HitFilter_;
};


} // namespace NDoom
