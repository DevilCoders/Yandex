#include <library/cpp/deprecated/dater_old/structs.h>
#include <kernel/indexer/faceproc/docattrs.h>
#include <kernel/doom/chunked_wad/doc_chunk_mapping_searcher.h>
#include <kernel/tarc/disk/searcharc.h>
#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/farcface.h>
#include <kernel/tarc/markup_zones/arcreader.h>
#include <kernel/tarc/markup_zones/unpackers.h>
#include <kernel/tarc/markup_zones/view_sent.h> // for TTArcViewSentReader

#include <yweb/protos/robot.pb.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/getopt/opt.h>

#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/stream/zlib.h>

struct TProgramTArcViewOptions {
    bool ExternalInfo;
    bool CheckOnly;
    bool MarkupZone;
    bool MarkupZoneAttrs;
    bool WeightZone;
    bool FullArchive;
    bool NeedExtended;
    bool urlDumpFormat;
    bool readFromInput;
    bool SequentialInput;
    bool Converted2Html;
    bool SentAttrs;
    bool DecodeSentAttrs;
    bool TitleAsAttr;
    bool AnchorLinks;
    bool CompressedExtBlob;
    ui32 DocumentNumber;
    ui32 DocumentId;
    bool DirectReadMode;
    TProgramTArcViewOptions()
        : ExternalInfo(false)
        , CheckOnly(false)
        , MarkupZone(false)
        , MarkupZoneAttrs(false)
        , WeightZone(false)
        , FullArchive(false)
        , NeedExtended(false)
        , urlDumpFormat(false)
        , readFromInput(false)
        , SequentialInput(false)
        , Converted2Html(false)
        , SentAttrs(false)
        , DecodeSentAttrs(false)
        , TitleAsAttr(false)
        , AnchorLinks(false)
        , CompressedExtBlob(false)
        , DocumentNumber(0)
        , DocumentId(0)
        , DirectReadMode(false)
    {}
};

class TProgramTArcViewDocIterator {
private:
    TProgramTArcViewOptions Opts;
    ui32 failDoc, curDoc, lastDoc;
    ui32 sequentialDoc = 0;
public:
    TProgramTArcViewDocIterator(int argc, char *argv[], int optInd, const TProgramTArcViewOptions &opts)
        : Opts(opts)
        , failDoc(0xFFFFFFFF)
        , curDoc(failDoc)
        , lastDoc(failDoc) {
        if (argc - optInd > 1)
            curDoc = FromString<ui32>(argv[optInd + 1]);
        if (argc - optInd == 3)
            lastDoc = atol(argv[optInd + 2]);
        if (curDoc != failDoc && curDoc > lastDoc)
            ythrow yexception() << "First <" <<  curDoc << "> doc is less than last <" <<  lastDoc << "> doc\n";
        ReadFromInput(curDoc);
    }
    bool ReadFromInput(ui32 &docId){
        if (!Opts.readFromInput)
            return false;
        if (Opts.SequentialInput) {
            docId = sequentialDoc++;
            return true;
        }
        int sc;
        char idStr[256];

        if ((sc = scanf("%s", &idStr[0])) == 1){
            docId = FromString<ui32>(idStr);
            return true;
        }
        if (sc != EOF) {
            Cerr << "\n expected integer docId in input\n";
            return false;
        }
        docId = failDoc;
        return false;
    }
    ui32 CurDoc(){
        return curDoc;
    }
    ui32 End(){
        return failDoc;
    }

    ui32 Next() {
        if (ReadFromInput(curDoc))
            return curDoc;
        if (curDoc == failDoc || lastDoc == failDoc || curDoc >= lastDoc)
            return (curDoc = failDoc);
        if (curDoc >= lastDoc)
            curDoc = failDoc;
        else
            ++curDoc;
        return curDoc;
    }

};

TString MakeStr(const char *ptr, const char *defPtr = nullptr){
    if (ptr)
        return TString(ptr);
    if (defPtr)
        return TString(defPtr);
    return TString("");
}

