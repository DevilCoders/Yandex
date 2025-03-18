#include <dict/tomaparser/syntprocessor.h>
#include <dict/tomaparser/synttreeformat.h>
#include <dict/tomaparser/tomacompiler.h>

#include <kernel/qtree/richrequest/richnode.h>

#include <library/cpp/getopt/opt.h>
#include <kernel/lemmer/core/language.h>

#include <library/cpp/charset/recyr.hh>
#include <util/datetime/cputimer.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/vector.h>
#include <util/system/defaults.h>

#if defined(_win_)
    #define WIN32_LEAN_AND_MEAN
    #include <io.h>
    #include <fcntl.h>
#include <util/string/split.h>
#endif

static inline void Usage(const char* prog) {
    Cerr << "usage: " << prog << " [options]... [file...]" << Endl
    << "write result of syntactic parsing of all file(s) to stdout" << Endl
    << "options:" << Endl
    << "   -g<file>     grammar file" << Endl
    << "   -G<file>     compiled grammar file" << Endl
    << "   -E<expression>     grammar expression" << Endl
    << "   -b           only compile grammar" << Endl
    << "   -o<file>     file for saving results instead of stdout" << Endl
    << "   -e<encoding> encoding to be used for loading and saving files" << Endl
    << "   -l<langlist> list of languages to parse" << Endl
    << "   -r<symbol>   root symbol of grammar" << Endl
    << "   -w           print warnings during compilation" << Endl
    << "   -f<number>   1-based index of field to process" << Endl
    << "   -c<number>   cutoff frequency" << Endl
    << "   -F<number>   cutoff field" << Endl
    << "   -t<string>   output types" << Endl
    << "   -k<format>   format specification" << Endl
    << "   -h           display this help and exit" << Endl;
}

static inline TAutoPtr<IOutputStream> OpenOutput(const TString& path) {
    if (path.empty() || path == "-") {
        return new TFixedBufferFileOutput(Duplicate(1));
    } else {
        return new TFixedBufferFileOutput(path);
    }
}

TLangMask ParseLanguageList(const TString& list) {
    TLangMask parsed;
    const char* language_separators = ",:; ";
    size_t pos = 0, endpos = 0;
    while (pos < list.length()) {
        pos = list.find_first_not_of(language_separators, endpos);
        if (pos == TString::npos) break;
        endpos = list.find_first_of(language_separators, pos);
        if (endpos == TString::npos) endpos = list.length();

        TString langname = list.substr(pos, endpos - pos);
        const TLanguage* lang = NLemmer::GetLanguageByName(langname.c_str());
        if (lang)
           parsed.SafeSet(lang->Id);
        else
           Cerr << "Unrecognized language name " << langname << " - ignored";
    }
    return parsed;
}


