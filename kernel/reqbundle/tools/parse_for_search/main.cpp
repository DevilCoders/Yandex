#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/streams/factory/factory.h>
#include <library/cpp/getopt/last_getopt.h>

#include <kernel/reqbundle/parse_for_search.h>
#include <kernel/reqbundle/serializer.h>

#include <util/string/split.h>
#include <util/generic/deque.h>


int main(int argc, const char* argv[]) {
    NLastGetopt::TOpts opts;
    opts.SetTitle("Parse and merged bundles in each line as in production CGI processing");

    TString inputPath;
    opts.AddLongOption('i', "input", "input bundles; base64-encoded, tab-separated")
        .Optional()
        .RequiredArgument("PATH")
        .DefaultValue("-")
        .StoreResult(&inputPath);

    NLastGetopt::TOptsParseResult optsResult{&opts, argc, argv};

    THolder<IInputStream> input = OpenInput(inputPath);

    TString line;
    while (input->ReadLine(line)) {
        TDeque<TStringBuf> parts;
        StringSplitter(line).Split('\t').AddTo(&parts);

        NReqBundle::TReqBundleSearchParser::TOptions options;
        NReqBundle::TReqBundleSearchParser parser{options};

        for (TStringBuf part: parts) {
            parser.AddBase64(part);
        }

        if (parser.IsInErrorState()) {
            Cerr << parser.GetFullErrorMessage();
            parser.ClearErrorState();
        }

        NReqBundle::TReqBundlePtr bundle = parser.GetPreparedForSearch();
        if (bundle) {
            NReqBundle::NSer::TSerializer ser;
            Cout << Base64EncodeUrl(ser.Serialize(*bundle));
        }
        Cout << Endl;
    }
}
