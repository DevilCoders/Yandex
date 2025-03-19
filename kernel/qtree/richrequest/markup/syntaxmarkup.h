#pragma once

#include "markup.h"

#include <util/generic/string.h>

class TSyntaxMarkup: public NSearchQuery::TMarkupData<NSearchQuery::MT_SYNTAX> {
public:
    TUtf16String SyntaxType;
    ui32 Level;
public:
    TSyntaxMarkup()
        : Level(0)
    {
    }

    TSyntaxMarkup(const TUtf16String& type, ui32 level)
        : SyntaxType(type)
        , Level(level)
    {
    }

    bool Merge(TMarkupDataBase& newNode) override {
        const TSyntaxMarkup& nn = newNode.As<TSyntaxMarkup>();
        if (nn.Level == Level) {
            SyntaxType = nn.SyntaxType;
            return true;
        } else {
            return false;
        }
    }

    inline bool operator ==(const TSyntaxMarkup& rhs) const {
        return Level == rhs.Level && SyntaxType == rhs.SyntaxType;
    }

    NSearchQuery::TMarkupDataPtr Clone() const override {
        return new TSyntaxMarkup(*this);
    }

    bool Serialize(NRichTreeProtocol::TMarkupDataBase& message, bool humanReadable) const override;
    static NSearchQuery::TMarkupDataPtr Deserialize(const NRichTreeProtocol::TMarkupDataBase& message);
protected:
    bool DoEqualTo(const TMarkupDataBase& rhs) const override {
        return *this == rhs.As<TSyntaxMarkup>();
    }
};