void PrintDocSummary(IOutputStream& os, const TBlob& extInfo, TArchiveHeader* curHdr) {
    TDocDescr docDescr;
    docDescr.UseBlob(extInfo.Data(), (unsigned int)extInfo.Size());
    if (!docDescr.IsAvailable()) {
        os << "\t" << curHdr->DocId <<"\t\t\t\t\t\t\t\n";
        return;
    }
    TDocInfos docInfo;
    docDescr.ConfigureDocInfos(docInfo);
    TString lang("-1"), mime = MakeStr(strByMime(docDescr.get_mimetype()), "UNKNOWN_MIME"), nameByDC("UNKNOWN_DOC_CODE");
    ECharset dc = docDescr.get_encoding();
    if (CODES_UNKNOWN < dc && dc < CODES_MAX)
        nameByDC = NameByCharset(dc);

    if (docInfo.find("lang") != docInfo.end())
        lang = docInfo["lang"];

    os << docDescr.get_urlid() << "\t"
       << curHdr->DocId << "\t"
       << docDescr.get_hostid() << "\t"
       << docDescr.get_size() << "\t"
       << docDescr.get_mtime() << "\t"
       << nameByDC << "\t"
       << lang.c_str() << "\t"
       << mime << "\t"
       << docDescr.get_url() << "\n";
}

static void PrintAnchor(IOutputStream& os, TBuffer& buf) {
    typedef ::google::protobuf::RepeatedPtrField< ::TAnchorText_TAnchorTextEntry >::const_iterator  TFieldIterator;

    TMemoryInput    mi(buf.Data(), buf.Size());
    TAnchorText     resp;

    resp.ParseFromArcadiaStream(&mi);

    os << "CrawlRank    " << resp.crawlrank()    << "\n";
    os << "AddTime      " << resp.addtime()      << "\n";
    os << "CrawlRankErf " << resp.crawlrankerf() << "\n";

    for (TFieldIterator i = resp.anchortext().begin(); i != resp.anchortext().end(); ++i) {
        os << "\tUrl           " << i->url().c_str()        << "\n";
        os << "\tAnchorText    " << i->anchortext().c_str() << "\n";
        os << "\tFlags         " << i->flags()              << "\n";
        os << "\tCrawlRank     " << i->crawlrank()          << "\n";
        os << "\tDiscoveryTime " << i->discoverytime()      << "\n";
        os << "\tLanguage      " << i->language()           << "\n";
    }
}

void PrintDoc(const TProgramTArcViewOptions& opt, IOutputStream& os, const TBlob& extInfo, const TBlob& docText, TArchiveHeader* curHdr) {
    if (opt.urlDumpFormat)
        return PrintDocSummary(os, extInfo, curHdr);
    os << "\n~~~~~~ Document " << curHdr->DocId << " ~~~~~~\n\n";

    if (opt.ExternalInfo) {
        PrintExtInfo(os, extInfo);
        if (opt.TitleAsAttr) {
            TUtf16String title = ExtractDocTitleFromArc(docText);
            os << "TITLE=" << title << "\n";
        }
        os << '\n';
    }

    TTArcViewSentReader sentReader(opt.SentAttrs, opt.DecodeSentAttrs);
    PrintDocText(os, docText, opt.MarkupZone, opt.WeightZone, opt.NeedExtended, opt.TitleAsAttr, sentReader, opt.MarkupZoneAttrs);
}

void PrintDoc(const TProgramTArcViewOptions& opt, IOutputStream& os, TArchiveIterator& arcIter , TArchiveHeader* curHdr) {
    TBlob extInfo = arcIter.GetExtInfo(curHdr);
    TBlob docText = arcIter.GetDocText(curHdr);
    PrintDoc(opt, os, extInfo, docText, curHdr);
}

