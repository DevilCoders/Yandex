#pragma once

#include "arcreader.h"

// velavokr@ is author of this code
class TTArcViewSentReader : public TSentReader {
private:
    TUtf16String SegStMarker;
    TUtf16String DaterDateMarker;
    bool SentAttrs;
    bool DecodeSentAttrs;

public:
    TTArcViewSentReader(const bool sentAttrs, const bool decodeSentAttrs);
    void DecodeSegStAttribute(TUtf16String& raw, const TString& s, ui8 segVersion) const;
    TUtf16String GetText(const TArchiveSent& sent, ui8 segVersion) const override;
};
