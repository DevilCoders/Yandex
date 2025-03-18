#include <tools/snipmake/argv/opt.h>

#include <yweb/structhtml/htmlstatslib/runtime/zonestub.h>

#include <kernel/snippets/archive/zone_checker/zone_checker.h>
#include <kernel/snippets/idl/snippets.pb.h>
#include <kernel/snippets/iface/archive/manip.h>

#include <kernel/tarc/markup_zones/arcreader.h>
#include <kernel/tarc/markup_zones/text_markup.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/html/pcdata/pcdata.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/stream/file.h>
#include <util/stream/str.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/charset/wide.h>
#include <util/folder/path.h>

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

static void PrintOverlappedHtml(IOutputStream& out, const TBlob& docText)
{
    TVector<TArchiveSent> outSents;
    TArchiveMarkupZones mZones;
    TArchiveWeightZones wZones;
    GetArchiveMarkupZones((const ui8*)docText.Data(), &mZones);

    TVector<int> sentNumbers;
    GetSentencesByNumbers((const ui8*)docText.Data(), sentNumbers, &outSents, nullptr, false);

    const char* colors[] = {"template", "unique", "main_content", "unique_main_content"};
    int counts[4] = {0, 0, 0, 0};

    NSnippets::TForwardInZoneChecker titleChecker(mZones.GetZone(AZ_TITLE).Spans);
    NSnippets::TForwardInZoneChecker mainContentChecker(mZones.GetZone(AZ_MAIN_CONTENT).Spans);
    NSnippets::TForwardInZoneChecker templateSentChecker(mZones.GetZone(AZ_TEMPLATE_SENT).Spans);

    /*for (const auto& z : mZones.GetZone(AZ_TEMPLATE_SENT).Spans) {
       out << "<div>" << z.SentBeg << ":" << z.OffsetBeg << " - " << z.SentEnd << ":" << z.OffsetEnd << "</div>\n";
    }*/

    for (const TArchiveSent& sent : outSents) {
        titleChecker.SeekToSent(sent.Number);
        mainContentChecker.SeekToSent(sent.Number);
        templateSentChecker.SeekToSent(sent.Number);
        if (titleChecker.IsSentInCurrentZone(sent.Number)) {
            continue;
        }
        bool inMainContent = mainContentChecker.IsSentInCurrentZone(sent.Number);
        bool isUnique = !templateSentChecker.IsSentInCurrentZone(sent.Number);
        int index = inMainContent*2 + isUnique;
        counts[index]++;
        const char* color = colors[index];
        TUtf16String escaped = sent.OnlyText;
        EscapeHtmlChars<false>(escaped);
        out << "<div ";
        if (color) {
            out << "class=\"" << color << "\"";
        }
        out << ">" << escaped << "</div>\n";
    }
}

static bool Print(IOutputStream& out, const TString& ctxIn, const TString& ctxId) {
    NSnippets::NProto::TSnippetsCtx ctx;
    if (ctxIn.size() % 4 != 0) {
        Cerr << "Job " << ctxId << ": corrupted context" << Endl;
        return false;
    }

    {
        const TString buf = Base64Decode(ctxIn);
        if (!ctx.ParseFromArray(buf.data(), buf.size())) {
            Cerr << "Job " << ctxId << ": cannot parse context" << Endl;
            return false;
        }
        if (!ctx.GetQuery().HasQtreeBase64()) {
            Cerr << "Job "<< ctxId << ": no qtree data in context" << Endl;
            return false;
        }
    }

    NSnippets::TVoidFetcher fetchText;
    NSnippets::TVoidFetcher fetchLink;
    NSnippets::TArcManip arcCtx(fetchText, fetchLink);
    const NSnippets::NProto::TArc* arc = nullptr;
    NSnippets::TArc* sarc = nullptr;
    if (!ctx.HasTextArc()) {
        Cerr << "NO TEXT ARC" << Endl;
        return false;
    }
    arc = &ctx.GetTextArc();
    sarc = &arcCtx.GetTextArc();

    sarc->LoadState(*arc);

    TStringStream extInfo;
    extInfo << "\n~~~~~~ Document " << ctxId << " ~~~~~~\n\n";
    PrintExtInfo(extInfo, sarc->GetDescrBlob());
    PrintTitle(extInfo, sarc->GetData());
    TString extInfoEscaped = EncodeHtmlPcdata(extInfo.Str(), true);
    out << "<pre>" << extInfoEscaped << "</pre>\n";

    PrintOverlappedHtml(out, sarc->GetData());
    return true;
}

struct TProcessingResult
{
    TString Id;
    TString HtmlBody;
};

using TResultPtr = TSimpleSharedPtr<TProcessingResult>;

int main (int argc, char** argv)
{
    using namespace NLastGetopt;

    TOpts opt;
    TOpt& v = opt.AddLongOption("version").HasArg(NO_ARGUMENT);
    TOpt& i = opt.AddCharOption('i', REQUIRED_ARGUMENT, "input file");
    opt.SetFreeArgsNum(0);

    TOptsParseResult o(&opt, argc, argv);

    if (Has_(opt, o, &v)) {
        Cout << GetProgramSvnVersion() << Endl;
        return 0;
    }

    TString header;

    {
        TUnbufferedFileInput htmlHeader("header.html");
        header = htmlHeader.ReadAll();
    }

    TString inFile = GetOrElse_(opt, o, &i, "");
    TInput inf(new TBufferedInput(&Cin, 1<<24));

    TVector<TResultPtr> results;

    TFsPath outDir("overlap_samples");
    TString ctx;
    TString ctxId;
    while(inf.ReadRecord(ctx, ctxId)) {
        TResultPtr result(new TProcessingResult());
        result->Id = ctxId;
        TStringOutput out(result->HtmlBody);
        if (Print(out, ctx, ctxId)) {
            results.push_back(result);
        }
    }

    for (size_t i = 0; i < results.size(); ++i) {
        TString prevId;
        if (i > 0) {
            prevId = results[i-1]->Id;
        }
        TString nextId;
        if (i < results.size() - 1) {
            nextId = results[i+1]->Id;
        }
        TFsPath outFile = outDir / results[i]->Id;
        TFixedBufferFileOutput outf(outFile.GetPath() + ".html");
        outf << header;
        if (i > 0) {
            outf << "<a id=\"prevLink\" href=\"" << results[i-1]->Id << ".html\"> &lt;- Previous</a> \n";
        }
        if (i < results.size() - 1) {
            outf << "<a id=\"nextLink\" href=\"" << results[i+1]->Id << ".html\"> Next -&gt;</a> \n";
        }
        outf << "<br>\n";
        outf << results[i]->HtmlBody;
        outf << "</body></html>";
    }

    return 0;
}
