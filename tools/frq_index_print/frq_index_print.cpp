/// author@ cheusov@ Aleksey Cheusov
/// created: Sun, 12 Oct 2014 15:43:33 +0300
/// see: OXYGEN-897

#include <ysite/yandex/common/ylens.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/stream/format.h>
#include <util/stream/output.h>

int main(int argc, char **argv)
{
    NLastGetopt::TOpts opts;
    opts.AddHelpOption();
    opts.AddLongOption("docid")
        .RequiredArgument("DOCID")
        .Help("docid to print");
    opts.AddLongOption("print-hex")
        .NoArgument()
        .Help("print hex values");

    opts.SetFreeArgsNum(1);
    opts.SetFreeArgTitle(0, "INDEXFILE", "path to indexfrq file");

    NLastGetopt::TOptsParseResult args(&opts, argc, argv);

    const TDocsMaxFreqs indexfrq(args.GetFreeArgs()[0].data());

    ui32 begin, end;

    if (args.Has("docid")) {
        begin = args.Get<ui32>("docid");
        end = begin + 1;
    } else {
        begin = 0;
        end = indexfrq.GetSize();
        Cout << "Count:" << end << Endl;
    }

    const bool hex = args.Has("print-hex");
    for (; begin != end; ++begin){
        Cout << begin << " : ";
        if (hex) {
            Cout << Hex(indexfrq[begin]);
        } else {
            Cout << indexfrq[begin];
        }
        Cout << Endl;
    }

    return 0;
}
