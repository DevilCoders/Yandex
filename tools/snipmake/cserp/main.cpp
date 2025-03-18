#include "parsejson.h"
#include "parseitem.h"
#include "img.h"

#include <tools/snipmake/cserp/idl/serpitem.pb.h>
#include <tools/snipmake/argv/opt.h>
#include <tools/snipmake/snipdat/metahost.h>

#include <library/cpp/svnversion/svnversion.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_reader.h>

#include <util/stream/file.h>
#include <library/cpp/cgiparam/cgiparam.h>

static bool Verbose = false;

void GetItems(const NSnippets::TSerpNode& v, TVector<NSnippets::NProto::TSerpItem>& res) {
    NSnippets::NProto::TSerpItem s;
    if (NSnippets::ParseSerpItem(v, s)) {
        res.push_back(s);
    }
    for (const auto& i : v.Children) {
       GetItems(*i, res);
    }
}
void NormalizeCgi(TStringBuf hostport, TCgiParameters& cgi) {
    if (hostport == "maps.yandex.ru") {
        cgi.EraseAll("sctx"); //is it ok to erase?
    }
}
TString GetUrl(const NSnippets::NProto::TSerpItem& a) {
    if (!a.HasUrl()) {
        return TString();
    }
    return a.GetUrl();
}
TString GetInnerText(const NSnippets::NProto::TSerpItem& a) {
    if (!a.HasInnerText()) {
        return TString();
    }
    return a.GetInnerText();
}
bool UrlsDiffer(const TString& a, const TString& b) {
    if (a == b) {
        return false;
    }
    TStringBuf ascheme;
    TStringBuf ahostport;
    TStringBuf apath;
    TStringBuf acgi;
    const bool aparse = NSnippets::ParseUrl(a, ascheme, ahostport, apath, acgi);
    TStringBuf bscheme;
    TStringBuf bhostport;
    TStringBuf bpath;
    TStringBuf bcgi;
    const bool bparse = NSnippets::ParseUrl(b, bscheme, bhostport, bpath, bcgi);
    if (!aparse || !bparse || ascheme != bscheme || ahostport != bhostport || apath != bpath) {
        return true;
    }
    TCgiParameters ap(acgi);
    TCgiParameters bp(bcgi);
    NormalizeCgi(ahostport, ap);
    NormalizeCgi(bhostport, bp);
    if (ap != bp) {
        return true;
    }
    return false;
}
bool UrlsDiffer(const NSnippets::NProto::TSerpItem& a, const NSnippets::NProto::TSerpItem& b) {
    return UrlsDiffer(GetUrl(a), GetUrl(b));
}
bool NumFoundDiffer(const NSnippets::TSerpNodePtr& a, const NSnippets::TSerpNodePtr& b) {
    if (!!a != !!b) {
        return true;
    }
    if (!a) {
        return false;
    }
    return a->InnerText != b->InnerText;
}
bool OnlyNumFoundDiffer(const NSnippets::TSerp& a, const NSnippets::TSerp& b) {
    if (!a.NumFound || !b.NumFound) {
        return false;
    }
    if (a.NumFound->Bounds != b.NumFound->Bounds) {
        return false;
    }
    if (!a.Image || !b.Image) {
        return false;
    }
    if (a.Image->GetW() != b.Image->GetW() || a.Image->GetH() != b.Image->GetH()) {
        return false;
    }
    for (size_t t = 0; t < 4; ++t) {
        size_t x, y, w, h;
        if (t == 0) {
            x = 0;
            y = 0;
            w = a.Image->GetW();
            h = a.NumFound->Bounds.Top;
        } else if (t == 1) {
            x = 0;
            y = a.NumFound->Bounds.Top + a.NumFound->Bounds.Height;
            w = a.Image->GetW();
            h = a.Image->GetH() - y;
        } else if (t == 2) {
            x = 0;
            y = a.NumFound->Bounds.Top;
            w = a.NumFound->Bounds.Left;
            h = a.NumFound->Bounds.Height;
        } else {
            x = a.NumFound->Bounds.Left + a.NumFound->Bounds.Width;
            y = a.NumFound->Bounds.Top;
            w = a.Image->GetW() - x;
            h = a.NumFound->Bounds.Height;
        }
        if (a.Image->GetSubHash(x, y, w, h) != b.Image->GetSubHash(x, y, w, h)) {
            return false;
        }
    }
    return true;
}
bool IsAdv(const NSnippets::NProto::TSerpItem& a) {
    return a.HasIsAdvItem() && a.GetIsAdvItem();
}

enum ECompareResult {
    NOT_EVEN_TRIED = 0,
    DIFFERENT_QUERY,
    SAME_IMAGE,
    NUM_FOUND_DIFF,
    NON_TEXT_DIFF,
    DIFFERENT_RANKING,
    TEXT_DIFF,
};

struct TCompareResult {
    ECompareResult Value;
    TString Message;

    TCompareResult(ECompareResult value, const TString& message)
      : Value(value)
      , Message(message)
    {
    }
};

