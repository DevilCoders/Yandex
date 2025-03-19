#include <kernel/dups/banned_info.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/streams/factory/factory.h>

#include <util/generic/deque.h>
#include <util/stream/file.h>
#include <util/stream/format.h>
#include <util/stream/str.h>
#include <util/string/split.h>

using namespace NDups::NBannedInfo;

struct TProgramParameters {
    TVector<TString> Urls;
    TString InputFileName;
};

class TMain {
private:
    TProgramParameters ProgramParameters;

public:
    TMain(int argc, const char* argv[]) {
        using namespace NLastGetopt;
        TOpts opts = NLastGetopt::TOpts::Default();
        opts.AddHelpOption();
        opts.AddVersionOption();
        opts.SetFreeArgsMin(0);
        opts.SetFreeArgDefaultTitle("url", "Input list of urls to check");
        opts.AddLongOption('i', "input", "File with urls").StoreResult(&ProgramParameters.InputFileName);
        TOptsParseResult res(&opts, argc, argv);
        ProgramParameters.Urls = res.GetFreeArgs();
    }

    int Run() {
        TDeque<TString> urls;
        if (ProgramParameters.InputFileName) {
            auto stream = OpenInput(ProgramParameters.InputFileName);
            TString line;
            while (stream->ReadLine(line)) {
                StringSplitter(line).SplitBySet(" \t").AddTo(&urls);
            }
        } else if (!ProgramParameters.Urls.empty()) {
            urls.insert(urls.end(), ProgramParameters.Urls.begin(), ProgramParameters.Urls.end());
        }

        EraseIf(urls, std::mem_fn(&TString::empty));

        if (urls.empty()) {
            ythrow yexception() << "Empty set of urls";
        }

        for (const TString& url : urls) {
            const ui32 hash = ::NDups::NBannedInfo::GetHashKeyForUrl(url);
            Cout << hash << '\t' << url << Endl;
        }

        return 0;
    }
};

int main(int argc, const char* argv[]) {
    try {
        return TMain(argc, argv).Run();
    } catch (...) {
        Cerr << "Uncaught exception:\n"
             << CurrentExceptionMessage() << Endl;
        throw;
    }
}
