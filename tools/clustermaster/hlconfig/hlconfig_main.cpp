#include <tools/clustermaster/common/master_list_manager.h>

#include <yweb/video/util/util.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>

int main(int argc, char **argv) {
    NLastGetopt::TOpts opts;
    TString hostListPath;

    opts.SetFreeArgsMin(1);
    opts.SetFreeArgsMax(2);
    opts.SetFreeArgTitle(0, "TAG", "tag to list");
    opts.SetFreeArgTitle(1, "HOST", "host to list clusters on");
    opts.AddCharOption('h', "path to hosts.list (by default read from stdin)").OptionalArgument("PATH").StoreResult(&hostListPath);
    opts.AddHelpOption('?');

    if (argc <= 1) {
        opts.PrintUsage(argv[0] ? argv[0] : "hlconfig");
        exit(1);
    }
    NLastGetopt::TOptsParseResult optr(&opts, argc, argv);
    TVector<TString> HostArgs = optr.GetFreeArgs();

    TMasterListManager manager;
    if (optr.Has('h')) {
       THolder<TIFStream> inFile = MakeHolder<TIFStream>(hostListPath);
       manager.LoadHostlistFromString(inFile->ReadAll());
    } else {
       manager.LoadHostlistFromString(Cin.ReadAll());
    }

    TVector<TString> tags;
    manager.GetList(HostArgs[0], (HostArgs.size() > 1 ? HostArgs[1] : ""), tags);
    for(TVector<TString>::iterator tag = tags.begin(); tag != tags.end(); tag++) {
        Cout << *tag << Endl;
    }

    return 0;
}
