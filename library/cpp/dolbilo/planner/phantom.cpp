#include "phantom.h"

#include "distribution.h"
#include "unsorted.h"

#include <library/cpp/getopt/opt.h>

#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/util.h>
#include <util/string/vector.h>
#include <util/charset/unidata.h>

TPhantomLoader::TPhantomLoader()
    : Host("localhost")
    , Port(80)
    , DistrType("poisson")
    , Qps(100)
{
}

TPhantomLoader::~TPhantomLoader() {
}

TString TPhantomLoader::Opts() {
    return "h:p:d:q:";
}

bool TPhantomLoader::HandleOpt(const TOption* option) {
    Y_ASSERT(option->opt);

    switch (option->key) {
        case 'h':
            Host = option->opt;
            return true;
        case 'p':
            Port = FromString<ui16>(option->opt);
            return true;
        case 'd':
            DistrType = option->opt;
            return true;
        case 'q':
            Qps = FromString<size_t>(option->opt);
            if (Qps == 0) {
                ythrow yexception() << "too small q value";
            }
            return true;
    }

    return false;
}

void TPhantomLoader::Usage() const {
    Cerr << "   -h host to connect to" << Endl
         << "   -p port" << Endl
         << "   -q queries per second" << Endl
         << "   -d output distribution type(poisson(default), uniform, stpd)" << Endl
         << Endl
         << "`phantom' mode loads data from `ammo' or `stpd' file formatted like [<len> <timestamp>? <tag>?\\n<blob>]*" << Endl
         << "`timestamp' is microseconds since the beginning of the plan" << Endl
         << "`tag' is not supported" << Endl
    ;
}

namespace {

struct TMissle {
    TString Request;
    TMaybeFail<TDuration> Timestamp; // only for `stpd` files
};

using TMaybeMissle = TMaybe<TMissle>;

inline TMaybeMissle ReadRequest(IInputStream* input) {
    TString line;
    while (line.empty()) { // some tools add extra newlines, some don't
        if (!input->ReadLine(line))
            return {};
    }

    TMissle r;
    TStringBuf linebuf(line);

    const ui64 len = FromString<ui64>(linebuf.NextTok(' '));
    r.Request.resize(len);
    input->LoadOrFail(r.Request.begin(), len);

    if (!linebuf.empty() && IsDigit(linebuf[0])) {
        const ui64 ts = FromString<ui64>(linebuf.NextTok(' '));
        r.Timestamp = TDuration::MilliSeconds(ts);
    }

    if (!linebuf.empty()) {
        static bool log;
        SimpleCallOnce(log, [&]() {
            Cerr << "`phantom' planner does not support tags like " << TString(linebuf).Quote() << Endl;
        });
    }

    return r;
}

} // namespace

void TPhantomLoader::Process(TParams* params) {
    if (DistrType.equal("stpd")) {
        TMaybeMissle m = ReadRequest(params->Input());
        if (!m.Defined())
            return; // empty file
        TDuration delay;
        TMaybeMissle next;
        while ((next = ReadRequest(params->Input())).Defined()) {
            delay = *next->Timestamp - *m->Timestamp;
            params->Add(TDevastateItem(delay, Host, Port, m->Request, 0));
            m = next;
        }
        // it's better to reuse last `delay` than set it to Zero due to `--circular` case
        params->Add(TDevastateItem(delay, Host, Port, m->Request, 0));
    } else {
        const IDistributionRef distr = TDistribFactory::Instance(Qps).Find(DistrType);
        TMaybeMissle m;
        while ((m = ReadRequest(params->Input())).Defined()) {
            if (m->Timestamp.Defined()) {
                static bool log;
                SimpleCallOnce(log, [&]() {
                    Cerr << "`phantom' timestamp is ignored, -d " << DistrType << " is used" << Endl;
                });
            }
            try {
                params->Add(TDevastateItem(TDuration::MicroSeconds(distr->Delta()), Host, Port, m->Request, 0));
            } catch (...) {
                Cerr << CurrentExceptionMessage() << Endl;
            }
        }
    }
}
