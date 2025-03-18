#pragma once

#include "datawork_api.h"
#include "datawork_util.h"
#include "datawork_filter.h"

template <typename TDispatcher>
struct TInfoWork {
    static int Work(int argc, char* argv[], TDatMetaPage* meta, TFile& /*ifile*/) {
        Opt opt(argc, argv, "i:?");
        opt.Err = 1;
        int ch;
        while ((ch = opt.Get()) != EOF) {
            switch (ch) {
                case 'i':
                    break;
                case '?':
                default:
                    return IDataworkApi::E_WRONG_USAGE;
            }
        }
        argc -= opt.Ind;
        argv += opt.Ind;
        if (argc != 0)
            return IDataworkApi::E_WRONG_USAGE;

        int ret;
        if ((ret = TDispatcher::DispatchInfo(meta->RecordSig))) {
            if (ret != MBDB_BAD_RECORDSIG)
                err(1, "%s", ErrorMessage(ret, "info error").data());
            else
                return IDataworkApi::E_NORECORD;
            ;
        }
        return IDataworkApi::AdjustRet(ret);
    }
    static void Register(IDataworkApi* api) {
        api->RegSomeFunc("info", Work, "info   -i <input>\n");
    }
};

template <typename TDispatcher>
struct TListTypesWork {
    static int Work(int /*argc*/, char** /*argv*/) {
        TDispatcher::DispatchListTypes();
        return 0;
    }
    static void Register(IDataworkApi* api) {
        api->RegSomeFunc("list-types", Work, "list-types\n");
    }
};

template <typename TDispatcher>
struct TIndexWork {
    static int Work(int argc, char* argv[], TDatMetaPage* meta, TFile& ifile) {
        char fname[FILENAME_MAX];
        const char *nin = nullptr, *nout = nullptr;
        int plog = -1, ret, ch;

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

        if (!nout || !*nout) {
            if (DatNameToIdx(fname, nin))
                return IDataworkApi::E_WRONG_USAGE;
            nout = fname;
        }
        if (!strcmp(nout, "-"))
            nout = "/dev/stdout";

        if ((ret = TDispatcher::DispatchIndex(ifile, nout, meta, 1u << plog, *argv))) {
            if (ret != MBDB_BAD_RECORDSIG)
                err(1, "%s", ErrorMessage(ret, "Data index error").data());
            else
                return IDataworkApi::E_NORECORD;
        }

        return IDataworkApi::AdjustRet(ret);
    }
    static void Register(IDataworkApi* api) {
        api->RegSomeFunc("index", Work, "index  -i <input> -o <output> -p <pgsize-log2> <key-type>\n");
    }
};

template <template <typename T> class TDispatcher>
struct TFilterWork {
    static int Work(int argc, char* argv[], TDatMetaPage* meta, TFile& ifile) {
        const char* nout = nullptr;
        TFile ofile;
        int ch;
        int ret = -1;
        class Opt opt(argc, argv, "i:o:?");
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

        try {
            if (argc == 3 && !strcmp(argv[1], "eqfile")) {
                TSelectCore<TEqFile> flt;
                flt.Init(argv[2]);
                ret = TDispatcher<TSelectCore<TEqFile>>::DispatchFilter(ifile, ofile, meta, argv[0], flt);
            } else if (argc == 3 && !strcmp(argv[1], "nefile")) {
                TSelectCore<TNeFile> flt;
                flt.Init(argv[2]);
                ret = TDispatcher<TSelectCore<TNeFile>>::DispatchFilter(ifile, ofile, meta, argv[0], flt);
            } else if (argc == 3 && !strcmp(argv[1], "eqrange")) {
                TBinSearchCore flt;
                flt.Init(argv[2], argv[2]);
                ret = TDispatcher<TBinSearchCore>::DispatchFilter(ifile, ofile, meta, argv[0], flt);
            } else if (argc == 4 && !strcmp(argv[1], "eqrange")) {
                TBinSearchCore flt;
                flt.Init(argv[2], argv[3]);
                ret = TDispatcher<TBinSearchCore>::DispatchFilter(ifile, ofile, meta, argv[0], flt);
            } else if (argc == 3 && !strcmp(argv[1], "eq")) {
                TSelectCore<TEqFilter> flt;
                flt.Init(argv[2]);
                ret = TDispatcher<TSelectCore<TEqFilter>>::DispatchFilter(ifile, ofile, meta, argv[0], flt);
            } else if (argc == 3 && !strcmp(argv[1], "ne")) {
                TSelectCore<TNeFilter> flt;
                flt.Init(argv[2]);
                ret = TDispatcher<TSelectCore<TNeFilter>>::DispatchFilter(ifile, ofile, meta, argv[0], flt);
            } else if (argc == 3 && !strcmp(argv[1], "lt")) {
                TSelectCore<TLtFilter> flt;
                flt.Init(argv[2]);
                ret = TDispatcher<TSelectCore<TLtFilter>>::DispatchFilter(ifile, ofile, meta, argv[0], flt);
            } else if (argc == 3 && !strcmp(argv[1], "gt")) {
                TSelectCore<TGtFilter> flt;
                flt.Init(argv[2]);
                ret = TDispatcher<TSelectCore<TGtFilter>>::DispatchFilter(ifile, ofile, meta, argv[0], flt);
            } else {
                warn("filter <field> <operation> <value> [<value2>]:filter supports now only eq,lt,gt,eqrange,eqfile,nefile operations");
                if (argc > 1)
                    err(1, "but requested %s", argv[1]);
                exit(1);
            }
        } catch (const std::exception& e) {
            ret = -1;
            fputs(e);
        }

        if (ret != 0) {
            if (ret != MBDB_BAD_RECORDSIG)
                err(1, "%s", ErrorMessage(ret, "Data load error").data());
            else
                return IDataworkApi::E_NORECORD;
        }
        return IDataworkApi::AdjustRet(ret);
    }
    static void Register(IDataworkApi* api) {
        api->RegSomeFunc("filter", Work, "filter -i <input> -o <output> <field> <operation> <value> [<value2>] ::operation = one of eq,lt,gt,eqrange,eqfile\n");
    }
};
