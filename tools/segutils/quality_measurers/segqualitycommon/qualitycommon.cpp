#include "qualitycommon.h"

#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/tarcio.h>
#include <kernel/tarc/markup_zones/text_markup.h>

#include <library/cpp/getopt/opt.h>

#include <util/folder/dirut.h>
#include <util/string/vector.h>
#include <util/string/strip.h>
#include <util/thread/pool.h>

namespace NSegutils {

void TOpts::UsageAndExit(const char* me, int code) {
    Cerr << me << " -r <log|arc|json> -a <log or arc or json path> -m <mapping path> -f <files dir> -c <config dir> "
                    "-W <min> [-n <skip>] [-N <num>] [-S] [-h] [-l]";

    CallString();

    Cerr << Endl;

    Cerr << "\t" << "-r <log|arc|json> for the blogs log / news arc / news json mode" << Endl;
    Cerr << "\t" << "-a <path> path to the blogs log or the news arc" << Endl;
    Cerr << "\t" << "-m <path> path to the file-url mapping" << Endl;
    Cerr << "\t" << "-f <path> path to the files dir" << Endl;
    Cerr << "\t" << "-c <path> path to the dir with htparser.ini, 2ld.list and dict.dict" << Endl;
    Cerr << "\t" << "-W <min> min words in news article to use it" << Endl;
    Cerr << "\t" << "-l learn mode" << Endl;
    Cerr << "\t" << "-S skip sport news" << Endl;
    Cerr << "\t" << "-h header features (segment features otherwise)" << Endl;

    UsageDescription();
    exit(code);
}

TOpts::EMode TOpts::ProcessOpts(int argc, const char** argv) {
    if (argc < 2 || argc > 1 && strcmp(argv[1], "--help") == 0)
        UsageAndExit(argv[0], 0);

    TString mapping;
    Opt opts(argc, argv, ("r:a:m:f:c:n:N:SW:hlH" + OptsString()).c_str());

    THtmlFileReader reader;

    TString confDir;
    int optlet;
    while (EOF != (optlet = opts.Get())) {
        switch (optlet) {
        default:
            if (!Process(optlet, opts))
                UsageAndExit(argv[0], 1);
            break;
        case 'a':
            RefDataPath = opts.GetArg();
            break;
        case 'm':
            reader.InitMapping(opts.GetArg());
            break;
        case 'l':
            LearnMode = true;
            break;
        case 'f':
            reader.SetDirectory(opts.GetArg());
            LearnReader.SetDirectory(opts.GetArg());
            TestReader.SetDirectory(opts.GetArg());
            LogFileDirectoryPath = opts.GetArg();
            break;
        case 'S':
            SkipSport = true;
            break;
        case 'W':
            MinWords = FromString<ui32> (opts.GetArg());
            break;
        case 'r': {
            TString m = opts.GetArg();
            Mode = m == "log" ? M_LOG : m == "arc" ? M_ARC : m == "json" ? M_JSON : M_NONE;
            if (M_NONE == Mode)
                UsageAndExit(argv[0], 1);
            break;
        }
        case 'c':
            confDir = opts.GetArg();
            break;
        case 'h':
            Headers = true;
            break;
        case 'H':
            RefHasHeaders = false;
            break;
        }
    }

    if (!confDir) {
        Clog << "error: Failed to initialize parser" << Endl;
        UsageAndExit(argv[0], 1);
    }

    Ctx.Reset(new TParserContext(confDir));

    Ctx->SetUriFilterOff();
    THashSet<TString> learn;

    if (THtmlFileReader::MDM_Mapping == reader.MetaMode) {
        THashMap<TString, ui32> hostcount;

        for (TMapping::const_iterator it = reader.File2MetaData.begin(); it != reader.File2MetaData.end(); ++it) {
            NSegm::TUrlInfo inf(reader.GetUrl(it->first), Ctx->GetOwnerCanonizer());
            hostcount[inf.ParseRes.Owner]++;
        }

        ui32 lcount = 0;
        ui32 tcount = 0;

        for (THashMap<TString, ui32>::const_iterator it = hostcount.begin(); it != hostcount.end(); ++it) {
            if (lcount < tcount) {
                learn.insert(it->first);
                lcount += it->second;
            } else {
                tcount += it->second;
            }
        }

        for (TMapping::const_iterator it = reader.File2MetaData.begin(); it != reader.File2MetaData.end(); ++it) {
            NSegm::TUrlInfo inf(reader.GetUrl(it->first), Ctx->GetOwnerCanonizer());

            if (learn.contains(inf.ParseRes.Owner)) {
                LearnReader.File2MetaData[it->first] = it->second;
                LearnReader.FileList.push_back(it->first);
                LearnReader.Url2File[inf.RawUrl] = it->first;
            } else {
                TestReader.File2MetaData[it->first] = it->second;
                TestReader.FileList.push_back(it->first);
                TestReader.Url2File[inf.RawUrl] = it->first;
            }
        }
    }

    return Mode;
}


bool TDocProcessor::ProcessLive() {
    using namespace NSegm;

    try {
        Opts->Ctx->SegNumerateDocument(&Handler, Data);
    } catch (const yexception& e) {
        Clog << e.what() << Endl;
        return false;
    }

    Handler.GetTokens().CollectHashes(Title, THashToken::TitleFilter(true));
    Handler.GetTokens().CollectHashes(Text, THashToken::TitleFilter(false));

    return true;
}

bool TDocProcessor::ProcessRef(const TUtf16String& title, const TUtf16String& text) {
    using namespace NSegm;
    THtmlDocument d;
    d.Url = Data.Url;
    d.HttpCharset = CODES_UTF8;
    d.Html = Sprintf("<html>"
        "<head><title>%s</title></head>"
        "<body>%s</body>"
        "</html>", WideToUTF8(title).c_str(), WideToUTF8(text).c_str());

    TSegHandler refnum;

    try {
        refnum.SetSkipMainContent();
        Opts->Ctx->SegNumerateDocument(&refnum, d);
    } catch (const yexception& e) {
        Clog << e.what() << Endl;
        return false;
    }

    refnum.GetTokens().CollectHashes(RefHeader, THashToken::TitleFilter(true));
    refnum.GetTokens().CollectHashes(RefContent, THashToken::TitleFilter(false));

    return true;
}

bool TDocProcessor::Postprocess() {
    MakeLCS<size_t>(Text, RefHeader, &RealHeader);
    MakeLCS<size_t>(Text, RefContent, &RealContent);

    float allh = RefHeader.size() ? float(RealHeader.size()) / RefHeader.size() : 1.;
    float allc = RefContent.size() ? float(RealContent.size()) / RefContent.size() : 1.;
    return allh > 0.75 && allc > 0.65 && RealContent.size() > Opts->MinWords;
}

bool TArcProcessor::OnDocBase(TDocProcessor& proc, const TArchiveHeader* curHdr, const TArchiveIterator& arcIter) {
    TBlob docText = arcIter.GetDocText(curHdr);

    TVector<TArchiveSent> outSents;
    TArchiveMarkupZones mZones;
    GetArchiveMarkupZones((const ui8*) docText.Data(), &mZones);
    GetSentencesByNumbers((const ui8*) docText.Data(), TVector<int> (), &outSents, nullptr, true);

    const TArchiveZone& titleZone = mZones.GetZone(AZ_TITLE);
    TUtf16String title;
    TUtf16String text;

    for (TVector<TArchiveSent>::const_iterator it = outSents.begin(); it != outSents.end(); ++it) {
        if (!titleZone.Spans.empty() && it->Number <= titleZone.Spans.front().SentEnd)
            title += it->OnlyText;
        else
            text += it->OnlyText;
    }

    EscapeHtmlChars<false> (title);
    EscapeHtmlChars<false> (text);

    return proc.ProcessRef(title, text) && proc.ProcessLive();
}

void TArcProcessor::Process() {
    typedef THashMap<TString, ui32> TDocMapping;
    TDocMapping mapping;

    {
        TArchiveIterator arcIter;
        arcIter.Open(Opts->RefDataPath.c_str());

        for (const TArchiveHeader* curHdr = arcIter.NextAuto();
                        curHdr;
                        curHdr = arcIter.NextAuto()) {
            TBlob extInfo = arcIter.GetExtInfo(curHdr);
            TDocDescr docDescr;
            docDescr.UseBlob(extInfo.Data(), (unsigned int) extInfo.Size());

            TString url = AddSchemePrefix(docDescr.get_url());

            if (Opts->SkipSport && (~url.find("sport")
                            || ~url.find("football")
                            || ~url.find("soccer")
                            || ~url.find("champion")))
                continue;

            TMapping::const_iterator it = Opts->GetReader().Url2File.find(url);

            if (it != Opts->GetReader().Url2File.end() && Opts->GetReader().File2MetaData.contains(it->second))
                mapping[url] = curHdr->DocId;
        }
    }

    TVector<std::pair<ui32, TString> > ids;

    for (TMapping::const_iterator it = Opts->GetReader().Url2File.begin();
                    it != Opts->GetReader().Url2File.end(); ++it) {

        TDocMapping::const_iterator dit = mapping.find(it->first);

        if (dit != mapping.end())
            ids.push_back(std::make_pair(dit->second, dit->first));
    }

    Sort(ids.begin(), ids.end());

    {
        TArchiveIterator arcIter;
        arcIter.Open(Opts->RefDataPath.c_str());

        const TArchiveHeader* curHdr = nullptr;
        for (TVector<std::pair<ui32, TString> >::const_iterator it = ids.begin(); it != ids.end(); ++it) {
            while (curHdr = arcIter.NextAuto())
                if (it->first <= curHdr->DocId)
                    break;

            if (!curHdr)
                break;

            if (it->first != curHdr->DocId)
                continue;

            TBlob extInfo = arcIter.GetExtInfo(curHdr);
            TDocDescr docDescr;
            docDescr.UseBlob(extInfo.Data(), (unsigned int) extInfo.Size());

            OnDoc(curHdr, arcIter, it->second);
        }
    }
}

void ParseLogDoc(const TString& line, TString& url, TUtf16String& content)
{
    TVector<TString> vs = SplitString(line, "\t", 2);

    if (vs.size() < 2)
        ythrow yexception ();

    url = vs[0];
    content = UTF8ToWide(StripInPlace(vs[1]));
    EscapeHtmlChars<false>(content);
}

TLogDoc::TLogDoc() {}

TLogDoc::TLogDoc(const TString& line) {
    TUtf16String content;
    ParseLogDoc(line, Url, content);
    Header = TUtf16String();
    Content = content;
}

void TLogDoc::AppendContent(const TUtf16String& content)
{
    Content += u" " + content;
}

void TLogProcessor::Process() {
    TString line;
    TUnbufferedFileInput in(Opts->RefDataPath);
    {
        TLogDoc logDoc;
        while (in.ReadLine(line)) {
            line = StripInPlace(line);
            if (line.empty())
                continue;

            TString url;
            TUtf16String content;
            ParseLogDoc(line, url, content);
            if (logDoc.Url.size() == 0) {
                logDoc.Url = url;
            }

            if (logDoc.Url == url) {
                logDoc.AppendContent(content);
            } else {
                TMapping::const_iterator it = Opts->GetReader().Url2File.find(logDoc.Url);
                if (it == Opts->GetReader().Url2File.end() || !Opts->GetReader().File2MetaData.contains(it->second)) {
                    continue;
                }
                OnDoc(logDoc);
                logDoc.Url = url;
                logDoc.Content = content;
            }
        }
        OnDoc(logDoc);
    }
}

}
