#pragma once

#include <library/cpp/getopt/last_getopt.h>

class ITest {
public:
    ITest(int argc, const char** argv)
        : ReadFromStdin_(false)
    {
        NLastGetopt::TOpts opts;

        NLastGetopt::TOpt iso = opts.AddLongOption("stdin").NoArgument();
        opts.SetFreeArgsMax(1);

        NLastGetopt::TOptsParseResult parseRes(&opts, argc, argv);

        if (parseRes.Has("stdin"))
            ReadFromStdin_ = true;
        else
            InputFile_ = parseRes.GetFreeArgs()[0];
    }

    ~ITest() {
    }

    void Run() {
        if (ReadFromStdin_) {
            const TString& input = Cin.ReadAll();
            ProcessSingleDoc(input);
        } else {
            ProcessTestsFile(InputFile_);
        }
    }

protected:
    virtual void ProcessTestsFile(const TString&) = 0;

    virtual void ProcessSingleDoc(const TString&) = 0;

private:
    bool ReadFromStdin_;
    TString InputFile_;
};
