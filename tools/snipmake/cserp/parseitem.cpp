#include "parseitem.h"
#include "parsejson.h"

#include <tools/snipmake/cserp/idl/serpitem.pb.h>

#include <google/protobuf/text_format.h>
#include <google/protobuf/messagext.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

namespace NSnippets {
    static void TryParseItemText(const TSerpNode& n, NProto::TSerpItem& res) {
        if (!n.HasClass("serp-item__text") && !n.HasClass("serp-item__list-item")) {
            return;
        }
        if (n.InnerText.size()) {
            res.AddItemText(n.InnerText);
        }
    }
    static void TryParseMetaText(const TSerpNode& n, NProto::TSerpItem& res) {
        if (!n.HasClass("serp-meta__item")) {
            return;
        }
        if (n.InnerText.size()) {
            res.AddMetaText(n.InnerText);
        }
    }
    static void TryParseTitle(const TSerpNode& n, NProto::TSerpItem& res) {
        if (!n.HasClass("serp-item__title")) {
            return;
        }
        res.SetTitleText(n.InnerText);
    }
    static void TryParseUrl(const TSerpNode& n, NProto::TSerpItem& res) {
        if (!n.HasClass("serp-item__title-link")) {
            return;
        }
        res.SetUrl(n.Href);
    }
    static void TryParseUrlLink(const TSerpNode& n, NProto::TSerpItem& res) {
        if (!n.HasClass("serp-url__link")) {
            return;
        }
        NProto::TUrlLink& lnk = *res.AddUrlLinks();
        lnk.SetText(n.InnerText);
        lnk.SetUrl(n.Href);
    }
    static void TryParseSitelink(const TSerpNode& n, NProto::TSerpItem& res) {
        if (!n.HasClass("serp-sitelinks__link")) {
            return;
        }
        NProto::TUrlLink& lnk = *res.AddSitelinks();
        lnk.SetText(n.InnerText);
        lnk.SetUrl(n.Href);
    }
    void TryGetQuery(const TSerpNode& n, TString& res) {
        if (n.Name == "text" && n.HasClass("input__control")) {
            res = n.Value;
        }
    }
    TString FindQuery(const TSerpNodePtr& v) {
        TString s;
        if (!v.Get()) {
            return s;
        }
        TryGetQuery(*v, s);
        if (s.size()) {
            return s;
        }
        for (const auto& i : v->Children) {
            s = FindQuery(i);
            if (s.size()) {
                return s;
            }
        }
        return TString();
    }
    bool IsNumFound(const TSerpNode& n) {
        if (n.HasClass("input__found")) {
            return true;
        }
        return false;
    }
    TSerpNodePtr FindNumFound(const TSerpNodePtr& v) {
        if (!v.Get()) {
            return TSerpNodePtr();
        }
        if (IsNumFound(*v)) {
            return v;
        }
        for (const auto& i : v->Children) {
            TSerpNodePtr res = FindNumFound(i);
            if (res) {
                return res;
            }
        }
        return TSerpNodePtr();
    }
    static void RecursiveParse(const TSerpNode& n, NProto::TSerpItem& res) {
        for (const auto& i : n.Children) {
                TryParseItemText(*i, res);
                TryParseMetaText(*i, res);
                TryParseUrlLink(*i, res);
                TryParseSitelink(*i, res);
                RecursiveParse(*i, res);
                TryParseTitle(*i, res);
                TryParseUrl(*i, res);
        }
    }
    bool ParseSerpItem(const TSerpNode& n, NProto::TSerpItem& res) {
        if (!n.HasClass("serp-item")) {
            return false;
        }
        if (n.HasClass("serp-adv__item")) {
            res.SetIsAdvItem(true);
        }
        res.SetInnerText(n.InnerText);
        NProto::TBounds& b = *res.MutableBounds();
        b.SetTop(n.Bounds.Top);
        b.SetLeft(n.Bounds.Left);
        b.SetWidth(n.Bounds.Width);
        b.SetHeight(n.Bounds.Height);
        b.SetRight(n.Bounds.Right);
        b.SetBottom(n.Bounds.Bottom);
        RecursiveParse(n, res);
        return true;
    }
    void DumpSerpItem(const NProto::TSerpItem& s) {
        TString res;
        google::protobuf::io::StringOutputStream out(&res);
        google::protobuf::TextFormat::Printer p;
        p.SetUseUtf8StringEscaping(true);
        p.Print(s, &out);
        Cout << res << Endl;
    }
}
