#include "HtmlParser.h"
#include "SingleLineMapreduceFormatProcessor.h"

#include <library/cpp/getopt/last_getopt.h>

int main(int argc, char** argv) {
    using namespace NLastGetopt;
    TOpts opts = TOpts::Default();

    TString formatString;
    opts.AddHelpOption();
    opts.AddLongOption("format", "Valid values: \"single-document\" (default), \"null-terminated\", \"singleline-mapreduce\"").StoreResult(&formatString).DefaultValue("null-terminated");
    TOptsParseResult parseResults(&opts, argc, argv);

    if (formatString == "single-document") {
        TString html = Cin.ReadAll();
        TSimpleSharedPtr<TTextAndTitleSentences> sentences = Html2Text(html);

        Cout << "Title=" << WideToUTF8(sentences->GetTitle()) << Endl;
        Cout << Endl;
        Cout << WideToUTF8(sentences->GetText(WideNewLine)) << Endl;
    } else if (formatString == "null-terminated") {
        bool firstLine = true;
        TString html;
        while (Cin.ReadTo(html, '\x00')) {
            TSimpleSharedPtr<TTextAndTitleSentences> sentences = Html2Text(html);

            Cout << "Title=" << WideToUTF8(sentences->GetTitle()) << Endl;
            Cout << Endl;
            Cout << WideToUTF8(sentences->GetText(WideNewLine)) << Endl;

            if (firstLine) {
                firstLine = false;
            } else {
                Cout << '\x00';
            }
        }
    } else if (formatString == "singleline-mapreduce") {
        SingleLineMapreduceFormatProcessor::ProcessHtml(Cin, Cout);
    } else {
        ythrow yexception() << "Bad format name '" + formatString + "'";
    }
}
