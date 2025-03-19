#pragma once

#include <library/cpp/wordpos/wordpos.h>
#include <util/generic/vector.h>

namespace NIndexerCore {

    class TDirectTextCreator;

    struct TExtraText {
        const char* Zone; // can be equal to NULL
        const wchar16* Text;
        size_t TextLen;
        RelevLevel Relev;

        TExtraText()
            : Zone(nullptr)
            , Text(nullptr)
            , TextLen(0)
            , Relev(MID_RELEV)
        { }
    };
    typedef TVector<TExtraText> TExtraTextZones;
    void AppendExtraText(TDirectTextCreator&, const TExtraTextZones&);

} // namespace NIndexerCore