struct TOpts {
    inline TOpts(int argc, char** argv)
        : OutputFile("-")
        , Encoding(CODES_YANDEX)
        , LangMask(LANG_RUS)
        , RootSymbol("Z")
        , PrintWarnings(true)
        , Field(0)
        , Cutoff(0)
        , CutoffField(0)
        , CompileGrammar(false) {
            Opt opt(argc, argv, "g:G:E:bo:e:l:r:wf:c:F:t:k:h?");
        int optlet;
        while (EOF != (optlet = opt.Get())) {
            switch (optlet) {
                case 'g':
                    GrammarFile = opt.GetArg();
                    break;
                case 'G':
                    CompiledGrammarFile = opt.GetArg();
                    break;
                case 'E':
                    Expression = opt.GetArg();
                    break;
                case 'b':
                    CompileGrammar = true;
                    break;
                case 'o':
                    OutputFile = opt.GetArg();
                    break;
                case 'e':
                    Encoding = CharsetByName(opt.GetArg());
                    break;
                case 'l':
                    LangMask = ParseLanguageList(opt.GetArg());
                    break;
                case 'r':
                    RootSymbol = opt.GetArg();
                    break;
                case 'w':
                    PrintWarnings = true;
                    break;
                case 'f':
                    Field = FromString<int>(opt.GetArg());
                    break;
                case 'c':
                    Cutoff = FromString<int>(opt.GetArg());
                    break;
                case 'F':
                    CutoffField = FromString<int>(opt.GetArg());
                    break;
                case 't':
                    StringSplitter(opt.GetArg()).Split(':').SkipEmpty().Collect(&Types);
                    break;
                case 'k':
                    Formats.push_back(opt.GetArg());
                    break;

                case '?':
                case 'h':
                    Usage(argv[0]);
                    exit(0);
                    break;

                default:
                    ythrow yexception() << "unknown cmd option(" <<  opt.GetArg() << ")";
            }
        }
        if (Formats.size() == 0)
            Formats.push_back("21");
        for (int i = 0; i < argc - opt.Ind; ++i) {
            InputFiles.push_back((argv + opt.Ind)[i]);
        }
        if (GrammarFile.empty() && Expression.empty() && CompiledGrammarFile.empty() && !CompileGrammar)
            ythrow yexception() << "one of the options(" <<  'g' << ", " <<  'E' << " or " <<  'G' << ") are required";
    }
    TString OutputFile;
    ECharset Encoding;
    TLangMask LangMask;
    TString RootSymbol;
    bool PrintWarnings;
    int Field;
    int Cutoff;
    int CutoffField;
    TVector<TString> Types;
    TVector<TString> Formats;
    TString Expression;

    TString GrammarFile;
    TString CompiledGrammarFile;
    TVector<TString> InputFiles;
    bool CompileGrammar;
};

class TProgress {
public:
    TProgress(IOutputStream* out, ui32 scale = 100)
        : mOut(out)
        , mScale(scale)
        , mCurrent(0) {}

    ~TProgress() {}

    TProgress& operator= (ui64 newCurrent) {
        ChangeProgress(newCurrent);
        return *this;
    }

    TProgress& operator+= (ui64 newCurrent) {
        ChangeProgress(mCurrent + newCurrent);
        return *this;
    }

    TProgress& operator++() {
        ChangeProgress(mCurrent + 1);
        return *this;
    }

private:
    void ChangeProgress(ui64 newCurrent) {
        mCurrent = newCurrent;
        if ((mCurrent % mScale) == 0)
            (*mOut << mCurrent << "\r").Flush();
    }

private:
    IOutputStream* mOut;
    const ui32 mScale;
    ui64 mCurrent;
};

class TMain {

public:
    inline TMain(const TOpts* opts)
        : mOpts(opts)
        , mOut(OpenOutput(opts->OutputFile)) {
            Init();
    }

    inline ~TMain() {

    }

    inline void Init() {
        if (mOpts->CompileGrammar)
            return;
        for (int i = 0; i < mOpts->Formats.ysize(); ++i) {
            TString format = mOpts->Formats[i];
            int treeformat = format[0] - 48;
            ETerminalFormat terminalformat = static_cast<ETerminalFormat>(format[1] - 48);
            TTreeFormatter* formatter = nullptr;
            if (treeformat == 1)
                formatter = new TFlatFormatter(mOut.Get(), mOpts->Types, terminalformat);
            else if (treeformat == 2)
                formatter = new TFullFormatter(mOut.Get(), terminalformat);
            else if (treeformat == 3)
                formatter = new TSubTreeFormatter(mOut.Get(), terminalformat);
            mFormatters.push_back(formatter);
        }
    }

    inline void Run() {
        if (mOpts->CompileGrammar) {
            CompileGrammar();
            return;
        }
        LoadGrammar();
        if (mOpts->InputFiles.ysize() > 0) {
            TVector<TString>::const_iterator it(mOpts->InputFiles.begin()), end(mOpts->InputFiles.end());
            for (; it != end; ++it) {
                TFileInput in(*it);
                Process(&in);
            }
        } else {
            Process(&Cin);
        }
    }

