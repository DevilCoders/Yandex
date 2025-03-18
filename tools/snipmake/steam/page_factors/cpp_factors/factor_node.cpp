#include "factor_node.h"

#include <util/generic/is_in.h>
#include <util/generic/string.h>
#include <util/generic/map.h>
#include <util/string/strip.h>
#include <util/system/defaults.h>

namespace NSegmentator {

static const TString DATA_GUID = "data-guid";

static const TMap<ui32, TString> ANNOTATE_CODES_2_TYPE = {
    {0, "AUX"},
    {1, "AAD"},
    {2, "ACP"},
    {3, "AIN"},
    {4, "ASL"},

    {10, "DMD"},
    {11, "DHC"},
    {12, "DHA"},
    {13, "DCT"},
    {14, "DCM"},

    {20, "LMN"},
    {21, "LCN"},
    {22, "LIN"}
};

const TString& GetSegType(ui32 segCode) {
    const TString* segType = ANNOTATE_CODES_2_TYPE.FindPtr(segCode);
    Y_ASSERT(segType != nullptr);
    return *segType;
}


bool IsInlineTag(const NHtml::TTag& tag) {
    return false == tag.is(HT_br);
}


const HT_TAG MEDIA_TAGS[] = {
    HT_IMG, HT_AUDIO, HT_VIDEO, HT_OBJECT, HT_EMBED, HT_APPLET,
};


bool IsMediaTag(const NHtml::TTag& tag) {
    return IsIn(MEDIA_TAGS, std::end(MEDIA_TAGS), tag.id());
}


// TFactorNode
bool TFactorNode::IsEmpty() {
    if (Empty.IsActive()) {
        return Empty.Get();
    }

    if (Node->IsText()) {
        TWtringBuf text = Node->Text();
        return Empty.Set(StripString(text).empty());
    }
    const NHtml::TTag& tag = NHtml::FindTag(Node->Tag());
    if (IsMediaTag(tag)) {
        return Empty.Set(false);
    }
    TFactorNode* child = FirstChild;
    while (nullptr != child) {
        if (!child->IsEmpty()) {
            return Empty.Set(false);
        }
        child = child->Next;
    }
    return Empty.Set(true);
}

TStringBuf TFactorNode::GetGuid() const {
    return Node->Attr(DATA_GUID);
}

}  // NSegmentator
