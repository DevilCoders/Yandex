#pragma once

#include <tools/segutils/segcommon/data_utils.h>

#include <kernel/segutils/numerator_utils.h>

#include <library/cpp/getopt/opt.h>

#include <util/folder/path.h>

namespace NSegutils {

class TTest {
protected:
    THolder<TParserContext> Ctx;
    THtmlFileReader Reader;

public:
    void Init(int argc, const char** argv);
    void RunTest();
    virtual ~TTest() {}

protected:
    virtual void DoInit() {}
    virtual TString GetArgs() const { return ""; }
    virtual bool ProcessArg(int, Opt&) { return false; }
    virtual TString GetHelp() const { return ""; }

    virtual TString ProcessDoc(const THtmlFile& f) { return f.FileName; }

    void UsageAndExit(const char* me, int code);

    static void PrintEvents(const NSegm::TEventStorage& st, IOutputStream& out);
};

}
