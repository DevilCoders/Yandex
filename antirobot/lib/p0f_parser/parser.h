#pragma once

#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/generic/fwd.h>
#include <util/generic/maybe.h>
#include <util/string/vector.h>

namespace P0fParser {
    struct TP0f {
        ui8 Olen;
        ui8 Version;
        ui8 ObservedTTL;

        TMaybe<ui8> EOL;
        TMaybe<ui8> ITTLDistance;
        TMaybe<ui8> UnknownOptionID;

        TMaybe<ui16> MSS;
        TMaybe<ui16> WSize;
        TMaybe<ui16> Scale;

        bool LayoutNOP = false;
        bool LayoutMSS = false;
        bool LayoutWS = false;
        bool LayoutSOK = false;
        bool LayoutSACK = false;
        bool LayoutTS = false;

        bool QuirksDF = false;
        bool QuirksIDp = false;
        bool QuirksIDn = false;
        bool QuirksECN = false;
        bool Quirks0p = false;
        bool QuirksFlow = false;
        bool QuirksSEQn = false;
        bool QuirksACKp = false;
        bool QuirksACKn = false;
        bool QuirksUPTRp = false;
        bool QuirksURGFp = false;
        bool QuirksPUSHFp = false;
        bool QuirksTS1n = false;
        bool QuirksTS2p = false;
        bool QuirksOPTp = false;
        bool QuirksEXWS = false;
        bool QuirksBad = false;

        bool PClass = false;
    };

    TP0f StringToP0f(const TString& str);
    TP0f StringToP0f(TStringBuf buf);
} // namespace P0fParser
