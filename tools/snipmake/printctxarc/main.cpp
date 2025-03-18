#include <tools/snipmake/argv/opt.h>

#include <kernel/snippets/idl/snippets.pb.h>
#include <kernel/snippets/iface/archive/manip.h>

#include <kernel/tarc/markup_zones/arcreader.h>
#include <kernel/tarc/markup_zones/text_markup.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/stream/file.h>
#include <library/cpp/string_utils/base64/base64.h>

class TInput {
private:
    THolder<IInputStream> Input;
    size_t Line;
public:
    TInput(IInputStream* input)
        : Input(input)
        , Line(0)
    {
    }

    bool ReadRecord(TString& dst, TString& recId) {
        recId = ToString<size_t>(++Line);
        return Input->ReadLine(dst);
    }
};

static void PrintTitle(IOutputStream& out, const TBlob& docText) {
    TArchiveMarkupZones mZones;
    GetArchiveMarkupZones((const ui8*)docText.Data(), &mZones);
    TArchiveZone& z = mZones.GetZone(AZ_TITLE);
    TUtf16String title;
    if (!z.Spans.empty()) {
        TVector<int> sentNumbers;
        for (TVector<TArchiveZoneSpan>::const_iterator i = z.Spans.begin(); i != z.Spans.end(); ++i) {
            for (ui16 n = i->SentBeg; n <= i->SentEnd; ++n) {
                sentNumbers.push_back(n);
            }
        }
        TVector<TArchiveSent> outSents;
        GetSentencesByNumbers((const ui8*)docText.Data(), sentNumbers, &outSents, nullptr, false);

        TSentReader sentReader;
        for (TVector<TArchiveSent>::iterator i = outSents.begin(); i != outSents.end(); ++i) {
            if (title.length()) {
                title.append(' ');
            }
            title.append(sentReader.GetText(*i, mZones.GetSegVersion()));
        }
    }
    out << "TITLE=" << title << "\n";
}

static void Print(IOutputStream& out, const TString& ctxIn, const TString& ctxId, bool link, bool printSentAttrs) {
    NSnippets::NProto::TSnippetsCtx ctx;
    if (ctxIn.size() % 4 != 0) {
        Cerr << "Job " << ctxId << ": corrupted context" << Endl;
        return;
    }

    {
        const TString buf = Base64Decode(ctxIn);
        if (!ctx.ParseFromArray(buf.data(), buf.size())) {
            Cerr << "Job " << ctxId << ": cannot parse context" << Endl;
            return;
        }
        if (!ctx.GetQuery().HasQtreeBase64()) {
            Cerr << "Job "<< ctxId << ": no qtree data in context" << Endl;
            return;
        }
    }

    NSnippets::TVoidFetcher fetchText;
    NSnippets::TVoidFetcher fetchLink;
    NSnippets::TArcManip arcCtx(fetchText, fetchLink);
    const NSnippets::NProto::TArc* arc = nullptr;
    NSnippets::TArc* sarc = nullptr;
    if (link) {
        if (!ctx.HasLinkArc()) {
            out << "NO LINK ARC" << Endl;
            return;
        }
        arc = &ctx.GetLinkArc();
        sarc = &arcCtx.GetLinkArc();
    } else {
        if (!ctx.HasTextArc()) {
            out << "NO TEXT ARC" << Endl;
            return;
        }
        arc = &ctx.GetTextArc();
        sarc = &arcCtx.GetTextArc();
    }
    sarc->LoadState(*arc);

    out << "\n~~~~~~ Document " << ctxId << " ~~~~~~\n\n";
    PrintExtInfo(out, sarc->GetDescrBlob());
    PrintTitle(out, sarc->GetData());
    out << "\n";
    PrintDocText(out, sarc->GetData(), true, false, true, false, TSentReader(printSentAttrs));
}

int main (int argc, char** argv)
{
    Cerr << "WARNING: this program is not complete in any sense" << Endl;

    using namespace NLastGetopt;
    TOpts opt;
    TOpt& v = opt.AddLongOption("version").HasArg(NO_ARGUMENT);
    TOpt& l = opt.AddLongOption("linkarc").HasArg(NO_ARGUMENT);
    TOpt& attrs = opt.AddLongOption("sentattrs").HasArg(NO_ARGUMENT);
    TOpt& i = opt.AddCharOption('i', REQUIRED_ARGUMENT, "input file");
    opt.SetFreeArgsNum(0);

    TOptsParseResult o(&opt, argc, argv);

    if (Has_(opt, o, &v)) {
        Cout << GetProgramSvnVersion() << Endl;
        return 0;
    }

    bool link = Has_(opt, o, &l);
    bool printSentAttrs = Has_(opt, o, &attrs);
    TString inFile = GetOrElse_(opt, o, &i, "");
    THolder<TInput> inp;
    if (!!inFile) {
        inp.Reset(new TInput(new TFileInput(inFile)));
    } else {
        inp.Reset(new TInput(new TBufferedInput(&Cin)));
    }

    TString ctx;
    TString ctxId;
    while(inp->ReadRecord(ctx, ctxId)) {
        Print(Cout, ctx, ctxId, link, printSentAttrs);
    }
    return 0;
}
