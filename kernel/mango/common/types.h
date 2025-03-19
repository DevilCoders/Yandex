#pragma once

#include <limits.h>
#include "constraints.h"
#include "protos.h"
#include "types_format.h"
#include <library/cpp/on_disk/2d_array/array2d.h>
#include <util/system/defaults.h>
#include <util/generic/ylimits.h>
#include <util/generic/typetraits.h>
#include <library/cpp/on_disk/head_ar/head_ar.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/stream/str.h>
#include <util/stream/mem.h>
#include <util/ysaveload.h>

typedef ui32 TDocId;
extern const TDocId DOCID_EMPTY;

extern const char *AUTHOR_URL_ATTRIBUTE;
extern const char *SOURCE_URL_ATTRIBUTE;
extern const char *PUBTIME_ATTRIBUTE;
extern const char *QCOUNT_ATTRIBUTE;
extern const char *AUTHORINTEREST_ATTRIBUTE;
extern const char *AUTHORAUTH_ATTRIBUTE;
extern const char *ISDUP_ATTRIBUTE;
extern const char *ISSPAMMER_ATTRIBUTE;
extern const char *ISBIASED_ATTRIBUTE;

namespace NMango
{

struct TFragmentType
{
    enum
    {
        Undefined = 0,
        Attr,
        Lemm,
        Arc,
        XMap,
        Erf,
        LinkAttr,
        LinkLemm,
        LinkArc,
        TimeMachine
    };
};

typedef TMangoLinkInfo TLinkInfo;
typedef TMangoHerfInfo THerfInfo;

class TErfInfo : public TMangoErfInfo
{
public:
    TErfInfo() {}
    inline ui64 UrlId() const {
        return (((ui64)UrlHash1) << 32) + (ui64)UrlHash2;
    }
    inline void SetUrlId(ui64 h) {
        UrlHash2 = (ui32)h;
        UrlHash1 = h >> 32;
    }
};

struct TErfInfoExt : public TErfInfo
{
    char  Host[MAX_HOST_LENGTH];

    TErfInfoExt() {
        Host[0] = 0;
    }
};

struct TLinkInfoExt : public TLinkInfo
{
    TIndexQuote RawLink;

    TLinkInfoExt(const TLinkInfo& link, const TIndexQuote& rawLink)
        : TLinkInfo(link)
        , RawLink(rawLink)
    {
    }
};

typedef FileMapped2DArray<ui32, TLinkInfo> TLinkInfoMatrix;
typedef TArrayWithHead<THerfInfo> THerfInfoArray;
typedef TArrayWithHead<TErfInfo> TErfInfoArray;

typedef FileMapped2DArray<ui32, TErfInfo> TSparsedErfInfoArray;

template <class TErf>
const TErf* GetSparsedErf(const FileMapped2DArray<ui32, TErf>* erfArray, ui32 docId) {
    if (erfArray && docId < erfArray->Size() && erfArray->GetLength(docId) > 0)
        return &erfArray->GetAt(/*pos1=*/docId, /*pos2=*/0); // second dimension is used to make fresh erf optional
    return nullptr;
}

} // NMango

namespace NErf {
    template<>
    class TDynamicReflection<NMango::TErfInfo> : public TDynamicReflection<TMangoErfInfo> {
    };
};

Y_DECLARE_PODTYPE(NMango::TErfInfo);
Y_DECLARE_PODTYPE(NMango::THerfInfo);
Y_DECLARE_PODTYPE(NMango::TErfInfoExt);
Y_DECLARE_PODTYPE(NMango::TLinkInfo);
