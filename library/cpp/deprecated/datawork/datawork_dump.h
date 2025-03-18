#pragma once

#include "datawork_api.h"
#include "datawork_util.h"

#include <library/cpp/getopt/opt.h>

ui64 HostCrc4Config(TStringBuf Name, const char* config); //todo remove that prohibits library

template <typename TDispatcher>
struct TDumpWork {
    static int Work(int argc, char* argv[], TDatMetaPage* meta, TFile& ifile) { //use reference to prefilled NLastGetOpt::TOpt (which already has setters for HostCrc4Config & DataworkConf.TreatTDataworkDateAsInteger) as param
        const char* nout = nullptr;
        TFile ofile;
        int ret, ch;

        class Opt opt(argc, argv, "Hi:o:c:?");
        opt.Err = 1;
        while ((ch = opt.Get()) != EOF) {
            switch (ch) {
                case 'c':
                    HostCrc4Config("", opt.Arg); //todo remove that prohibits library
                    break;
                case 'i':
                    break;
                case 'o':
                    nout = opt.Arg;
                    break;
                case 'H':
                    DataworkConf.TreatTDataworkDateAsInteger = false;
                    break;
                case '?':
                default:
                    return IDataworkApi::E_WRONG_USAGE;
            }
        }
        argc -= opt.Ind;
        argv += opt.Ind;

        OpenOutputFile(nout, ofile);
        if ((ret = TDispatcher::DispatchDump(ifile, ofile, meta, argc, argv))) {
            if (ret != MBDB_BAD_RECORDSIG)
                err(1, "Data dump error: %d", ret);
            else
                return IDataworkApi::E_NORECORD;
            ;
        }
        return IDataworkApi::AdjustRet(ret);
    }

    static void Register(IDataworkApi* api) {
        api->RegSomeFunc("dump", Work,
                         "dump   -i <input> -o <output> [-H] <field-list>\n");
    }
};

template <typename TDispatcher>
struct TCountWork {
    static int Work(int argc, char* argv[], TDatMetaPage* meta, TFile& ifile) {
        const char* nout = nullptr;
        TFile ofile;
        int ret, ch;

        class Opt opt(argc, argv, "Hi:o:?");
        opt.Err = 1;
        while ((ch = opt.Get()) != EOF) {
            switch (ch) {
                case 'i':
                    break;
                case 'o':
                    nout = opt.Arg;
                    break;
                case '?':
                default:
                    return IDataworkApi::E_WRONG_USAGE;
            }
        }
        argc -= opt.Ind;
        argv += opt.Ind;

        OpenOutputFile(nout, ofile);
        if ((ret = TDispatcher::DispatchCount(ifile, ofile, meta, argc, argv))) {
            if (ret != MBDB_BAD_RECORDSIG)
                err(1, "Data dump error: %d", ret);
            else
                return IDataworkApi::E_NORECORD;
            ;
        }
        return IDataworkApi::AdjustRet(ret);
    }

    static void Register(IDataworkApi* api) {
        api->RegSomeFunc("count", Work,
                         "count   -i <input>\n");
    }
};
