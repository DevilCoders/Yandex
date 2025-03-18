/*
 */
#include <ysite/datasrc/dsconf/dconfig.h>
#include <ysite/datasrc/dsconf/printsection.h>
#include <ysite/indexer/dindexer.h>
#include <ysite/indexer/merge2.h>

#include <library/cpp/getopt/opt.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/network/socket.h>
#include <util/system/defaults.h>
#include <util/system/file.h>
#include <util/system/maxlen.h>

#ifdef USE_MPROF
    extern "C" {
    void mprof_restart(char* filename);
    void mprof_stop(void);
    }
#endif

static TWorkManager *Manager = nullptr;
static int fl_stop_indexing = 0;

#ifdef _win32_
    BOOL WINAPI DsindexerControlHandler (DWORD dwCtrlType)
    {
        if (Manager) {
            switch (dwCtrlType) {
                case CTRL_BREAK_EVENT:  // use Ctrl+C or Ctrl+Break to simulate
                case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in debug mode
                case CTRL_SHUTDOWN_EVENT:
                    printf("User break for indexer. Please wait for closing the index...\n");
                    Manager->IndexClose();
                    return TRUE;
                    break;

            }
        }
        return FALSE;
    }
#else
#   include <signal.h>
    void DsindexerControlHandler(int) {
        if (fl_stop_indexing)
            exit(1);
        if (Manager) {
            fl_stop_indexing = 1;
            Manager->IndexClose();
        }
    }
#endif

enum MY_ERROR {
    MY_COMMON    =  -1, // exception
    MY_CONFIG    = 101, // errors in config
    MY_EXPIRED   = 102, // expired
};

static void print_ysite_ver() {
    printf("%s\n", GetProgramSvnVersion());
}

void dsindexerUsage(const char* prog) {
    fprintf(stderr, "Usage: %s [-h] [-v] [-l] [-k] [-w WEB_PAGE] [-f DIR_TO_INDEX] [-r] [-i OLD_INDEX_DIR] [CONFIG_FILE]\n", prog);
    fprintf(stderr, "       -h - to give this help\n");
    fprintf(stderr, "       -v - to display version number\n");
    fprintf(stderr, "       -l - to print limits and configs without indexing\n");
    fprintf(stderr, "       -k - to keep the old index\n");
    //fprintf(stderr, "The following options are not supported in this version:\n");
    fprintf(stderr, "       -w - to begin the indexing with WEB_PAGE\n");
    fprintf(stderr, "            (an equivalent of directive \"StartUrls\" of WEBDS data source)\n");
    fprintf(stderr, "       -f - to index the local DIR_TO_INDEX\n");
    fprintf(stderr, "            (an equivalent of directive \"Folder/Path\" of FTDS data source)\n");
    fprintf(stderr, "       -r - to reindex the previous index\n" );
    fprintf(stderr, "            (an equivalent of directive \"GlobalOptions : Reindex\" of CONFIG_FILE)\n");
    fprintf(stderr, "       -i - to update the previous index placed in OLD_INDEX_DIR\n" );
    fprintf(stderr, "            (an equivalent of directive \"IndexDir\"  of CONFIG_FILE)\n");
    fprintf(stderr, "CONFIG_FILE - the configuration file, \"dsindexer.cfg\" by default.\n");

    // for internal use:  [-d{-1|0|1}]
    //fprintf(stderr, "    -d  1 - merge & remove portions (by default)"\n);
    //fprintf(stderr, "     0 - merge & not remove portions\n");
    //fprintf(stderr, "    -1 - not merge & not remove portions\n");

}