void PrintDoc(const TProgramTArcViewOptions& opt, IOutputStream& os, TFullArchiveIterator& arcIter, TArchiveHeader*) {
    os << "\n~~~~~~ Document " << arcIter.GetDocId() << " ~~~~~~\n\n";
    const TFullArchiveDocHeader* hdr = arcIter.GetFullHeader();
    os << "URL=" << hdr->Url << "\n";

    time_t t = hdr->IndexDate;
    if (t) {
        const char* tt = ctime(&t);
        os << "INDEXDATE=" << tt;
    }

    if (hdr->Flags)
        os << "FLAGS=" << hdr->Flags << "\n";

    const char* mime_str = strByMime((MimeTypes)hdr->MimeType);
    if (mime_str)
        os << "MIMETYPE=" << mime_str << "\n";

    ECharset e = (ECharset)hdr->Encoding;
    if (CODES_UNKNOWN < e && e < CODES_MAX)
        os << "CHARSET=" <<  NameByCharset(e) << "\n";

    if ((ELanguage)hdr->Language < LANG_MAX)
        os << "LANGUAGE=" <<  NameByLanguage(static_cast<ELanguage>(hdr->Language)) << "\n";

    arcIter.MakeDocText();
    os << "SIZE=" << (ui32)arcIter.GetDocTextSize() << "\n";

    static TBuffer ConvBuf;
    static TBuffer AnchorBuf;

    ConvBuf.Clear();
    AnchorBuf.Clear();

    arcIter.GetDocTextPart(FABT_HTMLCONV, &ConvBuf);
    if (ConvBuf.Size() != 0)
        os << "HTMLCONVSIZE=" << ConvBuf.Size() << "\n";

    arcIter.GetDocTextPart(FABT_ANCHORTEXT, &AnchorBuf);
    if (AnchorBuf.Size() != 0)
        os << "ANCHORTEXTSIZE=" << AnchorBuf.Size() << "\n";

    TBlob extInfo = arcIter.GetExtInfo();
    if (!extInfo.Empty()) {
        TMemoryInput in(extInfo.Data(), extInfo.Size());
        THashMap<TString, TString> attrs;
        if (opt.CompressedExtBlob) {
            TZLibDecompress zIn((IInputStream*)&in, ZLib::ZLib);
            UnpackFrom(&zIn, &attrs);
        } else {
            UnpackFrom(&in, &attrs);
        }
        for (THashMap<TString, TString>::const_iterator i = attrs.begin(); i != attrs.end(); ++i)
            os << i->first << "=" << i->second << "\n";
    }

    if (opt.Converted2Html && ConvBuf.Size() != 0)
        os.Write(ConvBuf.Data(), ConvBuf.Size());
    else if (opt.AnchorLinks && AnchorBuf.Size() != 0)
        PrintAnchor(os, AnchorBuf);
    else
        os.Write(arcIter.GetDocText(), arcIter.GetDocTextSize());
    os << "\n";
    os.Flush();
}

void PrintArchive(const TProgramTArcViewOptions& opt, IOutputStream& os, TArchiveIterator& arcIter) {
    TArchiveHeader* curHdr = arcIter.NextAuto();
    ui32 curDocumentNumber = 0;
    while (curHdr) {
        ++curDocumentNumber;
        if (!opt.CheckOnly && (opt.DocumentNumber == 0 || opt.DocumentNumber == curDocumentNumber)) {
            PrintDoc(opt, os, arcIter, curHdr);
            if (opt.DocumentNumber != 0) {
                break;
            }
        }
        curHdr = arcIter.NextAuto();
    }
}

void PrintFullArchive(const TProgramTArcViewOptions& opt, IOutputStream& os, TFullArchiveIterator& arcIter) {
    bool error = false;
    ui32 curDocumentNumber = 0;
    while (arcIter.Next()) {
        try {
            ++curDocumentNumber;
            if (opt.DocumentNumber == 0 || opt.DocumentNumber == curDocumentNumber) {
                if (opt.CheckOnly) {
                    arcIter.MakeDocText();
                } else {
                    PrintDoc(opt, os, arcIter, nullptr);
                }
                if (opt.DocumentNumber != 0) {
                    break;
                }
            }
        } catch (std::exception& ex) {
            Cerr << "Error in docid " << arcIter.GetDocId() << " : " << ex.what() << Endl;
            error = true;
        }
    }
    if (error)
        ythrow yexception() << "Errors found in archive";
}

void PrintDocumentByDocid(const TString& arcName, const TProgramTArcViewOptions& options, IOutputStream& os) {
    TSearchArchive sarc;
    sarc.Open(arcName, AOM_FILE, AT_FLAT);
    TBlob docText = sarc.GetDocText(options.DocumentId);
    TBlob extInfo = sarc.GetExtInfo(options.DocumentId);
    TBlob fullDoc = sarc.GetDocFullInfo(options.DocumentId);
    TArchiveHeader* header = (TArchiveHeader*)fullDoc.Data();
    sarc.GetDocFullInfo(options.DocumentId);
    PrintDoc(options, os, extInfo, docText, header);
}

