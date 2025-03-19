#pragma once

#include <util/generic/fwd.h>

namespace NDoom {


template <class Base, class KeyTransformation, class Key, class KeyRef = Key>
class TKeyTransformingIndexWriter: public Base {
public:
    using THit = typename Base::THit;
    using TKey = Key;
    using TKeyRef = KeyRef;
    using TBaseKeyRef = typename Base::TKeyRef;

    using Base::Base;

    TKeyTransformingIndexWriter() = default;

    template <class... Args>
    TKeyTransformingIndexWriter(const KeyTransformation& keyTransformation, Args... args)
        : Base(std::forward<Args>(args)...)
        , KeyTransformation_(keyTransformation)
    {
    }

    template <class... Args>
    void Reset(Args&&... args) {
        CurrentKeyHits_.clear();
        Base::Reset(std::forward<Args>(args)...);
    }

    void WriteHit(const THit& hit) {
        CurrentKeyHits_.push_back(hit);
    }

    void WriteKey(const TKeyRef& key) {
        if (CurrentKeyHits_.empty()) {
            return;
        }
        bool success = KeyTransformation_(key, &TransformedKey_);
        if (success) {
            for (const auto& hit : CurrentKeyHits_) {
                Base::WriteHit(hit);
            }
            Base::WriteKey(TransformedKey_);
        }
        CurrentKeyHits_.clear();
    }

private:
    KeyTransformation KeyTransformation_;
    TBaseKeyRef TransformedKey_;
    TVector<THit> CurrentKeyHits_;
};


} // namespace NDoom
