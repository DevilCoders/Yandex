#pragma once

#include "datawork_util.h"

#include <library/cpp/getopt/opt.h>

#define PLOG_DEFAULT "14"

template <typename TDispatcher>
struct TLoadWork {
    static int Work(int argc, char* argv[]) {
        const char *nin = nullptr, *nout = nullptr;
        TFile ifile, ofile;
        int plog = atoi(PLOG_DEFAULT), ret, ch;

        class Opt opt(argc, argv, "i:o:p:?");
        opt.Err = 1;
        while ((ch = opt.Get()) != EOF) {
            switch (ch) {
                case 'i':
                    nin = opt.Arg;
                    break;
                case 'o':
                    nout = opt.Arg;
                    break;
                case 'p':
                    plog = atoi(opt.Arg);
                    break;
                case '?':
                default:
                    return IDataworkApi::E_WRONG_USAGE;
            }
        }
        argc -= opt.Ind;
        argv += opt.Ind;
        if (argc != 1 || plog < 10 || plog > 30)
            return IDataworkApi::E_WRONG_USAGE;

        OpenFiles(nin, nout, ifile, ofile);
        if ((ret = TDispatcher::DispatchLoad(*argv, ifile, ofile, 1u << plog))) {
            if (ret != MBDB_BAD_RECORDSIG)
                err(1, "Data load error: %d", ret);
            else
                return IDataworkApi::E_NORECORD;
            ;
        }

        return IDataworkApi::AdjustRet(ret);
    }
    static void Register(IDataworkApi* api) {
        api->RegSomeFunc("load", Work, "load   -i <input> -o <output> [-p <pgsize-log2>:" PLOG_DEFAULT "] <record-type>\n");
    }
};

template <typename TDispatcher>
struct TLoadEmptyWork {
    static int Work(int argc, char* argv[]) {
        const char* nout = nullptr;
        int plog = atoi(PLOG_DEFAULT), ret, ch;
        TFile ofile;

        class Opt opt(argc, argv, "o:p:?");
        opt.Err = 1;
        while ((ch = opt.Get()) != EOF) {
            switch (ch) {
                case 'o':
                    nout = opt.Arg;
                    break;
                case 'p':
                    plog = atoi(opt.Arg);
                    break;
                case '?':
                default:
                    return IDataworkApi::E_WRONG_USAGE;
            }
        }
        argc -= opt.Ind;
        argv += opt.Ind;
        if (argc != 1 || plog < 10 || plog > 30)
            return IDataworkApi::E_WRONG_USAGE;

        OpenOutputFile(nout, ofile);

        ui32 sig;
        TString name(argv[0]);
        try {
            sig = ParseInt<ui32>(name);
        } catch (const yexception&) {
            sig = TDispatcher::SigByName(name);
        }

        TOutputPageFile opf;

        ret = opf.Init(ofile, 1u << plog, sig);
        if (ret != 0)
            return IDataworkApi::AdjustRet(ret);

        ofile.Close();
        return 0;
    }
    static void Register(IDataworkApi* api) {
        api->RegSomeFunc("load-empty", Work, "load-empty -o <output> [-p <pgsize-log2>:" PLOG_DEFAULT "] <record-type-or-sig>\n");
    }
};
