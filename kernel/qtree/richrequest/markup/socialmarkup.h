#pragma once

#include "markup.h"

enum ESocialOnto {
    SO_CITY = 1,
    SO_COUNTRY = 2,
    SO_NETWORK = 3,
    SO_DATE = 4,
    SO_UNIVERSITY = 5,
    SO_SCHOOL = 6,
    SO_PROFESSION = 7,
    SO_COMPANY = 8
};

class TSocialMarkup : public NSearchQuery::TMarkupData<NSearchQuery::MT_SOCIAL> {
private:
    ESocialOnto Type;

public:
    TSocialMarkup(ESocialOnto type)
        : Type(type)
    {}

    ESocialOnto GetType() const {
        return Type;
    }

    NSearchQuery::TMarkupDataPtr Clone() const override {
        return new TSocialMarkup(*this);
    }

    bool Serialize(NRichTreeProtocol::TMarkupDataBase& message, bool /*humanReadable*/) const override;
    static NSearchQuery::TMarkupDataPtr Deserialize(const NRichTreeProtocol::TMarkupDataBase& message);

private:
    bool DoEqualTo(const TMarkupDataBase& rhs) const override {
        return Type == rhs.As<TSocialMarkup>().Type;
    }
};