TString LocalConfig(const TString& WebPage, const TString& LocalDir, const TString& IndexDir, bool Reindex)
{
    TString IndexerConfig =
            "<Collection>\n";
    if (!!IndexDir)
        IndexerConfig +=
        "        IndexDir : " + IndexDir + "\n";
    if (Reindex)
        IndexerConfig +=
        "   GlobalOptions : Reindex";
    if (!!WebPage)
        IndexerConfig +=
            "   <DataSrc id=\"webds\">\n"
            "       Config : -w " + WebPage + "\n"
            "   </DataSrc>\n";
    if (!!LocalDir)
        IndexerConfig +=
            "   <DataSrc id=\"ftds\">\n"
            "       Config : -f " + LocalDir + "\n"
            "   </DataSrc>\n";
    IndexerConfig +=
            "   <IndexLog>\n"
            "       FileName : dsindexer.log\n"
            "          Level : Config Warning Info\n"
            "   </IndexLog>\n";
    IndexerConfig +=
        "</Collection>\n";
    return IndexerConfig;
}

/*
TString ChangeConfig(TYandexConfig& Config)
{
    std::ostrstream out;
    DSIndexerConfigInterceptor interceptor(out);
    interceptor.SetReindex(Reindex);
    interceptor.SetIndexDir(~IndexDir);
    interceptor.SetWebPage(~LocalDir);
    interceptor.SetLocalDir(~LocalDir);
    InterceptConfig(Config, interceptor);
    return TString(out.str(), 0, out.pcount());
}
*/

