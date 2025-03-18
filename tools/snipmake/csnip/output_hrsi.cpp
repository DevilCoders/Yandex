#include "output_hrsi.h"

#include <tools/snipmake/cserp/idl/serpitem.pb.h>

#include <kernel/snippets/urlmenu/dump/dump.h>

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/charset/wide.h>
#include <util/stream/output.h>
#include <library/cpp/string_utils/url/url.h>

namespace NSnippets {

    static TUtf16String DropHighlight(const TUtf16String& s) {
        TUtf16String res;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '\x07') {
                ++i;
                continue;
            }
            res += s[i];
        }
        return res;
    }

    static void AddUrlLink(NProto::TSerpItem& x, TStringBuf text, TStringBuf url) {
        NProto::TUrlLink& u = *x.AddUrlLinks();
        u.SetText(TString(text.data(), text.size()));
        u.SetUrl(TString(url.data(), url.size()));
    }

    static void AddUrlLinks(NProto::TSerpItem& x, const TString& url) {
        TStringBuf u = CutHttpPrefix(url);
        if (u.StartsWith("www.")) {
            u.Skip(4);
        }
        TStringBuf domain;
        u.NextTok('/', domain);
        if (!domain.size()) {
            return;
        }
        if (!u.size()) {
            AddUrlLink(x, domain, url);
        } else {
            AddUrlLink(x, domain, TStringBuf(url.data(), domain.data() + domain.size() + 1));
            AddUrlLink(x, u, url);
        }
    }

    static void AddUrlMenuLinks(NProto::TSerpItem& x, const TString& urlmenu) {
        TUrlMenuVector v;
        NUrlMenu::Deserialize(v, urlmenu);
        for (const auto& i : v) {
            NProto::TUrlLink& u = *x.AddUrlLinks();
            u.SetText(WideToUTF8(DropHighlight(i.second)));
            u.SetUrl(AddSchemePrefix(WideToUTF8(i.first)));
        }
    }

    void THrSerpItemOutput::Process(const TJob& job)
    {
        NProto::TSerpItem x;

        x.SetUrl(AddSchemePrefix(job.ArcUrl));
        x.SetTitleText(WideToUTF8(DropHighlight(job.Reply.GetTitle())));
        if (job.Reply.GetUrlMenu().size()) {
            AddUrlMenuLinks(x, job.Reply.GetUrlMenu());
        } else {
            AddUrlLinks(x, AddSchemePrefix(job.ArcUrl));
        }
        if (job.Reply.GetHeadline().size()) {
            x.AddItemText(WideToUTF8(DropHighlight(job.Reply.GetHeadline())));
        }
        TUtf16String snp;
        for (size_t i = 0; i < job.Reply.GetPassages().size(); ++i) {
            if (i) {
                snp += u" ... ";
            }
            snp += DropHighlight(job.Reply.GetPassages()[i]);
        }
        if (snp.size()) {
            x.AddItemText(WideToUTF8(snp));
        }

        TString res;
        google::protobuf::io::StringOutputStream out(&res);
        google::protobuf::TextFormat::Printer p;
        p.SetUseUtf8StringEscaping(true);
        p.Print(x, &out);
        Cout << res << Endl;
    }

} //namespace NSnippets
