#pragma once

#include <kernel/segnumerator/utils/url_functions.h>

#include <library/cpp/html/spec/tags.h>
#include <library/cpp/wordpos/wordpos.h>

namespace NSegm {
namespace NPrivate {
enum ETagBreakLevel {
    TBL_INLINE = 0, TBL_BR, TBL_BRBR, TBL_LI, TBL_TR, TBL_PP
};

enum ETagAttributeType {
    TAT_NONE = 0, TAT_CLASS, TAT_ID, TAT_WIDTH, TAT_LINKINT, TAT_LINKEXT, TAT_HINT, TAT_IMAGE
};

inline ETagAttributeType GetAttributeType(const char* name) {
    if (!strcmp("_s_class", name))
        return TAT_CLASS;
    if (!strcmp("_s_id", name))
        return TAT_ID;
    if (!strcmp("_s_width", name))
        return TAT_WIDTH;
    if (!strcmp("linkint", name))
        return TAT_LINKINT;
    if (!strcmp("link", name))
        return TAT_LINKEXT;
    if (!strcmp("image", name))
        return TAT_IMAGE;
    if (!strcmp("hint", name))
        return TAT_HINT;
    return TAT_NONE;
}

inline bool IsStructTag(HT_TAG t) {
    switch (t) {
    default:
        return false;
    case HT_ADDRESS:
    case HT_BLOCKQUOTE:
    case HT_PRE:

    case HT_P:
    case HT_H1:
    case HT_H2:
    case HT_H3:
    case HT_H4:
    case HT_H5:
    case HT_H6:

    case HT_BLINK:
    case HT_MARQUEE:
    case HT_CENTER:
    case HT_DIV:

    case HT_TABLE:
    case HT_CAPTION:
    case HT_TR:
    case HT_TD:
    case HT_TH:

    case HT_DL:
    case HT_DT:
    case HT_DD:

    case HT_DIR:
    case HT_MENU:
    case HT_OL:
    case HT_UL:
    case HT_LI:

    case HT_FORM:
    case HT_FIELDSET:
        return true;
    }
}

inline bool IsInputTag(HT_TAG t) {
    switch (t) {
    default:
        return false;
    case HT_INPUT:
    case HT_SELECT:
    case HT_TEXTAREA:
    case HT_BUTTON:
        return true;
    }
}

inline bool IsListRootTag(HT_TAG tag) {
    switch (tag) {
    default:
        return false;
    case HT_UL:
    case HT_OL:
    case HT_DL:
    case HT_MENU:
    case HT_DIR:
        return true;
    }
}

inline bool IsListLeafTag(HT_TAG tag) {
    switch (tag) {
    default:
        return false;
    case HT_LI:
    case HT_DT:
    case HT_DD:
        return true;
    }
}

inline bool IsTableLeafTag(HT_TAG tag) {
    return HT_TD == tag || HT_TH == tag;
}

inline bool IsListTag(HT_TAG tag) {
    return IsListLeafTag(tag) || IsListRootTag(tag);
}

inline bool IsTableTag(HT_TAG tag) {
    return HT_TABLE == tag || HT_TR == tag || IsTableLeafTag(tag);
}

inline bool IsHxTag(HT_TAG tag) {
    return HT_H1 <= tag && tag <= HT_H6;
}

inline bool IsHeaderTag(HT_TAG tag) {
    return IsHxTag(tag) || HT_CAPTION == tag || HT_TH == tag;
}

inline ETagBreakLevel GetBreakLevel(HT_TAG t) {
    switch (t) {
    default:
        return TBL_INLINE;
    case HT_ADDRESS:
    case HT_P:
    case HT_PRE:
    case HT_BLINK:
    case HT_MARQUEE:
    case HT_BLOCKQUOTE:
    case HT_CENTER:
    case HT_DIV:
    case HT_FIELDSET:
    case HT_TABLE:
    case HT_FORM:
    case HT_H1:
    case HT_H2:
    case HT_H3:
    case HT_H4:
    case HT_H5:
    case HT_H6:
    case HT_DL:
    case HT_DIR:
    case HT_MENU:
    case HT_OL:
    case HT_UL:
        return TBL_PP;
    case HT_CAPTION:
    case HT_TR:
        return TBL_TR;
    case HT_DD:
    case HT_DT:
    case HT_LI:
        return TBL_LI;
    case HT_BR:
        return TBL_BR;
    case HT_HR:
        return TBL_BRBR;
    }
}

inline bool IsBreakTag(HT_TAG t) {
    return GetBreakLevel(t) != TBL_INLINE;
}

inline ETagBreakLevel MergeBreaks(ETagBreakLevel one, ETagBreakLevel two) {
    return TBL_INLINE == one ? two : TBL_INLINE == two ? one : std::max(std::max(one, two),
            TBL_BRBR);
}

inline ETagBreakLevel MergeBreaks(HT_TAG one, HT_TAG two) {
    return MergeBreaks(GetBreakLevel(one), GetBreakLevel(two));
}

struct TBoldDistance {
    ui16 A;
    ui16 B;
    ui16 Big;
    ui16 Strong;
    ui16 Span;
    ui16 Block;
    ui16 SpanCount;
    HT_TAG BlockBold;

public:
    static TBoldDistance New() {
        TBoldDistance bd;
        Zero(bd);
        return bd;
    }