int dsindexer(int argc, char** argv)
{
#ifdef _win32_
    try {
        InitNetworkSubSystem();
#endif

#ifdef USE_MPROF
    mprof_restart("mprof.DsIndexer");
#endif

    TString path;
    {
        TArrayHolder<char> cur_dir(new char[MAX_PATH]);
        getcwd(cur_dir.Get(), MAX_PATH);
        path = cur_dir.Get();
    }
    SlashFolderLocal(path);

#ifdef _win32_
    SetConsoleCtrlHandler(DsindexerControlHandler, TRUE);
#else
    signal(SIGPIPE, SIG_IGN);
#   ifdef SIGHUP
        signal(SIGHUP, SIG_IGN);
#   endif
    signal(SIGINT, DsindexerControlHandler);
#endif

    bool NoIndex = false;
    TString WebPage, LocalDir, IndexDir;
    bool KeepOldIndex = false, Reindex = false;
    int fl_rmPortionsFromDisk = 1;
    int thcount = 0;
    int optlet;
    class Opt opt (argc, argv, "hvlkrd:w:f:i:t:");
    while ((optlet = opt.Get()) != EOF) {
        switch (optlet) {
            case 'h':
            case '?':
                dsindexerUsage(argv[0]);
                return 0;
                break;
            case 'v':
                print_ysite_ver();
                return 0;
            case 'k':
                KeepOldIndex = true;
                break;
            case 'd':
                fl_rmPortionsFromDisk = atol(opt.Arg);
                if (fl_rmPortionsFromDisk < -1 || fl_rmPortionsFromDisk > 1) {
                    fprintf(stderr, "%s: Error: incorrect option value: '-d %d'. Competent values: -1,0,1.\n", argv[0], fl_rmPortionsFromDisk);
                    exit(1);
                }
                break;
            case 'l':
                NoIndex = true;
                break;
            case 'w':
                WebPage = opt.Arg;
                break;
            case 'f':
                LocalDir = opt.Arg;
                break;
            case 'i':
                IndexDir = opt.Arg;
                break;
            case 'r':
                Reindex = true;
                break;
            case 't':
                thcount = atol(opt.Arg);
                break;
            default:
                dsindexerUsage(argv[0]);
                return 1;
                break;
        }
    }


    TString IndexerConfig;
    TString Inifile = (argc > opt.Ind) ? argv[opt.Ind] : "dsindexer.cfg";
    {
        resolvepath(Inifile, path);
        if (!NFs::Exists(Inifile)) {
            if (!!WebPage || !!LocalDir)
                IndexerConfig = LocalConfig(WebPage, LocalDir, IndexDir, Reindex);
        }
    }

    TDSIndexConfig Cfg;
    {
        if (!!IndexerConfig) {

            DsIndexConfigStrings ic;
            bool ok = ic.ParseMemory(IndexerConfig.data());
            TString Err;
            ic.PrintErrors(Err);
            if (!!Err)
                fprintf(stderr, "Config file \'%s\' was parsed with the message(s):\n%s\n", Inifile.data(), Err.data());
            if (!ok)
                return MY_CONFIG;

            IndexerConfig = PrintConfig(ic);

        } else if (NFs::Exists(Inifile)) {

            DsIndexConfigStrings ic;
            bool ok = ic.Parse(Inifile);
            TString Err;
            ic.PrintErrors(Err);
            if (!!Err)
                fprintf(stderr, "Config file \'%s\' was parsed with the message(s):\n%s\n", Inifile.data(), Err.data());
            if (!ok)
                return MY_CONFIG;

            if( Reindex || !!IndexDir){
                TStringStream out;
                DSIndexerConfigInterceptor interceptor(out);
                interceptor.SetReindex(Reindex);
                interceptor.SetIndexDir(IndexDir.data());
                interceptor.SetWebPage(WebPage.data());
                interceptor.SetLocalDir(LocalDir.data());
                InterceptConfig(ic, interceptor);
                IndexerConfig = out.Str();
            } else {
                IndexerConfig = PrintConfig(ic);
            }

        } else {
            fprintf(stderr, "Config file \'%s\' is absent.\n", Inifile.data());
            if (!!IndexerConfig)
                return MY_CONFIG;
        }

        DsIndexConfigStrings ic;
        bool ok = ic.ParseMemory(IndexerConfig.data()) && Cfg.Init(ic);
        TString Err;
        ic.PrintErrors(Err);
        if (!!Err)
            fprintf(stderr, "Config file \'%s\' was parsed with the message(s):\n%s\n", Inifile.data(), Err.data());
        if (!ok)
            return MY_CONFIG;
    }

    if (NoIndex) {
        TYandexConfig::Directives ILog;
        ILog["Filename"] = nullptr;
        ILog["Level"] = "Verbose";
        Cfg.OpenFromSection(ILog, path);
        Cfg.PrintConfig();
        return 0;
    }

#ifndef DSINDEXER_QUIET
    fprintf(stdout, "Start indexing...\n");
    // Cfg.EnableResultToCout();
#endif

    Cfg.PrintConfig();

    const INDEX_CONFIG* IC = Cfg.GetIndexConfig();

    if (KeepOldIndex == false) {
        // Check whether we will be able to rename index
        TString from = TString(IC->newindexdir) + "dircheck";
        TString to = Cfg.olddir + "dircheck";
        TFile(from, OpenAlways | WrOnly).Close(); // Create empty file
        if (rename(from.data(), to.data())) {
            remove(from.data());
            Cfg.Notify(LL_CRITERROR, "Cannot move a file from temporary directory to final. Please check whether they are on the same filesystem.\n");
#ifndef DSINDEXER_QUIET
            Cerr << "Cannot move a file from temporary directory to final. Please check whether they are on the same filesystem." << Endl;
#endif // DSINDEXER_QUIET
            return MY_CONFIG;
        } else {
            remove(to.data());
        }
    }

    try {
        TDSIndexer Indexer(IC);
        Manager = &Indexer;
        for (TDSConfiguredLibs::iterator I = Cfg.DSLibs.begin(); I != Cfg.DSLibs.end(); ++I)
            Indexer.Work(thcount, (*I).first, (*I).second.first.Symbol(), (*I).second.second.data());
        Indexer.Merge(fl_rmPortionsFromDisk);
    } catch (std::exception &e) {
        Cfg.Notify(LL_CRITERROR, "%s\n", e.what());
        ::RenameIndex("", IC->newindexdir);
        return MY_COMMON;
    }

    if (KeepOldIndex == false)
        ::RenameIndex(IC->newindexdir, Cfg.olddir, IC->YxLogNotify, IC->LogObj);

#ifdef USE_MPROF
    mprof_stop();
#endif
#ifdef _win32_
    } catch (...) {
    }
    WSACleanup();
#endif
    return 0;
}

