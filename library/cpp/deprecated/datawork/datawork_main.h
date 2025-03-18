#pragma once

#include <util/system/dynlib.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <library/cpp/deprecated/fgood/fgood.h>

#include "datawork_api.h"
#include "datawork_util.h"
#include <library/cpp/svnversion/svnversion.h>

class TDataworkMain: public IDataworkApi {
    struct TActDescription {
        TVector<TActionBinaryFunction> BinFuncs;
        TVector<TActionTextFunction> TextFuncs;
        TString Help;
    };

public:
    int RegisterTextAction(const char* name, TActionTextFunction func) override {
        TActDescription& d = Functions[TString(name)];
        if (!d.BinFuncs.empty())
            return E_WRONGMODE;

        Functions[TString(name)].TextFuncs.push_back(func);
        return 0;
    }

    int RegisterBinaryAction(const char* name, TActionBinaryFunction func) override {
        TActDescription& d = Functions[TString(name)];
        if (!d.TextFuncs.empty())
            return E_WRONGMODE;

        Functions[TString(name)].BinFuncs.push_back(func);
        return 0;
    }

    int RegisterHelp(const char* name, const char* help) override {
        TActDescription& d = Functions[TString(name)];
        TString nhlp(help);
        if (nhlp.EndsWith("\n"))
            nhlp = nhlp.substr(0, nhlp.length() - 1);

        if (d.Help.length() < nhlp.length())
            d.Help = nhlp;
        else
            return E_HELP_EXIST;

        return 0;
    }

    int RegisterDumpRec(const char* name, TDumpRecFunction func) override {
        DumpRecFuncs[TString(name)] = func; //always override
        return 0;
    }
    TDumpRecFunction GetDumpRec(const char* name) override {
        THashMap<TString, TDumpRecFunction>::iterator f = DumpRecFuncs.find(name);
        if (f == DumpRecFuncs.end())
            return nullptr;
        return f->second;
    }

public:
    TDataworkMain() {
    }

    int Dispatch(const char* myname, const char* name, int argc, char** argv) {
        TString cmd(name);
        cmd.to_lower();
        THashMap<TString, TActDescription>::iterator f = Functions.find(cmd);
        TDatMetaPage mp;
        if (f == Functions.end())
            return E_NOACTION;
        int r = 0;
        if (!f->second.TextFuncs.empty())
            for (TVector<TActionTextFunction>::iterator i = f->second.TextFuncs.begin(); i != f->second.TextFuncs.end(); i++) {
                r = (*i)(argc, argv);
                if (r == E_AGAIN) {
                    r = E_OK;
                    continue;
                }
                if (r == E_NORECORD)
                    continue;
                if (r == E_WRONG_USAGE)
                    errx(1, "Usage [%s] %s\n", myname, f->second.Help.data());
                return r;
            }
        else {
            int o = 0;
            TFile ifile;
            for (o = 0; o < argc; o++)
                if (!strcmp(argv[o], "-h") || !strcmp(argv[o], "--"))
                    break;

            if (o < argc && !strncmp(argv[o], "-h", 2))
                errx(1, "Usage [%s] %s\n", myname, f->second.Help.data());

            for (o = 0; o < argc; o++)
                if (!strncmp(argv[o], "-i", 2) || !strcmp(argv[o], "--"))
                    break;
            const char* inpfile = nullptr;

            if (o < argc && !strncmp(argv[o], "-i", 2))
                if (argv[o][2])
                    inpfile = &argv[o][2];
                else if (o == argc - 1)
                    errx(1, "Usage [%s] %s\n", myname, f->second.Help.data());
                else
                    inpfile = argv[o + 1];

            if ((r = OpenInputFile(inpfile, ifile)) != 0)
                err(1, "Can't read \"%s\"", inpfile);

            if ((r = GetMeta(ifile, &mp)) != 0)
                err(1, "Can't read meta from \"%s\" : %d", inpfile, r);

            for (TVector<TActionBinaryFunction>::iterator i = f->second.BinFuncs.begin(); i != f->second.BinFuncs.end(); i++)
                if ((r = (*i)(argc, argv, &mp, ifile)) != E_NORECORD) {
                    if (r == E_WRONG_USAGE)
                        errx(1, "Usage [%s] %s\n", myname, f->second.Help.data());
                    return r;
                }
        }
        return r;
    }

    void PrintUsages(const char* myname) {
        printf("%s help:\n", myname);
        for (auto& function : Functions) {
            printf("\t");
            puts(function.second.Help.data());
        }
        printf("\t--version (print svn and build info)\n");
    }

    int DoDataWork(char* mypath, int argc, char* argv[]) {
        PrintSvnVersionAndExit(argc, argv);
        char* myname = mypath;
        char* ptr = strrchr(myname, '/');
        char* ptr2 = strrchr(myname, '\\');
        if (ptr || ptr2)
            myname = std::max(ptr, ptr2) + 1;

        int ret = -1;

        if (!strnicmp(myname, "data", 4))
            if ((ret = Dispatch(myname, myname + 4, argc, argv)) != IDataworkApi::E_NOACTION)
                if (ret == IDataworkApi::E_NORECORD)
                    err(1, "no such record registered"); // XXX: print record sig
                else if (ret != 0)
                    err(1, "unknown error %d\n", ret);

        if (argc > 1 && ret != 0)
            if ((ret = Dispatch(myname, argv[1], argc - 1, argv + 1)) != IDataworkApi::E_NOACTION)
                if (ret == IDataworkApi::E_NORECORD)
                    errx(1, "no such record registered");
                else if (ret != 0)
                    err(1, "unknown error %d\n", ret);

        if (ret == IDataworkApi::E_NOACTION || argc == 1) {
            PrintUsages(myname);

            if (argc > 1)
                errx(1, "no such action %s or %s", myname, (argc > 1) ? argv[1] : "nothing");
        }

        return ret;
    }

private:
    THashMap<TString, TActDescription> Functions;
    THashMap<TString, TDumpRecFunction> DumpRecFuncs;
};
