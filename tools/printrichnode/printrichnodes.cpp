#include <kernel/search_daemon_iface/reqtypes.h>

#include <ysite/yandex/pure/pure.h>
#include <kernel/qtree/request/req_node.h>
#include <kernel/reqerror/reqerror.h>
#include <kernel/qtree/request/request.h>
#include <kernel/qtree/request/request.h>
#include <kernel/qtree/richrequest/loadfreq.h>
#include <kernel/qtree/richrequest/nodeiterator.h>
#include <kernel/qtree/richrequest/printrichnode.h>
#include <kernel/qtree/richrequest/proxim.h>
#include <kernel/qtree/richrequest/richnode.h>

#include <library/cpp/getopt/opt.h>
#include <kernel/lemmer/core/langcontext.h>
#include <kernel/lemmer/fixlist_load/from_text.h>

#include <library/cpp/charset/recyr.hh>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/system/defaults.h>

#include <cassert>
#include <cerrno>

#ifdef _win32_
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

bool CheckCopy(const TRichTreePtr& tree) {
    TRichTreePtr copy(tree->Copy());
    return (copy->Compare(tree.Get()));
}

void PrintResult(IInputStream* inf,
                 IOutputStream & outf,
                 const TLanguageContext& lang,
                 ECharset encoding,
                 bool checkSerialization,
                 bool updateTree,
                 bool basicTree,
                 const TPure& pure,
                 ui32 processingFlags,
                 size_t tokenizerVersion)
{
    TString sReq;
    while (inf->ReadLine(sReq)){
        outf << sReq << '\n';
        sReq = Recode(encoding, CODES_UTF8, sReq);
        if (sReq[0] != ':') {
            try {
                TCreateTreeOptions options(lang, processingFlags);
                options.TokenizerVersion = tokenizerVersion;
                TRichTreePtr richTree(CreateRichTree(UTF8ToWide(sReq), options));
                if (!richTree.Get() || sReq.empty())
                    continue;

                try {
                    richTree->Root->VerifyConsistency();
                } catch (const yexception& e){
                    outf << "VerifyTree failed (original): " << e.what() << Endl;
                }

                if (pure.Loaded()) {
                    LoadFreq(pure, *richTree->Root);
                }

                if (updateTree) {
                    UpdateRichTree(richTree->Root);
                    try {
                        richTree->Root->VerifyConsistency();
                    } catch (const yexception& e){
                        outf << "VerifyTree failed (updated): " << e.what() << Endl;
                    }
                }

                if (basicTree) {
                    PrintBasicRichTree(richTree.Get(), outf);
                    outf << Endl;
                } else if (!checkSerialization) {
                    TUtf16String p = PrintRichRequest(richTree.Get());
                    outf << p << Endl << Endl;
                    PrintRichTree(richTree.Get(), outf);
                    outf << Endl;
                } else {
                    TBinaryRichTree buffer;
                    TRichTreePtr binaryClone;
                    richTree->Serialize(buffer);
                    binaryClone = DeserializeRichTree(buffer);

                    if (!richTree->Compare(binaryClone.Get()) )
                        outf << Endl << "Binary serialization check failed!";

                    TString text;
                    richTree->SerializeToClearText(text);
                    TRichTreePtr textClone(new NSearchQuery::TRequest);
                    textClone->DeserializeFromClearText(text);

                    outf << Endl << text;

                    if (!richTree->Compare(textClone.Get()) )
                        outf << Endl << "Text serialization check failed!";
                    if (!CheckCopy(richTree))
                        outf << Endl << "Copying check failed!";
                }
            } catch (const TError& e){
                outf << Endl << e.what();
            } catch (const yexception& e){
                outf << Endl << e.what();
            }
            outf << Endl << Endl;
        }
    }
}

void Usage() {
    Cout << "Usage: printrichnode [options] [iname=test_req.txt [oname=out_req.txt]]" << Endl;
    Cout << "Options:" << Endl;
    Cout << "    -h, -?            Print this text" << Endl;
    Cout << "    -s                Check serialization" << Endl;
    Cout << "    -b                Print basic tree" << Endl;
    Cout << "    -u                Update tree before serialization" << Endl;
    Cout << "    -e <encoding>     Encoding" << Endl;
    Cout << "    -o <errfile>      Errors file" << Endl;
    Cout << "    -l <languages>    Lemmatization languages" << Endl;
    Cout << "    -w <stopwords>    Stopwords file" << Endl;
    Cout << "    -f <fixlist>      Fixlist file" << Endl;
    Cout << "    -d <decimator>    Decimator file" << Endl;
    Cout << "    -t <pure trie>    Pure trie location" << Endl;
    Cout << "    -z                Use new tokenization" << Endl;
    Cout << "    -W --wizard       Wizard compatible mode" << Endl;
    Cout << "    -x                Enable extended syntax" << Endl;
    Cout << "    -v                Tokenizer version" << Endl;
}

