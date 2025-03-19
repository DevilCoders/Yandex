#pragma once

#include "header_span.h"
#include "segment_span.h"

namespace NSegm {

struct TDocContext {
    ui32 TitleWords = 0;
    ui32 FooterWords = 0;
    ui32 MetaDescrWords = 0;
    ui32 Words = 0;
    ui32 MaxWords = 0;
    ui32 GoodSegs = 0;
};

typedef TSpan TParagraphSpan;
typedef TVector<TSpan> TParagraphSpans;

typedef TSpan TArticleSpan;
typedef TVector<TSpan> TArticleSpans;

}
