#pragma once

#include <library/cpp/getopt/opt2.h>

#include <util/generic/string.h>

class TOptions {
public:
    struct TValues {
        bool packet;
        TString parserName;
        TString parserConfig;
        TString encoding;
        TString output;
        TString input;

        TValues()
            : packet(false)
        {}
    };

private:
    TValues mValues;

public:
    TOptions(size_t argc, char* const* argv) {
        Opt2 o(argc, argv, "lFp:c:e:o:", IntRange(0, 1));

        mValues.packet = o.Has('l', "-- packet mode: treat input is a list of "
            "files; output is redirected to <file>.nlp_test_output");
        mValues.parserName = o.Arg('p', "html|rtf|pdf|mp3|txt|swf -- dynamic "
            "parser name", "html");
        mValues.parserConfig = o.Arg('c', "<config> -- parser configuration "
            "file", "");
        mValues.encoding = o.Arg('e', "<encoding> -- document encoding",
                                 "windows-1251");
        mValues.output = o.Arg('o', "<output> -- file to redirect output to; "
            "not used in packet mode", "");

        o.AutoUsageErr("[<input>]");

        if (!o.Pos.empty())
            mValues.input = o.Pos[0];
    }

    const TValues* operator ->() const {
        return &mValues;
    }
};