int main(int argc, char* argv[]) {
#ifdef _win32_
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    _setmode(_fileno(stdout), _O_BINARY);
#endif // _win32_

    try {
        const char* iname = nullptr;

        Opt::Ion wizardParam = {"wizard", Opt::WithoutArg, nullptr, int('W')};
        Opt::Ion lastParam   = {nullptr, Opt::WithoutArg, nullptr, 0};

        Opt::Ion longParams[2] = {wizardParam, lastParam};

        class Opt opt(argc, argv, "?hbd:e:f:l:o:w:v:sut:zx", &longParams[0]);
        int c = 0;

        bool checkSerialization = false;
        bool updateTree = false;
        bool basicTree = false;

        ui32 processingFlags = RPF_DEFAULT;

        TWordFilter stopwords;
        TFormDecimator decimator;
        ECharset encoding = CODES_UTF8;
        TPure pure;
        TLangMask lang(LI_DEFAULT_REQUEST_LANGUAGES);
        size_t tokenizerVersion = NTokenizerVersionsInfo::Default;

        while ((c = opt.Get()) != EOF) {
            switch (c){
                case 'b':
                    basicTree = true;
                    break;
                case 'e':
                    encoding = CharsetByName(opt.Arg);
                    break;
                case 'o':
                    if (freopen(opt.Arg, "w", stderr) == nullptr) {
                        perror(opt.Arg);
                        return EXIT_FAILURE;
                    }
                    break;
                case 's':
                    checkSerialization = true;
                    break;
                case 'u':
                    updateTree = true;
                    break;
                case 't':
                    pure.Init(opt.Arg);
                    break;
                case 'l':
                    lang = NLanguageMasks::CreateFromList(opt.Arg);
                    break;
                case 'f':
                    NLemmer::SetMorphFixList(opt.Arg);
                    break;
                case 'd':
                    decimator.InitDecimator(opt.Arg);
                    break;
                case 'w':
                    stopwords.InitStopWordsList(opt.Arg);
                    break;
                case 'z':
                    break;
                case 'W':
                    checkSerialization = true;
                    updateTree = true;
                    processingFlags |= RPF_USE_TOKEN_CLASSIFIER | RPF_TRIM_EXTRA_TOKENS;
                    break;
                case 'x':
                    processingFlags |= RPF_ENABLE_EXTENDED_SYNTAX;
                    break;
                case '?':
                case 'h':
                    Usage();
                    return 0;
                case 'v':
                    tokenizerVersion = FromString<size_t>(opt.Arg);
                    break;
                default:
                    Usage();
                    return EXIT_FAILURE;
            }
        }

        if (opt.Ind < argc){
            iname = argv[opt.Ind];
            opt.Ind++;
        }

        TLanguageContext langcontext(lang, nullptr, stopwords);
        langcontext.SetDecimator(decimator);
        THolder<TFileInput> inf(iname ? new TFileInput(iname) : nullptr);

        if (opt.Ind < argc){
            TFixedBufferFileOutput outf(argv[opt.Ind]);
            PrintResult(inf.Get() ? inf.Get() : &Cin,
                        outf,
                        langcontext,
                        encoding,
                        checkSerialization,
                        updateTree,
                        basicTree,
                        pure,
                        processingFlags,
                        tokenizerVersion);
            opt.Ind++;
        } else {
            PrintResult(inf.Get() ? inf.Get() : &Cin,
                        Cout,
                        langcontext,
                        encoding,
                        checkSerialization,
                        updateTree,
                        basicTree,
                        pure,
                        processingFlags,
                        tokenizerVersion);
        }

        if (opt.Ind < argc) {
            Cerr << "extra arguments ignored: " << argv[opt.Ind] << "..." << Endl;
        }

        return 0;

    } catch (const std::exception& e) {
        Cerr << e.what() << Endl;
    }

    return 1;
}
