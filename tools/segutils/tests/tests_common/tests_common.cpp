#include "tests_common.h"

using namespace NSegm;

namespace NSegutils {

void TTest::Init(int argc, const char** argv) {
    if(argc < 2 || strcmp(argv[1], "--help") == 0)
        UsageAndExit(argv[0], 0);

    TString args = "h:d:l:c:f:m:s" + GetArgs();
    Opt opts(argc, argv, args.data());
    int optlet;

    TString htparser_ini, dict_dict, _2ld_list, confdir, filedir, filelist, printonly;
    while (EOF != (optlet = opts.Get())) {
        switch (optlet) {
            case 'h':
                htparser_ini = opts.GetArg();
                break;
            case 'd':
                dict_dict = opts.GetArg();
                break;
            case 'l':
                _2ld_list = opts.GetArg();
                break;
            case 'c':
                confdir = opts.GetArg();
                break;
            case 'f':
                filedir = opts.GetArg();
                break;
            case 'm':
                filelist = opts.GetArg();
                break;
            case 's':
                printonly = opts.GetArg();
                break;
            default:
                if (!ProcessArg(optlet, opts))
                    UsageAndExit(argv[0], 1);
        }
    }

    if (!!confdir)
        Ctx.Reset(new TParserContext(confdir));
    else if (!!htparser_ini) {
        Ctx.Reset(new TParserContext(htparser_ini, dict_dict, _2ld_list));
    }

    if (!Ctx) {
        Clog << "Bad parser config" << Endl;
        UsageAndExit(argv[0], 1);
    }

    Ctx->SetUriFilterOff();

    Reader.SetDirectory(filedir);

    if (!filelist)
        Reader.InitMapping(Cin);
    else
        Reader.InitMapping(filelist);

    if (!!printonly) {
        Reader.FileList.clear();
        Reader.FileList.push_back(printonly);
    }

    Reader.FileNameColumn = 0;
    Reader.MetaData.Url = 1;
    Reader.MetaData.RealDate = -1;

    DoInit();
}

void TTest::RunTest() {
    for (TVector<TString>::const_iterator it = Reader.FileList.begin(); it != Reader.FileList.end(); ++it) {
        try {
            Cout << ProcessDoc(Reader.Read(*it));
            //TString fName = *it;
            //Cout << fName << Endl;
            //ProcessDoc(Reader.Read(fName));
        } catch (const yexception& e) {
            Cout << "Failed to process " << (Reader.CommonPrefix + *it) << " because " << e.what() <<Endl;
            continue;
        }
    }
}

void TTest::UsageAndExit(const char* me, int code) {
    Clog << "Usage: " << me << " (-h <htparser.ini> -d <dict.dict> -l <2ld.list> | -c <confdir>) -f <filedir> "
                        "[-m <filelist>] [-s <onlythisfile>] [-5]"
                    << GetHelp() << Endl;
    exit(code);
}

struct TPrintContext {
    TUtf16String Buffer;
    IOutputStream* Out;

    TPrintContext(IOutputStream* out)
        : Out(out)
    {}

    ~TPrintContext() {
        Flush(false, 0);
    }

    void OnEvent(const TSegEvent* ev) {
        if (ev->IsA<SET_SPACE> () && !ev->Text /* fake space */) {
            Buffer.append(' ');
        } else if (ev->IsA<SET_TOKEN> () || ev->IsA<SET_SPACE> ()) {
            Buffer.append(ev->Text);
        }

        if (ev->IsA<SET_PARABRK> () || ev->IsOnly<SET_SPAN_BEGIN>() || ev->IsOnly<SET_SPAN_END>()) {
            Flush(ev->IsA<SET_PARABRK>(), ev->Pos);

            if (ev->IsA<SET_SPAN_BEGIN> ()) {
                (*Out) << "<"  << ev->Attr;
                if (!!ev->Text)
                    (*Out) << "\n" << ev->Text << "\n";
                (*Out) << ">\n";
            } else if (ev->IsA<SET_SPAN_END> ()) {
                (*Out) << "</" << ev->Attr << ">\n";
            }
        }
    }

    void Flush(bool, TAlignedPosting) {
        if (!IsSpace(Buffer.data(), Buffer.size())) {
            Collapse(Buffer);

            (*Out) << Buffer << "\n";
        }

        Buffer.clear();
    }
};

void TTest::PrintEvents(const TEventStorage& st, IOutputStream& out) {
    TPrintContext ctx(&out);
    for (TEventStorage::const_iterator it = st.begin(); it != st.end(); ++it)
        ctx.OnEvent(it);
}


}