    void CompileGrammar() {
        THolder<IInputStream> rin;
        THolder<IOutputStream> rout;

        IInputStream* in = &Cin;
        IOutputStream* out = &Cout;

        if (mOpts->InputFiles.size() > 0) {
            rin.Reset(new TFileInput(mOpts->InputFiles[0]));
            in = rin.Get();
        }

        if (mOpts->InputFiles.size() > 1) {
            rout.Reset(new TFixedBufferFileOutput(mOpts->InputFiles[1]));
            out = rout.Get();
        }
        TTomaCompiler compiler;
        compiler.Compile(in, out);
    }

    inline void LoadGrammar() {
        if (!mOpts->CompiledGrammarFile.empty()) {
            mProcessor.Reset(new TSyntProcessor(mOpts->CompiledGrammarFile, mOpts->LangMask));
        } else {
            THolder<IInputStream> input;
            if (!mOpts->GrammarFile.empty()) {
                input.Reset(new TFileInput(mOpts->GrammarFile));
            } else {
                TStringInput in(mOpts->Expression);
                input.Reset(new TStringInput(mOpts->Expression));
            }
            mProcessor.Reset(new TSyntProcessor(input.Get(), mOpts->RootSymbol, mOpts->LangMask));
        }
    }

    inline void Process(IInputStream* in) {
        TString line;
        TLanguageContext ctx;
        ctx.SetLangMask(mOpts->LangMask);
        TProgress progress(&Cerr);
        while (in->ReadLine(line)) {
            *mOut << line;
            TVector<TString> data;
            TSyntTree tree;
            if (line.empty() || line.StartsWith('#')) {
                *mOut << Endl;
                continue;
            }
            if (mOpts->Field || mOpts->CutoffField) {
                StringSplitter(line).Split('\t').SkipEmpty().Collect(&data);
            }
            if (mOpts->CutoffField && mOpts->Cutoff) {
                int f = FromString<int>(data[mOpts->CutoffField-1]);
                if (f < mOpts->Cutoff) {
                    continue;
                }
            }
            data.insert(data.begin(), line);
            int coverage = 0;
            int status = 0;
            int depth = 0;
            double  quality = 0.0;
            TRichNodePtr node;
            try {
                node = CreateRichNode(UTF8ToWide(data[mOpts->Field]), TCreateTreeOptions(ctx));
                if (node.Get())
                    coverage = mProcessor->Process(node.Get(), &tree);
            } catch (...) {
                *mOut << "\t" << status << "\t" << coverage << "\t" << depth << "\t" << quality << "\t" << "-" << Endl;
                continue;
            }
            if (tree.size() > 0) {
                TSyntNode* root = tree[0].Get();
                if (root->Children.size() == 1)
                    status = 1;
                depth = root->GetDepth();
                quality = root->Quality();
            }
            *mOut << "\t" << status << "\t" << coverage << "\t" << depth << "\t" << Sprintf("%.6f", quality) << "\t";
            for (size_t i = 0; i < mFormatters.size(); ++i) {
                if (tree.size() > 0) {
                    TSyntNode* root = tree[0].Get();
                    mFormatters[i]->Format(root);
                } else {
                    *mOut << "-";
                }
                *mOut << "\t";
            }
            *mOut << Endl;
            ++progress;
        }
    }

private:
    const TOpts* mOpts;
    THolder<IOutputStream> mOut;
    THolder<TSyntProcessor> mProcessor;
    THolder<TTomaCompiler> mCompiler;
    TVector<TTreeFormatter*> mFormatters;
};

using namespace NSpike;

int main(int argc, char** argv) {
#ifdef _win_
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    try {
        THolder<TOpts> opts;
        try {
            opts.Reset(new TOpts(argc, argv));
        } catch (...) {
            Usage(argv[0]);
            throw;
        }
        THolder<TMain> m(new TMain(opts.Get()));
        m->Run();
        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
    }
    return 1;
}

