#pragma once

#include <library/cpp/microbdb/microbdb.h>

struct IDataworkApi {
    typedef int (*TActionTextFunction)(int argc, char** argv);
    typedef int (*TActionBinaryFunction)(int argc, char** argv, TDatMetaPage* meta, TFile& ifile);
    typedef int (*TDumpRecFunction)(const void* Rec, const char* name, void* data);

    enum {
        MAXERRLEN = 1 << 10,
    };

    enum EDataworkResult {
        E_OK = 0,       //DO NOT CRUSH
        E_NORECORD = 1, //DO NOT CRUSH
        E_NOACTION = 2,
        E_WRONGMODE = 3,  //DO NOT CRUSH (PLUGIN IS NOT REGISTERED ON SUCH ACTION)
        E_HELP_EXIST = 4, //DO NOT CRUSH
        E_WRONG_USAGE = 5,
        E_AGAIN = 6,
        E_UNKNOWN = -1
    };
    static int AdjustRet(int ret) {
        if (ret != 0)
            ret += 1000000;
        return ret;
    }

    char* LastError;
    virtual int RegisterHelp(const char* name, const char* help) = 0;
    virtual int RegisterTextAction(const char* name, TActionTextFunction func) = 0;
    virtual int RegisterBinaryAction(const char* name, TActionBinaryFunction func) = 0;
    virtual int RegisterDumpRec(const char* name, TDumpRecFunction func) = 0;
    virtual TDumpRecFunction GetDumpRec(const char* name) = 0;
    virtual ~IDataworkApi() {
    }

    inline void RegSomeFunc(const char* name, IDataworkApi::TActionTextFunction func, const char* help) {
        int r = RegisterTextAction(name, func);
        if (r != 0)
            if (r == E_WRONGMODE)
                warn("can't register %s incompatible read mode\n", name);
            else
                err(1, "can't register %s :%d\n", name, r);

        RegisterHelp(name, help);
    }

    inline void RegSomeFunc(const char* name, IDataworkApi::TActionBinaryFunction func, const char* help) {
        int r = RegisterBinaryAction(name, func);
        if (r != 0)
            if (r == E_WRONGMODE)
                warn("can't register %s incompatible read mode\n", name);
            else
                err(1, "can't register %s :%d\n", name, r);

        RegisterHelp(name, help);
    }
};
