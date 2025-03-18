#include <yweb/structhtml/htmlstatslib/htmlstatslib.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/stream/input.h>
#include <util/stream/buffered.h>
#include <util/generic/string.h>
#include <util/generic/hash.h>



int main(int argc, const char* argv[])
{
    TString groupFile;
    bool requireGroup = false;

    using namespace NLastGetopt;
    TOpts opts;
    opts.AddCharOption('g', REQUIRED_ARGUMENT, "group list filename").StoreResult(&groupFile);
    opts.AddCharOption('n', NO_ARGUMENT, "drop URLs with no explicit group").SetFlag(&requireGroup);
    opts.SetFreeArgsNum(0);
    TOptsParseResult optsParsed(&opts, argc, argv);

    if (!groupFile && requireGroup) {
        Cerr << "-n can only be used together with -g" << Endl;
        return 1;
    }

    NHtmlStats::TUrlGrouper grouper;
    if (!!groupFile) {
        grouper.LoadGroups(groupFile);
    }

    TBufferedInput inf(&Cin, 1<<24);
    TString url;
    while (inf.ReadLine(url)) {
        if (!url) {
            continue;
        }
        TString group = grouper.GetGroupName(url, requireGroup);
        if (!group) {
            continue;
        }
        Cout << group << "\t" << url << Endl;
    }
}
