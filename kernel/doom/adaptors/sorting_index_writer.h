#pragma once

#include <utility>

#include <util/generic/vector.h>
#include <util/generic/algorithm.h> /* For Sort. */

namespace NDoom {


template<class Base>
class TSortingIndexWriter: public Base {
public:
    using TKey = typename Base::TKey;
    using TKeyRef = typename Base::TKeyRef;
    using THit = typename Base::THit;

    template<class... Args>
    TSortingIndexWriter(Args&&... args): Base(std::forward<Args>(args)...) {}

    void WriteHit(const THit& hit) {
        Hits_.push_back(hit);
    }

    void WriteKey(const TKeyRef& key) {
        Sort(Hits_.begin(), Hits_.end());

        for (const THit& hit : Hits_)
            Base::WriteHit(hit);

        Base::WriteKey(key);
        Hits_.clear();
    }

private:
    TVector<THit> Hits_;
};


} // namespace NDoom


