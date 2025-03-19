#include "segmentator.h"

namespace NSegm {
namespace NPrivate {
const ui32 MAX_ATTR_SCAN = 128;

void TSegmentator::FlushAttributes(const TNumerStat& stat) {
    AttributeExpectation = ExpectAttrNone;

    if (!Ctx->ImgCtx.Empty()) {
        ui32 hosthash = 0;
        ELinkType lt = Ctx->OwnerInfo.CheckLink(Ctx->ImgCtx.Src, hosthash);
        Storer->OnImage(Ctx->ImgCtx.Src, Ctx->ImgCtx.Alt, lt, stat.TokenPos.Pos);
        Ctx->ImgCtx.Clear();
    }
}

void TSegmentator::ProcessAttribute(const TAttrEntry attr, const TNumerStat& stat) {
    ETagAttributeType type = GetAttributeType(~attr.Name);
    ui32 len = +attr.Value;
    len = Min(len, MAX_ATTR_SCAN);

    switch (AttributeExpectation) {
    default:
        break;
    case ExpectAttrBlock:
        ProcessBlockAttribute(type, ~attr.Value, len, stat);
        break;
    case ExpectAttrInlineBlock:
        ProcessInlineBlockAttribute(type, ~attr.Value, len, stat);
        break;
    case ExpectAttrSpan:
        ProcessSpanAttribute(type, ~attr.Value, len, stat);
        break;
    case ExpectAttrAncor:
        ProcessAncorAttribute(type, ~attr.Value, len, stat);
        [[fallthrough]];
    case ExpectAttrArea:
        //we do not even expect ancors/areas in noindex
        if (ATTR_URL == attr.Type || ATTR_NOFOLLOW_URL == attr.Type || ATTR_UNICODE_URL == attr.Type) {
            ProcessLinkAttribute(type, ~attr.Value, +attr.Value, stat);
        }
        break;
    case ExpectAttrImg:
        if (ATTR_URL == attr.Type || ATTR_NOFOLLOW_URL == attr.Type || ATTR_UNICODE_URL == attr.Type) {
            ProcessImgAttribute(type, ~attr.Value, +attr.Value, stat);
        } else {
            ProcessImgAttribute(type, ~attr.Value, len, stat);
        }
        break;
    }
}

void TSegmentator::ProcessImgAttribute(ETagAttributeType type, const char* text, ui32 len, const TNumerStat&) {
    if (TAT_IMAGE == type)
        Ctx->ImgCtx.Src.assign(text, len);
    else if (TAT_HINT == type)
        Ctx->ImgCtx.Alt.assign(text, len);
}

void TSegmentator::ProcessSpanAttribute(ETagAttributeType type, const char* text, ui32 len, const TNumerStat&) {
    if ((TAT_ID == type || TAT_CLASS == type) && CheckHeaderCSSMarker(text, len))
        BoldDistance.OpenBoldSpan(SpanCount);
}

void TSegmentator::ProcessAncorAttribute(ETagAttributeType type, const char* text, ui32 len, const TNumerStat&) {
    if ((TAT_ID == type || TAT_CLASS == type)) {
        if (CheckHeaderCSSMarker(text, len))
            BoldDistance.OpenAncor();
        LinkMarkers.CheckCSS(text, len);
    }
}

void TSegmentator::ProcessInlineBlockAttribute(ETagAttributeType type, const char * text, ui32 len, const TNumerStat&) {
    if ((TAT_ID == type || TAT_CLASS == type) && CheckHeaderCSSMarker(text, len))
        BoldDistance.OpenBoldBlock(Ctx->BlockStack.back().Tag);
}

void TSegmentator::ProcessLinkAttribute(ETagAttributeType type, const char * text, ui32 len, const TNumerStat& stat) {
    if (TAT_LINKEXT != type && TAT_LINKINT != type)
        return;

    TString url(text, len);

    if (TAT_LINKEXT == type)
        LinkMarkers.CheckHref(text, len);

    if (!IsA<DNT_LINK>(CurrentNode) || !CurrentNode->ListEmpty())
        OpenLinkNode(stat, url);

    if (ExpectAttrArea == AttributeExpectation)
        CloseLinkNode(stat);
}

void TSegmentator::ProcessBlockAttribute(ETagAttributeType type, const char * text, ui32 len, const TNumerStat&) {
    Y_VERIFY(IsA<DNT_BLOCK>(CurrentNode), " ");

    if (TAT_WIDTH == type && ExpectAttrBlock == AttributeExpectation) {
        CurrentNode->Props.Width = MurmurHash<ui32>(text, len);
    } else if (TAT_CLASS == type || TAT_ID == type) {
        CurrentNode->Props.BlockMarkers.CheckAttr(text, len, TAT_ID == type);

        if (TAT_CLASS == type)
            CurrentNode->Props.Class = MurmurHash<ui32>(text, len);
    }
}
}
}
