#include "passagereply.h"

#include <kernel/search_daemon_iface/dp.h>
#include <search/idl/meta.pb.h>

#include <util/charset/wide.h>
#include <util/generic/buffer.h>
#include <util/stream/buffer.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/string/cast.h>

namespace NSnippets {

const TUtf16String EV_NOTITLE_WIDE = UTF8ToWide(EV_NOTITLE);
const TUtf16String EV_NOHEADLINE_WIDE = UTF8ToWide(EV_NOHEADLINE);

void TPassageReply::SetError(const TString& error) {
    Data = TPassageReplyData();
    ErrorMessage = error;
    HadError = true;
}

void TPassageReply::SetError() {
    SetError("unknown error");
}

void TPassageReply::Set(const TPassageReplyData& data) {
    Data = data;
    HadError = false;
}

// For internal tools
TPassageReply::TPassageReply(const NMetaProtocol::TArchiveInfo& ai)
{
    Data.Title = UTF8ToWide(ai.GetTitle());
    Data.Headline = UTF8ToWide(ai.GetHeadline());
    for (int j = 0; j < ai.passage_size(); ++j) {
        Data.Passages.push_back(UTF8ToWide(ai.GetPassage(j)));
    }
    for (int j = 0; j < ai.passageattr_size();  ++j) {
        Data.Attrs.push_back(ai.GetPassageAttr(j));
    }
    for (int j = 0; j < ai.gtarelatedattribute_size(); ++j) {
        const NMetaProtocol::TPairBytesBytes& gta = ai.GetGtaRelatedAttribute(j);
        const TString& name = gta.GetKey();
        const TString& value = gta.GetValue();
        if (name == DP_SPECSNIPATTRS) {
            Data.SpecSnippetAttrs = value;
        } else if (name == DP_PASSAGESTYPE) {
            Data.LinkSnippet = (value == "1");
        } else if (name == DP_LINKSNIPATTRS) {
            Data.LinkAttrs = value;
        } else if (name == DP_HEADLINE_SRC) {
            Data.HeadlineSrc = value;
        } else if (name == DP_IMGDUPSATTRS) {
            Data.ImgDupsAttrs = value;
        } else if (name == DP_SNIPPETS_EXPLANATION) {
            Data.SnippetsExplanation = value;
        } else if (name == DP_DOCQUERYSIG) {
            Data.DocQuerySig = value;
        } else if (name == DP_DOCSTATICSIG) {
            Data.DocStaticSig = value;
        } else if (name == DP_ENTITYCLASSIFY) {
            Data.EntityClassifyResult = value;
        } else if (name == DP_IMAGESJSON) {
            Data.ImagesJson = value;
        } else if (name == DP_IMAGES_DUPS) {
            Data.ImagesDups = value;
        } else if (name == DP_MARKERS) {
            Data.Markers.push_back(value);
        } else if (name == DP_SEA) {
            Data.PreviewJson = value;
        } else if (name == DP_SCHEMA_VTHUMB) {
            Data.SchemaVthumb = value;
        } else if (name == DP_CLICK_LIKE_SNIP) {
            Data.ClickLikeSnip = value;
        }
    }
}

void AddGta(NMetaProtocol::TArchiveInfo& ai, const TString& gtaName, const TString& gtaValue)
{
    NMetaProtocol::TPairBytesBytes* gta = ai.AddGtaRelatedAttribute();
    gta->SetKey(gtaName);
    gta->SetValue(gtaValue);
}

void TPassageReply::PackToArchiveInfo(NMetaProtocol::TArchiveInfo& ai) const {
    ai.SetTitle(WideToUTF8(GetTitle()));
    ai.SetHeadline(WideToUTF8(GetHeadline()));
    const TVector<TUtf16String>& passages = GetPassages();
    for (const TUtf16String& passage : passages) {
        ai.AddPassage(WideToUTF8(passage));
    }
    for (const TString& attr : GetPassagesAttrs()) {
        ai.AddPassageAttr(attr);
    }
    AddGta(ai, DP_SPECSNIPATTRS, GetSpecSnippetAttrs());
    AddGta(ai, DP_PASSAGESTYPE, ToString<int>(GetPassagesType()));
    AddGta(ai, DP_LINKSNIPATTRS, GetLinkAttrs());
    AddGta(ai, DP_HEADLINE_SRC, GetHeadlineSrc());
    AddGta(ai, DP_IMGDUPSATTRS, GetImgDupsAttrs());
    AddGta(ai, DP_SNIPPETS_EXPLANATION, GetSnippetsExplanation());
    AddGta(ai, DP_DOCQUERYSIG, GetDocQuerySig());
    AddGta(ai, DP_DOCSTATICSIG, GetDocStaticSig());
    AddGta(ai, DP_ENTITYCLASSIFY, GetEntityClassifyResult());
    AddGta(ai, DP_IMAGESJSON, GetImagesJson());
    AddGta(ai, DP_IMAGES_DUPS, GetImagesDups());
    AddGta(ai, DP_ALT_SNIPPETS, GetAltSnippets());
    AddGta(ai, DP_SEA, GetPreviewJson());
    AddGta(ai, DP_SCHEMA_VTHUMB, GetSchemaVthumb());
    AddGta(ai, DP_HILITED_URL, GetHilitedUrl());
    AddGta(ai, DP_URLMENU, GetUrlMenu());
    AddGta(ai, DP_CLICK_LIKE_SNIP, GetClickLikeSnip());
    for (size_t i = 0; i < GetMarkersCount(); ++i) {
        AddGta(ai, DP_MARKERS, GetMarker(i));
    }
}

void TPassageReply::PackAltSnippets(NSnippets::NProto::TSecondarySnippets& ss)
{
    TBuffer unencoded(64*1024);
    TBufferOutput bufOut(unencoded);
    ss.SerializeToArcadiaStream(&bufOut);
    Base64Encode(TStringBuf(unencoded.data(), unencoded.size()), Data.AltSnippetsPacked);
}

}
