#pragma once

#include <utility>                  /* For std::forward. */

namespace NDoom {


template<class HitTransformation, class Base>
class THitTransformingIndexReader : public Base {
public:
    using THit = typename Base::THit;

    using Base::Base; /* For default-constructible hit transformation. */

    template<class... Args>
    THitTransformingIndexReader(const HitTransformation& hitTransformation, Args&&... args) : Base(std::forward<Args>(args)...), HitTransformation_(hitTransformation) {}

    bool ReadHit(THit* hit) {
        bool result = Base::ReadHit(hit);
        *hit = HitTransformation_(*hit);
        return result;
    }

private:
    HitTransformation HitTransformation_;
};


template<size_t FormBound>
class TFormBoundingHitTransformation {
public:
    template<class Hit>
    Hit operator()(const Hit& hit) const {
        Hit result = hit;
        result.SetForm(result.Form() % FormBound);
        return result;
    }
};


class TZeroRelevanceHitTransformation {
public:
    template<class Hit>
    Hit operator()(const Hit& hit) const {
        Hit result = hit;
        result.SetRelevance(0);
        return result;
    }
};

class TIdentityHitTransformation {
public:
    template<class Hit>
    Hit operator()(const Hit& hit) const {
        return hit;
    }
};


} // namespace NDoom
