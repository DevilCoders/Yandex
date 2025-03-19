#pragma once

#include <util/string/printf.h>
#include <library/cpp/numerator/numerate.h>
#include <kernel/indexer/face/inserter.h>

class TExtBreaksHandler : public INumeratorHandler {
private:
    ui32 MaxTextBreak;
    ui32 MaxDistance;
    ui32 MaxBreaker;
    bool TextWithBreakers;

public:
    TExtBreaksHandler()
        : MaxTextBreak(0)
        , MaxDistance(0)
        , MaxBreaker(1)
        , TextWithBreakers(0)
    {
    }

    void OnMoveInput(const THtmlChunk&, const TZoneEntry* zone, const TNumerStat& stat) override {
        if (!zone || zone->IsNofollow())
            return;
        for (size_t i = 0; i < zone->Attrs.size(); ++i) {
            const TAttrEntry& attr = zone->Attrs[i];
            if (attr.Pos != APOS_DOCUMENT &&
                    (  (0 == strncmp(~attr.Name, "action", 6))
                    || (0 == strncmp(~attr.Name, "applet", 6))
                    || (0 == strncmp(~attr.Name, "image", 5))
                    || (0 == strncmp(~attr.Name, "link", 4))
                    || (0 == strncmp(~attr.Name, "linkint", 7))
                    || (0 == strncmp(~attr.Name, "linkmus", 7))
                    || (0 == strncmp(~attr.Name, "object", 6))
                    || (0 == strncmp(~attr.Name, "script", 6))
            ))
            {
                ui32 CurrentBreaker = stat.TokenPos.Break();

                if (CurrentBreaker > 0) {
                    MaxDistance = Max(MaxDistance, CurrentBreaker - MaxBreaker);
                    MaxBreaker = CurrentBreaker;
                    TextWithBreakers = true;
                }
            }
        }
    }

    void OnTokenStart(const TWideToken& , const TNumerStat& stat) override {
        MaxTextBreak = stat.TokenPos.Break();
    }

    void OnTextEnd(const IParsedDocProperties* , const TNumerStat& ) override {
        if (MaxTextBreak > MaxBreaker)
            MaxDistance = Max(MaxDistance, MaxTextBreak - MaxBreaker);
    }

    ui32 GetTextLength() const {
        return MaxTextBreak;
    }

    ui32 CalculateLongTextValue() const {
        return TextWithBreakers ? ui32(100. * float(MaxDistance) / MaxTextBreak) : MaxTextBreak;
    }

    void InsertFactors(IDocumentDataInserter& inserter) const {
        inserter.StoreErfDocAttr("Breaks", Sprintf("%d", GetTextLength()));
        inserter.StoreErfDocAttr("LongText", Sprintf("%d", CalculateLongTextValue()));
    }
};
