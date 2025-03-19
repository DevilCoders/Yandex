#pragma once

#include <util/generic/vector.h>
#include <util/generic/algorithm.h> /* For Sort. */

namespace NDoom {


template<class Base>
class TSortingIndexReader: public Base {
public:
    using TKey = typename Base::TKey;
    using TKeyRef = typename Base::TKeyRef;
    using TKeyData = typename Base::TKeyData;
    using THit = typename Base::THit;

    template<class... Args>
    TSortingIndexReader(Args&&... args): Base(std::forward<Args>(args)...) {}

    bool ReadKey(TKeyRef* key, TKeyData* data = NULL) {
        Filled_ = false;
        Hits_.clear();
        return Base::ReadKey(key, data);
    }

    bool ReadHit(THit* outputHit) {
        if (!Filled_) {
            THit hit = THit();
            while (Base::ReadHit(&hit)) {
                Hits_.push_back(hit);
            }
            Sort(Hits_.begin(), Hits_.end());
            Position_ = 0;
            Filled_ = true;
        }
        if (Position_ < Hits_.size()) {
            *outputHit = Hits_[Position_];
            ++Position_;
            return true;
        }
        return false;
    }

private:
    TVector<THit> Hits_;
    size_t Position_ = 0;
    bool Filled_ = false;
};


} // namespace NDoom