    friend bool operator ==(const TBoldDistance& a, const TBoldDistance& b) {
        return (a.A == b.A) | (a.B == b.B) | (a.Big == b.Big) | (a.Strong == b.Strong)
                        | (a.Span == b.Span) | (a.Block == b.Block)
                        | (a.SpanCount == b.SpanCount) | (a.BlockBold == b.BlockBold);
    }

    friend bool operator !=(const TBoldDistance& a, const TBoldDistance& b) {
        return !(a == b);
    }

    TString ToString() const {
        TStringStream s;
        s << "[A:" << A << ",B:" << B << ",Big:" << Big << ",Strong:" << Strong
                        << ",Span:" << Span << ",Block:" << Block << ",SpanCount:" << SpanCount
                        << ",BlockBold:" << NHtml::FindTag(BlockBold).name << "]";
        return s.Str();
    }

    bool CanBeHeader() const {
        return IsBold() && A < 2 && B < 2 && Big < 2 && Strong < 2 && Span < 2 && Block < 2;
    }

    bool IsBold() const {
        return A | B | Big | Strong | Span | Block;
    }

    bool CanBeHeader(const TBoldDistance& next) {
        if (!CanBeHeader())
            return false;

        if (!next.IsBold())
            return true;

        return (A == 1 && next.A < 2) || (B == 1 && next.B < 2) || (Big == 1 && next.Big < 2)
                        || (Strong == 1 && next.Strong < 2) || (Span == 1 && next.Span < 2)
                        || (Block == 1 && next.Block < 2);
    }

    void Move(bool close = false) {
        if (A)
            A++;
        if (B)
            B++;
        if (Big)
            Big++;
        if (Strong)
            Strong++;
        if (Span)
            Span++;
        if (Block && !close)
            Block++;
    }

    void RenewIrregular(const HT_TAG tag, bool close) {
        switch (tag) {
        default:
            return;
        case HT_B:
            B = close ? 0 : 1;
            return;
        case HT_BIG:
            Big = close ? 0 : 1;
            return;
        case HT_STRONG:
            Strong = close ? 0 : 1;
            return;
        }
    }

    void OpenAncor() {
        A = 1;
    }
    void CloseAncor() {
        A = 0;
    }

    void RenewBlock(HT_TAG tag, bool close) {
        if (tag == BlockBold && close) {
            BlockBold = HT_any;
            Block = 0;
        }
    }

    void RenewSpan(const ui16 spanCount, bool close) {
        if (spanCount == SpanCount && close)
            Span = 0;
    }

    void OpenBoldSpan(int spanCount) {
        Span = 1;
        SpanCount = (ui16)spanCount;
    }

    void OpenBoldBlock(HT_TAG tag) {
        Block = 1;
        BlockBold = tag;
    }
};
}
}
