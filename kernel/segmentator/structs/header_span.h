#pragma once

#include "spans.h"

namespace NSegm {

enum EHeaderSpanBytes {
    HeaderSpanBytes_0 = 0,
    HeaderSpanBytes_1 = 21,
    CurrentHeaderSpanBytes = HeaderSpanBytes_1,
};

#pragma pack(push, 1)
struct THeaderSpan: public TSpan {
    static const ui32 NBytes = CurrentHeaderSpanBytes;

    union {
        char Bytes[NBytes];

        struct {
            ui8 Words; //1
            ui8 LinkWords; //1
            ui8 Links; //1
            ui8 LocalLinks; //1
            ui8 Domains; //1
            ui8 FirstTag; //1
            ui8 LastTag; //1
            ui8 FrontAbsDepth; //1
            ui8 BackAbsDepth; //1

            ui8 TitleWords; //1
            ui8 FooterWords; //1

            union {
                ui16 AllMarkers;
                struct {
                    ui16 AdsCSS :1;
                    ui16 AdsHeader :1;
                    ui16 PollCSS :1;
                    ui16 MenuCSS :1;
                    ui16 CommentsCSS :1;
                    ui16 CommentsHeader :1;
                    ui16 AdsHref :1;
                    ui16 InArticle :1;
                    ui16 InReadabilitySpans :1;
                    ui16 InMainContentNews :1;
                    ui16 InMainHeadersNews :1;
                };
            }; //2

            ui16 CommasInText; //2
            ui16 SpacesInText; //2
            ui16 AlphasInText; //2
            ui16 SymbolsInText; //2
        };
    };

    ui16 Number;
    ui16 Total;
    bool IsStrictHeader;

    float Weight;

    THeaderSpan()
        : Number()
        , Total()
        , IsStrictHeader(false)
        , Weight()
    { Zero(Bytes); }

    THeaderSpan(TPosting b, TPosting e)
        : TSpan(b, e)
        , Number()
        , Total()
        , IsStrictHeader()
        , Weight()
    { Zero(Bytes); }
};
#pragma pack(pop)

using THeaderSpans = TVector<THeaderSpan>;
using TMainHeaderSpan = THeaderSpan;
using TMainHeaderSpans = TVector<TMainHeaderSpan>;

}
