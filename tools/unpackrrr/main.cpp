#include <tools/printwzrd/lib/printer.h>

#include <search/wizard/face/wizinfo.h>

#include <library/cpp/getopt/last_getopt.h>


int PrintUsage()
{
    Cout << "Name:\n\tunpackrrr - convert wizard rrr (rrr2) to human readable format.\n\n";
    Cout << "Usage:\n\tunpackrrr [-c]\n\n";
    Cout << "Options:\n";

    Cout << "\t-c, --cgi\n\t\tInput is given as wizard cgi-response: text=...&qtree=...&rrr2=....\n";
    Cout << "\t\tWithout this option the input is treated as single rrr/rrr2 value\n\n";

    Cout << "\t-h, --help\n\t\tPrint this message and exit\n\n";

    return 1;
}

void UnpackCgi(TPrintwzrdPrinter& printer, const TString& text) {
    TCgiParameters cgi(text);
    TAutoPtr<TWizardResults> res = TWizardResultsCgiPacker::Deserialize(&cgi);
    printer.Print(res->SourceRequests, res->RulesResults, cgi, text);
}

void UnpackRrrOnly(TPrintwzrdPrinter& printer, const TString& text) {
    TCgiParameters cgi;
    cgi.InsertEscaped("rrr2", text);
    THolder<IRulesResults> rrr(TWizardResultsCgiPacker::DeserializeRuleResults(cgi).Release());
    printer.Print(THolder<ISourceRequests>(), rrr, cgi, text);
}


int main(int argc, char* argv[]) {

    using namespace NLastGetopt;
    TOpts opts;
    TOpt& optCgi = opts.AddLongOption('c', "cgi", "Input is given as wizard cgi-response: text=...&qtree=...&rrr2=...");
    optCgi.Optional().NoArgument();

    TOpt& optHelp = opts.AddLongOption('h', "help", "Print this message and exit");
    optHelp.Optional().NoArgument();

    TOptsParseResult r(&opts, argc, argv);

    if (r.Has(&optHelp))
        return PrintUsage();

    bool isCgi = r.Has(&optCgi);

    TPrintwzrdOptions opt;
    opt.DebugMode = true;
    TPrintwzrdPrinter printer(Cout, opt);
    TString line;

    while (Cin.ReadLine(line)) {
        try {

            if (isCgi)
                UnpackCgi(printer, line);
            else
                UnpackRrrOnly(printer, line);

        } catch (yexception&) {
            Cerr << "[error]: " << CurrentExceptionMessage() << Endl;
        }
    }
}
