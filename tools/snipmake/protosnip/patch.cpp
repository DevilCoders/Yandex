#include "patch.h"

#include <kernel/snippets/iface/passagereply.h>
#include <search/idl/meta.pb.h>
#include <kernel/search_daemon_iface/dp.h>

#include <library/cpp/scheme/scheme.h>

#include <util/charset/wide.h>
#include <util/generic/strbuf.h>

namespace NSnippets {

static TString MergeClickLikeSnip(TStringBuf a, TStringBuf b) {
    if (!b) {
        return TString{a};
    }
    NSc::TValue va = NSc::TValue::FromJson(a);
    if (!va.IsDict()) {
        return TString{b};
    }
    NSc::TValue vb = NSc::TValue::FromJson(b);
    return va.MergeUpdate(vb).ToJson(true);
}

static void PatchGta(NMetaProtocol::TDocument& doc, const TStringBuf gta[][2], size_t gtaSize) {
    NMetaProtocol::TArchiveInfo& ar = *doc.MutableArchiveInfo();
    for (size_t i = 0; i < gtaSize; ++i) {
        bool found = false;
        for (size_t j = 0; j < ar.GtaRelatedAttributeSize(); ++j) {
            if (gta[i][0] == ar.MutableGtaRelatedAttribute(j)->GetKey()) {
                found = true;
                if (gta[i][0] == DP_CLICK_LIKE_SNIP) {
                    ar.MutableGtaRelatedAttribute(j)->SetValue(MergeClickLikeSnip(ar.MutableGtaRelatedAttribute(j)->GetValue(), gta[i][1]));
                } else {
                    ar.MutableGtaRelatedAttribute(j)->SetValue(gta[i][1].data(), gta[i][1].size());
                }
            }
        }
        if (!found) {
            NMetaProtocol::TPairBytesBytes* g = ar.AddGtaRelatedAttribute();
            g->SetKey(gta[i][0].data(), gta[i][0].size());
            g->SetValue(gta[i][1].data(), gta[i][1].size());
        }
    }
}

void PatchByPassageReply(NMetaProtocol::TDocument& doc, const TPassageReply& res) {
    NMetaProtocol::TArchiveInfo& ar = *doc.MutableArchiveInfo();
    ar.SetTitle(WideToUTF8(res.GetTitle()));
    ar.SetHeadline(WideToUTF8(res.GetHeadline()));
    ar.ClearPassage();
    ar.ClearPassageAttr();
    for (size_t i = 0; i < res.GetPassages().size(); ++i) {
        ar.AddPassage(WideToUTF8(res.GetPassages()[i]));
    }
    for (size_t i = 0; i < res.GetPassagesAttrs().size(); ++i) {
        ar.AddPassageAttr(res.GetPassagesAttrs()[i]);
    }
    TStringBuf gta[][2] = {
        { "_HeadlineSrc", res.GetHeadlineSrc() },
        { "_SnippetsExplanation", res.GetSnippetsExplanation() },
        { "_SpecSnipAttrs", res.GetSpecSnippetAttrs() },
        { "_ImagesJson", res.GetImagesJson() },
        { "_ImagesDups", res.GetImagesDups() },
        { "sea", res.GetPreviewJson() },
        { DP_ALT_SNIPPETS, res.GetAltSnippets() },
        { "_HilitedUrl", res.GetHilitedUrl() },
        { "_UrlMenu", res.GetUrlMenu() },
        { DP_CLICK_LIKE_SNIP, res.GetClickLikeSnip() },
        { DP_DOCQUERYSIG, res.GetDocQuerySig() },
        { DP_DOCSTATICSIG, res.GetDocStaticSig() },
    };
    PatchGta(doc, gta, Y_ARRAY_SIZE(gta));
}

}