TCompareResult Do(const NSnippets::TSerp& a, const NSnippets::TSerp& b) {
    if (a.Query != b.Query) {
        return TCompareResult(DIFFERENT_QUERY, "Different query: '" + a.Query + "' vs '" + b.Query + "'");
    }
    TVector<NSnippets::NProto::TSerpItem> ua;
    TVector<NSnippets::NProto::TSerpItem> ub;
    GetItems(*a.Layout.Get(), ua);
    GetItems(*b.Layout.Get(), ub);
    if (Verbose) {
        Cout << "Left items:" << Endl;
        for (const auto& x : ua) {
            NSnippets::DumpSerpItem(x);
        }
        Cout << "Right items:" << Endl;
        for (const auto& x : ub) {
            NSnippets::DumpSerpItem(x);
        }
    }
    bool textDiff = false;
    for (size_t i = 0; i < ua.size() || i < ub.size(); ++i) {
        if (i >= ua.size()) {
            return TCompareResult(DIFFERENT_RANKING, "Different ranking: same " + ToString(i) + ", but " + ToString(ub.size() - ua.size())  + " more results on the right");
        }
        if (i >= ub.size()) {
            return TCompareResult(DIFFERENT_RANKING, "Different ranking: same " + ToString(i) + ", but " + ToString(ua.size() - ub.size())  + " more results on the left");
        }
        if (UrlsDiffer(ua[i], ub[i])) {
            if (IsAdv(ua[i]) && IsAdv(ub[i])) {
                continue;
            }
            return TCompareResult(DIFFERENT_RANKING, "Different ranking: pos " + ToString(i + 1) + ": " + GetUrl(ua[i]) + " vs " + GetUrl(ub[i]));
        }
        if (GetInnerText(ua[i]) != GetInnerText(ub[i])) {
            textDiff = true;
        }
    }
    if (a.PngImage == b.PngImage) {
        return TCompareResult(SAME_IMAGE, "No diff in screenshots");
    }
    if (!textDiff) {
        if (NumFoundDiffer(a.NumFound, b.NumFound) && OnlyNumFoundDiffer(a, b)) {
            return TCompareResult(NUM_FOUND_DIFF, "Num-found diff");
        } else {
            return TCompareResult(NON_TEXT_DIFF, "Non-text diff?");
        }
    }
    return TCompareResult(TEXT_DIFF, "Ok, looks like a real diff");
}

struct TJob {
    TString IFile;
    NJson::TJsonValue Json;
    NSnippets::TSerp Serp;

    explicit TJob(const TString& ifile)
      : IFile(ifile)
    {
    }
};

int main(int argc, char** argv) {
    using namespace NLastGetopt;
    TOpts opt;
    TOpt& v = opt.AddLongOption("version").HasArg(NO_ARGUMENT);

    TOpt& verb = opt.AddCharOption('v', NO_ARGUMENT, "verbose output");
    TOpt& l = opt.AddCharOption('l', REQUIRED_ARGUMENT, "left json file");
    TOpt& r = opt.AddCharOption('r', REQUIRED_ARGUMENT, "right json file");

    opt.SetFreeArgsNum(0);

    TOptsParseResult o(&opt, argc, argv);

    if (Has_(opt, o, &v)) {
        Cout << GetProgramSvnVersion() << Endl;
        return 0;
    }

    if (Has_(opt, o, &verb)) {
        Verbose = true;
    }

    if (!CountMulti_(opt, o, &l) || !CountMulti_(opt, o, &r)) {
        Cerr << "need both -l and -r files" << Endl;
        return 1;
    }

    TList<TJob> ls, rs;
    TList<TJob*> lrs;

    for (int i = 0; i < CountMulti_(opt, o, &l); ++i) {
        ls.push_back(TJob(GetMulti_(opt, o, &l, i)));
        lrs.push_back(&ls.back());
    }
    for (int i = 0; i < CountMulti_(opt, o, &r); ++i) {
        rs.push_back(TJob(GetMulti_(opt, o, &r, i)));
        lrs.push_back(&rs.back());
    }

    for (TJob* i : lrs) {
        TFileInput f(i->IFile);
        if (!NJson::ReadJsonTree(&f, &i->Json)) {
            Cerr << "file is not a json: " << i->IFile << Endl;
            return 1;
        }
        if (!NSnippets::ParseSerp(i->Json, i->Serp)) {
            Cout << "Failed to parse serp: " << i->IFile << Endl;
            return 1;
        }
        AddImage(i->Serp);
    }
    TCompareResult res(NOT_EVEN_TRIED, "epic fail");
    const TJob* lres = nullptr;
    const TJob* rres = nullptr;
    for (const TJob& i : ls) {
        for (const TJob& j : rs) {
            TCompareResult ret = Do(i.Serp, j.Serp);
            if (+ret.Value > +res.Value) {
                res = ret;
                lres = &i;
                rres = &j;
            }
        }
    }
    Cout << res.Message << "\t" << lres->IFile << "\t" << rres->IFile << Endl;
    return 0;
}
