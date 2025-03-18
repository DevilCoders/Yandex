#include <library/cpp/getopt/last_getopt.h>
#include <tools/printdom/lib/print.h>

#include <util/stream/file.h>

using namespace NLastGetopt;


int main(int argc, const char* argv[]) {
    TString inputFile;
    TOptions options;
    bool asHtml = false;
    bool asJson = false;

    {
        TOpts opts;
        opts.AddHelpOption('h');
        opts.AddLongOption('i', "input", "input filename")
            .Required()
            .StoreResult(&inputFile)
            .RequiredArgument();
        opts.AddLongOption('e', "empty", "print empty texts")
            .Optional()
            .NoArgument()
            .SetFlag(&options.EmptyText)
            .DefaultValue("0");
        opts.AddLongOption('s', "styles", "print styles")
            .Optional()
            .NoArgument()
            .SetFlag(&options.Styles)
            .DefaultValue("0");
        opts.AddLongOption('v', "viewbound", "print element's viewbound")
            .Optional()
            .NoArgument()
            .SetFlag(&options.Viewbound)
            .DefaultValue("1");
        opts.AddLongOption("html", "print html representation")
            .Optional()
            .NoArgument()
            .SetFlag(&asHtml)
            .DefaultValue("0");
        opts.AddLongOption("json", "print json representation")
            .Optional()
            .NoArgument()
            .SetFlag(&asJson)
            .DefaultValue("0");

        TOptsParseResult res(&opts, argc, argv);
    }

    const TString data(TUnbufferedFileInput(inputFile).ReadAll());

    if (asHtml) {
        return PrintAsHtml(data);
    }
    if (asJson) {
        return PrintAsJson(data, options);
    }

    return PrintAsTree(data, options);
}