void PrintUsage()
{
    Cerr <<
    "Copyright (c) OOO \"Yandex\". All rights reserved.\n"
    "tarcview - prints doc from text archive\n"
    "Usage: tarcview [-aecdhilsxmwu] [-k number] [-o output] archivepath [FirstDocN [LastDocN]]\n"
    "archivepath - path to the arc-file without suffix 'arc'\n"
    "-a print sentence attributes\n"
    "-e print also external info\n"
    "-c don't print anything to output, check only\n"
    "-d print documents converted to HTML when available\n"
    "-h view full archive\n"
    "-i read doc nums from standard input\n"
    "-S use sequential numbers to the end of archive instead of standard input (forces -i)\n"
    "-l print foreign link data when available\n"
    "-s decode sentence attributes (like SegSt and DaterDate)\n"
    "-x view extended blocks\n"
    "-m print marks of markup zones\n"
    "-z print markup zone attributes\n"
    "-w print marks of weighted zones\n"
    "-u dump in urlXXX.dmp format\n"
    "-t print document title as additional attribute, and remove it from document body\n"
    "-f attribute blob in full archive is compressed\n"
    "-k only print a document if it's k-th from the beginning (regardless of docIds - for sparse archives)\n"
    "-D read document directly by given docId\n"
    "FirstDocN - LastDocN is DocId(s) of requiered doc(s),\n"
    "if LastDocN omitted, only one FirstDocN will be printed,\n"
    "if FirstDocN omitted, all docs will be printed\n"
    ;
}

template<typename ArcIterType>
static void Iterate(ArcIterType& arcIter, TProgramTArcViewDocIterator& docIterator, const TProgramTArcViewOptions& options) {
    TArchiveHeader* hdr = nullptr;
    ui32 curDocumentNumber = 0;
    while (docIterator.CurDoc() != docIterator.End()) {
        // in case of absence of previous and iterator already at next
        if (!options.readFromInput && hdr && hdr->DocId >= docIterator.CurDoc()) {
            docIterator.Next();
            continue;
        }

        if (options.SequentialInput && docIterator.CurDoc() >= arcIter.Size()) {
            break;
        }

        // near 50
        if (!options.SequentialInput && hdr && hdr->DocId < docIterator.CurDoc() && hdr->DocId + 50 > docIterator.CurDoc()) {
            // iterating to needed
            for(ui32 i = hdr->DocId ; i < docIterator.CurDoc() ; ) {
                hdr = arcIter.NextAuto();
                if (!hdr) {
                    ythrow yexception() << "Can't iterate to next record. May be you entered wrong interval?";
                }
                i = hdr->DocId;
            }
            // if record is absent
            if (hdr->DocId != docIterator.CurDoc()) {
                if (options.readFromInput) {
                    docIterator.Next();
                    continue;
                }
            }
        } else {
            // first record must exist
            if (!hdr) {
                hdr = arcIter.SeekToDoc(docIterator.CurDoc());
            } else {
                // others may absent
                IArchiveIterator::TArcErrorPtr err;
                if (TArchiveHeader* newHdr = arcIter.SeekToDoc(docIterator.CurDoc(), err)) {
                    hdr = newHdr;
                } else {
                    if (!!err && err->Error == IArchiveIterator::DocIdMismatch)
                        Cerr << "TArcView: " << err->Message.Str() << Endl;

                    docIterator.Next();
                    continue;
                }
            }
        }

        ++curDocumentNumber;
        if (options.DocumentNumber == 0 || curDocumentNumber == options.DocumentNumber) {
            PrintDoc(options, Cout, arcIter, hdr);

            if (options.DocumentNumber != 0) {
                break;
            }
        }
        docIterator.Next();
    }
}

