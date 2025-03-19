#pragma once

#include <util/generic/string.h>
#include <util/charset/wide.h>

namespace NSegm {
namespace NPrivate {
const ui32 MAX_FOOTER_SCAN = 128;

bool CheckAdsCSSMarker(const char*, ui32);
bool CheckAdsTextMarker(const wchar16*, ui32);

bool CheckFooterCSSMarker(const char*, ui32);
bool CheckFooterTextMarker(const wchar16*, ui32);

bool CheckCommentsCSSMarker(const char*, ui32);
bool CheckCommentsTextMarker(const wchar16*, ui32);

bool CheckMenuCSSMarker(const char*, ui32);
bool CheckHeaderCSSMarker(const char*, ui32);
bool CheckPollCSSMarker(const char*, ui32);

bool CheckUnlikelyCandidateCSS(const char*, ui32);
bool CheckPossibleCandidateCSS(const char*, ui32);
bool CheckPositiveCSS(const char*, ui32);
bool CheckNegativeCSS(const char*, ui32);

bool CheckAdsHrefMarker(const char * text, ui32 len);

struct TBlockMarkers {
    union {
        ui32 All;

        struct {
            ui32 AdsCSS :1;
            ui32 FooterCSS :1;
            ui32 CommentsCSS :1;
            ui32 HeaderCSS :1;
            ui32 MenuCSS :1;
            ui32 PollCSS :1;
            // readability
            ui32 PossibleCandidateCSS :1;
            ui32 UnlikelyCandidateCSS :1;
            ui32 PositiveClass :1;
            ui32 PositiveId :1;
            ui32 NegativeClass :1;
            ui32 NegativeId :1;
        };
    };

public:
    void CheckAttr(const char* text, ui32 len, bool id) {
        AdsCSS |= CheckAdsCSSMarker(text, len);
        FooterCSS |= CheckFooterCSSMarker(text, len);
        HeaderCSS |= CheckHeaderCSSMarker(text, len);
        MenuCSS |= CheckMenuCSSMarker(text, len);
        PollCSS |= CheckPollCSSMarker(text, len);
        CommentsCSS |= CheckCommentsCSSMarker(text, len);

        // readability
        PossibleCandidateCSS |= CheckPossibleCandidateCSS(text, len);
        UnlikelyCandidateCSS |= CheckUnlikelyCandidateCSS(text, len);

        if (id) {
            PositiveId |= CheckPositiveCSS(text, len);
            NegativeId |= CheckNegativeCSS(text, len);
        } else {
            PositiveClass |= CheckPositiveCSS(text, len);
            NegativeClass |= CheckNegativeCSS(text, len);
        }
    }

    TBlockMarkers& operator |=(const TBlockMarkers& markers) {
        All |= markers.All;
        return *this;
    }

    static TBlockMarkers New() {
        TBlockMarkers m;
        m.All = 0;
        return m;
    }
};

struct TLinkMarkers {
    union {
        ui32 Flags;

        struct {
            ui32 AdsHref :1;
            ui32 AdsCSS :1;
            ui32 Implied :1;
        };
    };

public:
    void CheckHref(const char* href, ui32 len) {
        AdsHref |= CheckAdsHrefMarker(href, len);
    }

    void CheckCSS(const char* css, ui32 len) {
        AdsCSS |= CheckAdsCSSMarker(css, len);
    }

    TLinkMarkers& operator |=(const TLinkMarkers& markers) {
        Flags |= markers.Flags;
        return *this;
    }

    static TLinkMarkers New() {
        TLinkMarkers m;
        Zero(m);
        return m;
    }
};

struct TTextMarkers {
    union {
        ui32 Flags;

        struct {
            ui32 AdsText :1;
            ui32 CommentsText :1;
            ui32 FooterText :1;
        };
    };

    // readability
    ui32 SymbolsInText;
    ui32 CommasInText;
    ui32 SpacesInText;
    ui32 AlphasInText;

public:
    void CheckText(const wchar16* text, ui32 textlen) {
        ui32 len = Min(textlen, MAX_FOOTER_SCAN);
        FooterText |= CheckFooterTextMarker(text, len);
        AdsText |= CheckAdsTextMarker(text, len);
        CommentsText |= CheckCommentsTextMarker(text, len);

        if (len > MAX_FOOTER_SCAN)
            FooterText |= CheckFooterTextMarker(text + textlen - MAX_FOOTER_SCAN, Min(textlen - MAX_FOOTER_SCAN, MAX_FOOTER_SCAN));

        if (!textlen)
            return;

        // todo: seems like there is a bug in the following calculations
        ++SymbolsInText;
        for (ui32 i = 1; i < textlen; ++i) {
            if (!IsSpace(text[i-1])) {
                if (IsSpace(text[i]))
                    ++SpacesInText;
                ++SymbolsInText;
            }

            if (IsAlpha(text[i-1])) {
                if (IsTerminal(text[i]))
                    ++CommasInText;
                ++AlphasInText;
            }
        }

        if (textlen)
            AlphasInText += IsAlpha(text[textlen-1]);
    }

    TTextMarkers& operator |=(const TTextMarkers& markers) {
        Flags |= markers.Flags;

        SymbolsInText += markers.SymbolsInText;
        CommasInText += markers.CommasInText;
        SpacesInText += markers.SpacesInText;
        AlphasInText += markers.AlphasInText;

        return *this;
    }

    static TTextMarkers New() {
        TTextMarkers m;
        Zero(m);
        return m;
    }
};
}
}
