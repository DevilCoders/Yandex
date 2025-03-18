#include <kernel/snippets/util/xml.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <library/cpp/http/io/stream.h>
#include <util/stream/str.h>
#include <util/string/cast.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/split.h>
#include <util/string/vector.h>

int main(int argc, char** argv)
{
    TString inName;
    TString outName;

    using namespace NLastGetopt;
    TOpts opts;
    opts.AddHelpOption();

    opts.AddLongOption("i").StoreResult(&inName).RequiredArgument("input xml file").Required();
    opts.AddLongOption("o").StoreResult(&outName).RequiredArgument("output xml file").Required();

    TOptsParseResult r(&opts, argc, argv);

    TBuffered<TUnbufferedFileInput> in(8192, inName);
    TBuffered<TUnbufferedFileOutput> out(20 * 1024 * 1024, outName);

    TString line;
    while (in.ReadLine(line))
    {
        out << NSnippets::EncodeTextForXml10(line, false) << Endl;
    }
}
