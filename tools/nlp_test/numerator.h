#pragma once

#include <library/cpp/html/pdoc/pds.h>
#include <library/cpp/numerator/numerate.h>

#include "output.h"

namespace NDetail {

static const wchar16* nullToken = nullptr;

class TNumerator : public INumeratorHandler {
private:
    TOutput mOutput;
    TString Indent;

protected:
    void OnTokenStart(const TWideToken& token, const TNumerStat& stat) override
    {
        mOutput.Write(Indent.c_str(), "Word:", stat.TokenPos,  stat.IsOverflow(), nullptr, token);
    }
    void OnSpaces(TBreakType t, const TChar* token, unsigned length, const TNumerStat& stat) override
    {
        if (token != nullptr)
            if (!IsSentBrk(t))
                mOutput.Write(Indent.c_str(), "Space:", stat.TokenPos,  stat.IsOverflow(), nullptr, token, length);
            else if (IsParaBrk(t))
                mOutput.Write(Indent.c_str(), "Para:", stat.TokenPos,  stat.IsOverflow(), nullptr, token, length);
            else
                mOutput.Write(Indent.c_str(), "Break", stat.TokenPos,  stat.IsOverflow(), nullptr, token, length);
        else
            mOutput.Write(Indent.c_str(), "Para Break:", stat.TokenPos,  stat.IsOverflow(), nullptr);
    }

    void OnMoveInput(const THtmlChunk&, const TZoneEntry* zone, const TNumerStat& stat) override {
        if (!zone || zone->IsNofollow())
            return;
        if (!zone->OnlyAttrs) {
            if (zone->IsOpen) {
                mOutput.Write(Indent.c_str(), "Zone:(", stat.TokenPos,  stat.IsOverflow(), zone->Name, nullToken, 0);
                Indent += "  ";
            }
            if (zone->IsClose) {
                if (!Indent.empty())
                    Indent.resize(Indent.size() - 2);
                mOutput.Write(Indent.c_str(), ")", stat.TokenPos,  stat.IsOverflow(), zone->Name, nullToken, 0);
            }
        }
        for (size_t i = 0; i < zone->Attrs.size(); ++i) {
            mOutput.Write(Indent.c_str(), "Attr:", stat.TokenPos,  stat.IsOverflow(), ~zone->Attrs[i].Name, zone->Attrs[i].DecodedValue.data(),
                          zone->Attrs[i].DecodedValue.size());
        }
    }

public:
    TNumerator(IOutputStream& stream)
        : mOutput(stream)
    {}
};

} // namespace NDetail


inline
void DoTest(THtProcessor* htProcessor, IParsedDocProperties* docProps, IOutputStream& stream)
{
    NDetail::TNumerator numerator(stream);
    htProcessor->NumerateHtml(numerator, docProps);
}
