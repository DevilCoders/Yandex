#include <tools/snipmake/snipdat/askctx.h>
#include <tools/snipmake/snippet_xml_parser/cpp_writer/snippet_dump_xml_writer.h>

#include <search/idl/meta.pb.h>
#include <search/session/compression/report.h>

#include <kernel/snippets/strhl/goodwrds.h>

#include <library/cpp/getopt/opt.h>

#include <library/cpp/charset/recyr.hh>
#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/network/socket.h>
#include <util/stream/file.h>
#include <library/cpp/http/io/stream.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/string/cast.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/split.h>

namespace NSnippets
{
    struct TJob {
        TString Tmp;
        TString Req;
        TString Qtree;
        NMetaProtocol::TReport Report;
        TJob(TString tmp)
          : Tmp(tmp)
          , Report()
        {
        }
        void GetReqRes(TString& req, TString& qtree, TString& res) {
            TVector<TString> v;
            TCharDelimiter<const char> del('\t');
            TContainerConsumer< TVector<TString> > cons(&v);
            SplitString(Tmp.data(), Tmp.data() + Tmp.size(), del, cons);
            if (v.size() != 3) {
                return;
            }
            req = v[0];
            qtree = v[1];
            res = v[2];
        }
        void Process() {
            TString in;
            GetReqRes(Req, Qtree, in);
            in = Base64Decode(in);
            Y_PROTOBUF_SUPPRESS_NODISCARD Report.ParseFromArray(in.data(), in.size());
            NMetaProtocol::Decompress(Report); // decompresses only if needed
        }
    };
    inline TUtf16String Rehighlight(const TUtf16String& s) {
        TUtf16String res;
        for (size_t i = 0; i < s.size(); ) {
            if (s[i] == 0x07 && i + 1 < s.size()) {
                if (s[i + 1] == '[' || s[i + 1] == ']') {
                    res += UTF8ToWide(s[i + 1] == '[' ? "<b>" : "</b>");
                    i += 2;
                    continue;
                }
            }
            res += s[i];
            ++i;
        }
        return res;
    }
    struct TMain {
        TString Mode;
        TMain()
          : Mode("xml")
        {
        }
        void ProcessReport(NMetaProtocol::TReport& report, const TString& req, const TString& qtree, TSnippetDumpXmlWriter& xmlOutWriter) {
            //if (report.HasErrorInfo()) {
                //if (report.GetErrorInfo().GetGotError() == NMetaProtocol::TErrorInfo::YES) {
                    //return;
                //}
            //}
            if (report.GroupingSize() < 1)
                return;
            const NMetaProtocol::TGrouping& grouping = report.GetGrouping(0);
            if (grouping.GetIsFlat() != NMetaProtocol::TGrouping::YES)
                return;

            for (size_t i = 0; i < grouping.GroupSize(); ++i) {
                const NMetaProtocol::TGroup& group = grouping.GetGroup(i);
                for (size_t j = 0; j < group.DocumentSize(); ++j) {
                    const NMetaProtocol::TDocument& doc = group.GetDocument(j);
                    if (doc.HasArchiveInfo()) {
                        if (Mode == "ctx") {
                            Cout << FindCtx(doc.GetArchiveInfo()) << Endl;
                            continue;
                        }
                        const NMetaProtocol::TArchiveInfo& ar = doc.GetArchiveInfo();
                        if (!ar.HasTitle())
                            continue;
                        TUtf16String title = Rehighlight(UTF8ToWide(ar.GetTitle()));
                        TUtf16String headline;
                        TString expl;
                        for (size_t i = 0; i < ar.GtaRelatedAttributeSize(); ++i) {
                            if (TString(ar.GetGtaRelatedAttribute(i).GetKey()) == "_SnippetsExplanation")
                                expl = TString(ar.GetGtaRelatedAttribute(i).GetValue());
                        }
                        if (ar.HasHeadline())
                            headline = Rehighlight(UTF8ToWide(ar.GetHeadline()));
                        TVector<TUtf16String> snippetPassages;
                        if (ar.PassageSize()) {
                            for (size_t i = 0; i < ar.PassageSize(); ++i)
                                snippetPassages.push_back(Rehighlight(UTF8ToWide(ar.GetPassage(i))));
                        }
                        if (ar.HasUrl()) {
                            if (Mode == "xml") {

                                xmlOutWriter.StartQDPair(req, Recode(CODES_YANDEX, CODES_UTF8, ar.GetUrl()),
                                    "1", "213", qtree);

                                TReqSnip snippet;
                                snippet.TitleText = title;

                                if (headline.size()) {
                                    TSnipFragment fragment;
                                    fragment.Text = headline;
                                    snippet.SnipText.push_back(fragment);
                                }
                                for (size_t i = 0; i < snippetPassages.size(); ++i) {
                                    TSnipFragment fragment;
                                    fragment.Text = snippetPassages[i];
                                    snippet.SnipText.push_back(fragment);
                                }

                                xmlOutWriter.WriteSnippet(snippet);
                                xmlOutWriter.FinishQDPair();

                            } else if (Mode == "expl") {
                                Cout << req << '\t' << qtree << '\t' << expl << Endl;
                            }
                        }
                    }
                }
            }
        }

        void PrintUsage() {
            Cout << "metasnip-parse [-i input] [-t outputType]" << Endl;
            Cout << "  -t xml: snippets.xml (default)" << Endl;
            Cout << "  -t expl: output _SnippetsExplanation" << Endl;
            Cout << "  -t ctx: output _SnippetsCtx" << Endl;
        }

        int Run(int argc, char** argv) {
            Opt opt(argc, argv, "i:m:M:t:");
            int optlet;
            TString inpf;
            while ((optlet = opt.Get()) != EOF) {
                switch (optlet) {
                    case 'i':
                        inpf = opt.Arg;
                        break;
                    case 't':
                        Mode = opt.Arg;
                        if (Mode != "xml" && Mode != "expl" && Mode != "ctx") {
                            PrintUsage();
                            return 1;
                        }
                        break;

                    case '?':
                    default:
                        PrintUsage();
                        return 0;
                        break;

                }
            }

            THolder<TFileInput> inf;
            IInputStream* qIn = &Cin;
            if (!inpf.empty()) {
                inf.Reset(new TFileInput(inpf));
                qIn = inf.Get();
            }
            TSnippetDumpXmlWriter xmlOutWriter(Cout);

            if (Mode == "xml") {
                xmlOutWriter.Start();
                xmlOutWriter.StartPool("4q");
            }
            TString tmp;
            while (qIn->ReadLine(tmp)) {
                TJob job(tmp);
                job.Process();
                ProcessReport(job.Report, job.Req, job.Qtree, xmlOutWriter);
            }
            if (Mode == "xml") {
                xmlOutWriter.FinishPool();
                xmlOutWriter.Finish();
            }
            return 0;
        }
    };

}

int main(int argc, char** argv) {
    NSnippets::TMain main;
    return main.Run(argc, argv);
}
