#pragma once

#include <util/generic/ylimits.h>
#include <util/system/types.h>

#include <utility>

namespace NDoom {

template<typename CategType>
class TGenericDocAttrsHit {
public:
    TGenericDocAttrsHit() = default;

    TGenericDocAttrsHit(ui32 attrNum, CategType categ)
        : AttrNum_(attrNum)
        , Categ_(categ)
    {
    }

    ui32 AttrNum() const {
        return AttrNum_;
    }

    CategType Categ() const {
        return Categ_;
    }

    void SetAttrNum(ui32 attrNum) {
        AttrNum_ = attrNum;
    }

    void SetCateg(CategType categ) {
        Categ_ = categ;
    }

    bool operator<(const TGenericDocAttrsHit& hit) const {
        return Pair() < hit.Pair();
    }

    bool operator>(const TGenericDocAttrsHit& hit) const {
        return Pair() > hit.Pair();
    }

    bool operator<=(const TGenericDocAttrsHit& hit) const {
        return Pair() <= hit.Pair();
    }

    bool operator>=(const TGenericDocAttrsHit& hit) const {
        return Pair() >= hit.Pair();
    }

    bool operator==(const TGenericDocAttrsHit& hit) const {
        return Pair() == hit.Pair();
    }

    bool operator!=(const TGenericDocAttrsHit& hit) const {
        return Pair() != hit.Pair();
    }

private:
    std::pair<ui32, CategType> Pair() const {
        return std::make_pair(AttrNum_, Categ_);
    }

    friend IOutputStream& operator<<(IOutputStream& stream, const TGenericDocAttrsHit& hit) {
        stream << "[" << hit.AttrNum() << ":" << hit.Categ() << "]";
        return stream;
    }

    ui32 AttrNum_ = 0;
    CategType Categ_ = 0;
};

using TDocAttrsHit = TGenericDocAttrsHit<ui32>;
using TDocAttrs64Hit = TGenericDocAttrsHit<ui64>;

} // namespace NDoom
