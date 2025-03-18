#include "prog_options.h"

const char* TProgramOptions::OPT_SPEC_VALUES = "+*?#";

void TProgramOptions::Init(int argc, const char* argv[]) {
    int argNo = 1;

    while (argNo < argc) {
        if (argv[argNo][0] != '-') {
            UnnamedOptions.push_back(argv[argNo]);
            ++argNo;
            continue;
        }

        // parse options
        TString optName = argv[argNo] + 1;
        char optSpec = GetOptionSpec(optName);

        if ((optSpec != '#') && (Options.find(optName) != Options.end())) {
            throw TProgOptionsException(("Option " + optName + " has been specified multiple times").data());
        }

        if ((argNo == argc - 1) || (argv[argNo + 1][0] == '-' && argv[argNo + 1][1] != '\x0') || (optSpec == ' ')) {
            if ((optSpec == '+') || (optSpec == '#')) {
                throw TProgOptionsException(("Required argument for option " + optName + " not provided").data());
            }
            Options[optName] = nullptr;
            ++argNo;
        } else {
            if (optSpec == '*') { //'*' : read till the end
                LongOption = "";
                for (++argNo; argNo < argc; ++argNo) {
                    LongOption += argv[argNo];
                    LongOption += " ";
                }
                if (LongOption.size() != 0) {
                    LongOption.resize(LongOption.size() - 1);
                    Options[optName] = LongOption.c_str();
                } else {
                    Options[optName] = nullptr;
                }
            } else {
                if (optSpec == '#') {
                    MultOptions[optName].push_back(argv[++argNo]);
                } else {
                    Options[optName] = argv[++argNo];
                }
                ++argNo;
            }
        }
    }
}
