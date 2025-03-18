#pragma once

#include <library/cpp/getopt/last_getopt.h>

namespace NLastGetopt {
inline void PrintUsageAndExit_(TOpts& opt, TOptsParseResult& o) {
    opt.PrintUsage(o.GetProgramName());
    exit(1);
}
inline TString Get_(TOpts& opt, TOptsParseResult& o, TOpt* op) {
    const TOptParseResult* r = o.FindOptParseResult(op);
    if (!r || r->Count() != 1) {
        PrintUsageAndExit_(opt, o);
    }
    return r->Back();
}
inline TString GetOrElse_(TOpts& opt, TOptsParseResult& o, TOpt* op, const char* dflt) {
    const TOptParseResult* r = o.FindOptParseResult(op);
    if (r && r->Count() > 1) {
        PrintUsageAndExit_(opt, o);
    }
    return r && r->Count() ? r->Back() : dflt;
}
inline bool Has_(TOpts& opt, TOptsParseResult& o, TOpt* op) {
    const TOptParseResult* r = o.FindOptParseResult(op);
    if (r && r->Count() > 1) { // still crap, it doesn't work this way
        PrintUsageAndExit_(opt, o);
    }
    return r;
}
inline int CountMulti_(TOpts& opt, TOptsParseResult& o, TOpt* op) {
    Y_UNUSED(opt);
    const TOptParseResult* r = o.FindOptParseResult(op);
    if (!r) {
        return 0;
    }
    return r->Count();
}
inline TString GetMulti_(TOpts& opt, TOptsParseResult& o, TOpt* op, size_t i) {
    const TOptParseResult* r = o.FindOptParseResult(op);
    if (!r || i >= r->Count()) {
        PrintUsageAndExit_(opt, o);
    }
    return r->Values()[i];
}
}
