#pragma once

#include "markup.h"

#include <kernel/qtree/richrequest/markup/wtypes/wtypes.h>      // enum declarations, string <-> enum conversions

#include <util/generic/ymath.h>

const int VOTING_CONTEXT_VECTOR_SIZE = 25;

class TOntoTypeMarkup: public NSearchQuery::TMarkupData<NSearchQuery::MT_ONTO> {
public:
    EOntoCatsType Type;
    float Weight;
    float One;
    EOntoIntsType Intent;
    float IntWght;

public:
    TOntoTypeMarkup()
    : Type(ONTO_UNKNOWN)
      , Weight(0.0)
      , One(0.0)
      , Intent(ONTO_INTS_UNKNOWN)
      , IntWght(0.0)
    {}

    TOntoTypeMarkup(EOntoCatsType type, float weight, float one, EOntoIntsType inttype, float intweight)
    : Type(type)
      , Weight(weight)
      , One(one)
      , Intent(inttype)
      , IntWght(intweight)
    {}

    bool Merge(TMarkupDataBase& newNode) override {
        const TOntoTypeMarkup& nn = newNode.As<TOntoTypeMarkup>();
        if(nn.Type == Type && nn.Intent == Intent) {
            Weight = nn.Weight;
            One = nn.One;
            IntWght = nn.IntWght;
            return true;
        }
        else
            return false;
    }

    bool operator ==(const TOntoTypeMarkup& rhs) const {
        static const double eps = 1.0e-6;
        return (Type == rhs.Type) && (fabs(Weight - rhs.Weight) <= fabs(Weight) * eps) && (fabs(One - rhs.One) <= fabs(One) * eps) && (Intent == rhs.Intent) && (fabs(IntWght - rhs.IntWght) <= fabs(IntWght) * eps);
    }

    NSearchQuery::TMarkupDataPtr Clone() const override {
        return new TOntoTypeMarkup(*this);
    }

    bool Serialize(NRichTreeProtocol::TMarkupDataBase& message, bool humanReadable) const override;
    static NSearchQuery::TMarkupDataPtr Deserialize(const NRichTreeProtocol::TMarkupDataBase& message);
protected:
    bool DoEqualTo(const TMarkupDataBase& rhs) const override {
        return *this == rhs.As<TOntoTypeMarkup>();
    }
};
