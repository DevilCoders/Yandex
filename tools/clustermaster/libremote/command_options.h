#pragma once

#include <library/cpp/getopt/opt.h>

struct TCommandOptions {
    int Timeout;
    int Retry;
    bool Verbose;

    TCommandOptions()
        : Timeout(-1)
        , Retry(5)
        , Verbose(false)
    {
    }

    bool Get(int &argc, char **&argv) {
        Opt opt(argc, argv, "+vqt:r:c");
        int optlet, opterr = false;
        while ((optlet = opt.Get()) != EOF) {
            switch (optlet) {
            case 'v': Verbose = true; break;
            case 'q': Verbose = false; break;
            case 't': Timeout = atoi(opt.Arg); break;
            case 'r': Retry = atoi(opt.Arg); break;
            default: opterr = true; break;
            }
        }

        argc -= opt.Ind;
        argv += opt.Ind;

        return !opterr;
    }
};
