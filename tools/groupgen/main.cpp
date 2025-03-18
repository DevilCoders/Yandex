#include <dict/tomaparser/generator.h>
#include <dict/tomaparser/syntprocessor.h>

#include <kernel/qtree/richrequest/nodeiterator.h>

#include <library/cpp/charset/wide.h>
#include <library/cpp/getopt/opt.h>

#include <library/cpp/charset/recyr.hh>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/vector.h>
#include <util/system/defaults.h>

#ifdef _win32_
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static inline void Usage(const char* prog) {
    Cerr << "usage: " << prog << " [options]... [file...]" << Endl
    << "generate groups" << Endl
    << "options:" << Endl
    << "  -g  path to grammar" << Endl
    << "  -p  form to generate (can be specified sveral times), e.g. 'род,мн'" << Endl
    << "  -h  display this help and exit" << Endl;
}

struct TOpts {
    TOpts(int argc, const char* argv[])
        : Input(&Cin)
        , Output(&Cout)
    {
        Opt opt(argc, argv, "g:e:p:h?");
        OPTION_HANDLING_PROLOG;
        OPTION_HANDLE('g', (Grammar = opt.GetArg()));
        OPTION_HANDLE('e', (Encoding = opt.GetArg()));
        OPTION_HANDLE('p', (Grams.push_back(opt.GetArg())));
        OPTION_HANDLE('h', (Usage(argv[0]),exit(0)));
        OPTION_HANDLE('?', (Usage(argv[0]),exit(0)));
        OPTION_HANDLING_EPILOG;

        if (argc - opt.Ind > 0) {
            in.Reset(new TFileInput((argv + opt.Ind)[0]));
            Input = in.Get();
        }
        if (argc - opt.Ind > 1) {
            out.Reset(new TFixedBufferFileOutput((argv + opt.Ind)[1]));
            Output = out.Get();
        }
        if (!Grammar)
            ythrow yexception() << "no grammar specified";

        if (!Grams.size())
            ythrow yexception() << "at least one form should be specified";

    }
    TString Grammar;
    TString Encoding;
    TVector<TString> Grams;
    IInputStream* Input;
    IOutputStream* Output;
    THolder<IInputStream> in;
    THolder<IOutputStream> out;
};

class TMain {
private:
    const TOpts* Opts;
public:
    TMain(const TOpts* opts)
        : Opts(opts) {
    }

    void Run() {
        TSyntProcessor processor(Opts->Grammar, "Z");
        TSyntGenerator generator;
        TString line;
        while (Opts->Input->ReadLine(line)) {
            Cout << line << Endl;
            TString prefix = line.substr(0, line.find('|'));
            TRichNodePtr node = CreateRichNode(CharToWide(prefix, csYandex), TCreateTreeOptions(TLangMask(LANG_RUS)));
            if (node.Get()) {
                TConstNodesVector words;
                GetChildNodes(*node, words, IsWord);
                TSyntNode* root = processor.Process(words);
                if (root) {
                    TSyntProcessor::PrintTree(root, &Cout);
                    Cout << Endl << root->Children.size() << Endl;
                    for (size_t i = 0; i < Opts->Grams.size(); ++i) {
                        TGramBitSet form = TGramBitSet::FromString(Opts->Grams[i]);
                        for (size_t j = 0; j < root->Children.size(); ++j) {
                            TString s = generator.Generate(root->Children[j].Get(),form);
                            if (j)
                                Cout << " | ";
                            Cout << s;
                        }
                        Cout << Endl;
                    }
                }
            } else {
                Cout << "-" << Endl;
            }
            Cout << Endl;
        }
    }

};

int main(int argc, const char* argv[]) {
#ifdef _win32_
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
#endif // _win32_
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

