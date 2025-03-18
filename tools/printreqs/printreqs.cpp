#include <kernel/qtree/request/printreq.h>
#include <kernel/qtree/request/req_node.h>
#include <kernel/reqerror/reqerror.h>
#include <kernel/qtree/request/request.h>

#include <library/cpp/getopt/opt.h>

#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/system/datetime.h>
#include <util/system/defaults.h>

#include <cassert>
#include <cerrno>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#define  OemToMaxim OemToAnsi
#define  MaximToOem AnsiToOem

#ifdef YYDEBUG
extern int yydebug;
#endif

IOutputStream& operator<< (IOutputStream &os, const TRequestNode *n);

namespace
{
    const char *iname = "test_req.txt";

    void PrintResult(TFileInput& inf, IOutputStream& outf, bool measureTime, ui32 parserFlags) {
        TString sReq;
        ui64 totalTime = 0;
        while (inf.ReadLine(sReq)) {
            outf << sReq << '\n';
#ifdef YYDEBUG
            if (yydebug == 1)
                Cerr << sReq << Endl;
#endif
            if (sReq[0] != ':') {
                try {
                    TString p;
                    const ui64 start = MicroSeconds();
                    THolder<TRequestNode> root(tRequest(parserFlags).Parse(UTF8ToWide(sReq)));
                    totalTime += MicroSeconds() - start;
                    if (root.Get())
                        PrintRequest(*root, p);
                    outf << p << "\n" << ((const TRequestNode*) root.Get()) << "\n";
                } catch (const TError& e) {
                    outf << '\n' << e.what();
                }
                outf << "\n\n";
            }
        }
        if (measureTime)
            Cerr << "Total time of parsing: " << totalTime << " mcsec" << Endl;
    }

    void PrintUsage() {
        Cerr << "usage: printreqs [options] [<infile>]\n";
        Cerr << "Available options:\n";
        Cerr << "  -W --wizard : wizard compatible mode\n";
        Cerr << "  -t          : measure total time of parsing\n";
        Cerr << "  -x          : enable extended syntax\n";
        Cerr << "  -p          : print lots of debug information to strerr\n";
        Cerr << "  <infile>    : a file with requests" << Endl;
    }
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
  _setmode(_fileno(stdout), _O_BINARY);
#endif

#ifdef YYDEBUG
  yydebug = 0;
#endif

  Opt::Ion wizardParam = {"wizard", Opt::WithoutArg, nullptr, int('W')};
  Opt::Ion lastParam   = {nullptr, Opt::WithoutArg, nullptr, 0};

  Opt::Ion longParams[2] = {wizardParam, lastParam};

  class Opt opt(argc, argv, "pto:x", &longParams[0]);
  ui32 parserFlags = RPF_DEFAULT;
  bool measureTime = false;
  int c;
  while ((c = opt.Get()) != EOF) {
     switch (c) {
     case 'W':
        parserFlags |= RPF_USE_TOKEN_CLASSIFIER | RPF_TRIM_EXTRA_TOKENS;
        break;
     case 't':
        measureTime = true;
        break;
     case 'p':
#ifdef YYDEBUG
        yydebug = 1;
#else
        Cerr << "use option -p only for debug version" << Endl;
        return EXIT_FAILURE;
#endif
        break;
    case 'x':
        parserFlags |= RPF_ENABLE_EXTENDED_SYNTAX;
        break;
     case '?':
     default:
        PrintUsage();
        return EXIT_FAILURE;
     }
  }

  if (opt.Ind < argc) {
     iname = argv[opt.Ind];
     opt.Ind++;
  }

  try {
      TFileInput inf(iname);

      if (opt.Ind < argc) {
         TFixedBufferFileOutput outf(argv[opt.Ind]);
         PrintResult(inf, outf, measureTime, parserFlags);
         opt.Ind++;
      } else {
         PrintResult(inf, Cout, measureTime, parserFlags);
      }
  } catch (const std::exception& e) {
      Cerr << "exception: " << e.what() << Endl;
      return EXIT_FAILURE;
  }

  if (opt.Ind < argc) {
     Cerr << "extra arguments ignored: " << argv[opt.Ind] << "..." << Endl;
  }

  return 0;
}
