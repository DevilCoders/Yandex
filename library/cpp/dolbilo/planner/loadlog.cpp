#include "loadlog.h"

#include "unsorted.h"

#include <library/cpp/getopt/opt.h>

#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/util.h>
#include <util/string/vector.h>
#include <util/string/split.h>

TLoadLogLoader::TLoadLogLoader()
    : Prefix_("http://localhost:8041/yandsearch?")
{
}

TLoadLogLoader::~TLoadLogLoader() {
}

TString TLoadLogLoader::Opts() {
    return "p:";
}

bool TLoadLogLoader::HandleOpt(const TOption* option) {
    if (option->key == 'p') {
        if (!option->opt) {
            ythrow yexception() << "-p requre argument(prefix for each request)";
        }

        Prefix_ = option->opt;

        return true;
    }

    return false;
}

void TLoadLogLoader::Usage() const {
    Cerr << "   -p prefix for each request(" << Prefix_ << ")" << Endl;
}

void TLoadLogLoader::Process(TParams* params) {
    TMonotonicAdapter saver(params);
    TString line;

    while (params->Input()->ReadLine(line)) {
        try {
            TVector<TString> parsed;
            StringSplitter(line).Split('\t').SkipEmpty().Collect(&parsed);

            if (parsed.size() < 12) {
                continue;
            }

            const ui64 start = FromString<ui64>(parsed[1]) * 1000000 + FromString<ui64>(parsed[2]);

            saver.Add(TInstant::MicroSeconds(start), TDevastateItem(Prefix_ + parsed[11], TDuration::Zero(), TString(), 0));
        } catch (...) {
            Cerr << CurrentExceptionMessage() << Endl;
        }
    }

    saver.Flush();
}