void main_except_wrap (int argc, char* argv[])
{
#ifdef _win32_
  _setmode(_fileno(stdout), _O_BINARY);
#endif

    TProgramTArcViewOptions options;

    int optlet;
    Opt opt(argc, argv, "azecdhilsxmwuto:k:fD:S");
    while ((optlet = opt.Get()) != EOF) {
        switch (optlet) {
        case 'e':
            options.ExternalInfo = true;
            break;
        case 'c':
            options.CheckOnly = true;
            break;
        case 'm':
            options.MarkupZone = true;
            break;
        case 'z':
            options.MarkupZoneAttrs = true;
            break;
        case 'w':
            options.WeightZone = true;
            break;
        case 'h':
            options.FullArchive = true;
            break;
        case 'x':
            options.NeedExtended = true;
            break;
        case 'i':
            options.readFromInput = true;
            break;
        case 'S':
            options.readFromInput = true;
            options.SequentialInput = true;
            break;
        case 'u':
            options.urlDumpFormat = true;
            break;
        case 'd':
            options.Converted2Html = true;
            break;
        case 'a':
            options.SentAttrs = true;
            break;
        case 's':
            options.DecodeSentAttrs = true;
            break;
        case 't':
            options.TitleAsAttr = true;
            break;
        case 'l':
            options.AnchorLinks = true;
            break;
        case 'o':
            if (freopen(opt.Arg, "wb", stdout) == nullptr) {
                ythrow yexception() << "Can't open " <<  opt.Arg << ": " << LastSystemErrorText();
            }
            break;
        case 'k':
            options.DocumentNumber = FromString<ui32>(opt.Arg);
            break;
        case 'f':
            options.CompressedExtBlob = true;
            break;
        case 'D':
            options.DocumentId = FromString<ui32>(opt.Arg);
            options.DirectReadMode = true;
            break;
        case '?':
        default:
            PrintUsage();
            return;
            break;
        }
    }

    int n = argc - opt.Ind - 1;
    if (n < 0 || n > 2) {
        PrintUsage();
        return;
    }

    TString arcName(argv[opt.Ind]);

    if (options.DirectReadMode) {
        PrintDocumentByDocid(arcName, options, Cout);
        return;
    }

    TProgramTArcViewDocIterator docIterator(argc, argv, opt.Ind, options);
    size_t bufSize = 32 << 20;
    if (docIterator.CurDoc() != docIterator.End())
        bufSize = 50000;

    if (options.FullArchive) {
        TFullArchiveIterator arcIter(bufSize);
        arcIter.Open((arcName + "tag").data());
        if (docIterator.CurDoc() == docIterator.End())
            PrintFullArchive(options, Cout, arcIter);
        else {
            arcIter.OpenTdr( (arcName + "tdr").data());
            Iterate(arcIter, docIterator, options);
        }
    } else if (NFs::Exists(arcName + ".chunked_text.mapping.wad")) {
        NDoom::TDocChunkMappingSearcher docChunkMappingSearcher(arcName + ".chunked_text.mapping.wad");
        TVector<THolder<TArchiveIterator>> arcIters;
        for (ui32 chunk = 0; ; ++chunk) {
            const TString chunkPrefix = TStringBuilder() << arcName << "." << chunk << ".";
            if (!NFs::Exists(chunkPrefix + "arc")) {
                break;
            }
            arcIters.push_back(MakeHolder<TArchiveIterator>());
            arcIters.back()->Open((chunkPrefix + "arc").data());
        }
        while (docIterator.CurDoc() != docIterator.End()) {
            if (docIterator.CurDoc() >= docChunkMappingSearcher.Size()) {
                break;
            }
            const NDoom::TDocChunkMapping mapping = docChunkMappingSearcher.Find(docIterator.CurDoc());
            IArchiveIterator::TArcErrorPtr error;
            TArchiveHeader* header = arcIters[mapping.Chunk()]->SeekToDoc(mapping.LocalDocId(), error);
            Y_ENSURE(!error);
            PrintDoc(options, Cout, *arcIters[mapping.Chunk()], header);
            docIterator.Next();
        }
    } else {
        TArchiveIterator arcIter(bufSize);
        arcIter.Open((arcName + "arc").data());
        if (docIterator.CurDoc() == docIterator.End())
            PrintArchive(options, Cout, arcIter);
        else {
            arcIter.OpenDir((arcName + "dir").data());
            Iterate(arcIter, docIterator, options);
        }
    }
}

int main(int argc, char *argv[]) {
   try {
      main_except_wrap(argc, argv);
   } catch (std::exception& e) {
      Cerr << "Error: " << e.what() << Endl;
      return 1;
   }
   return 0;
}
