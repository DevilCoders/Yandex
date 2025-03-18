#include "fullreqs.h"

#include "distribution.h"
#include "unsorted.h"

#include <library/cpp/getopt/opt.h>

#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/util.h>
#include <util/string/vector.h>

TFullReqsLoader::TFullReqsLoader()
    : Host("localhost")
    , Port(80)
    , DistrType("poisson")
    , Qps(100)
{
}

TFullReqsLoader::~TFullReqsLoader() {
}

TString TFullReqsLoader::Opts() {
    return "h:p:d:q:";
}

bool TFullReqsLoader::HandleOpt(const TOption* option) {
    switch (option->key) {
        case 'h': {
            if (!option->opt) {
                ythrow yexception() << "-h expects an argument (host)";
            }

            Host = option->opt;

            return true;
        }

        case 'p': {
            if (!option->opt) {
                ythrow yexception() << "-p expect argument(port)";
            }

            Port = FromString<ui16>(option->opt);

            return true;
        }

        case 'd': {
            if (!option->opt) {
                ythrow yexception() << "-d expects argument(distribution type)";
            }

            DistrType = option->opt;

            return true;
        }

        case 'q': {
            if (!option->opt) {
                ythrow yexception() << "-q expect argument(queries per second)";
            }

            Qps = FromString<size_t>(option->opt);

            if (Qps == 0) {
                ythrow yexception() << "too small q value";
            }

            return true;
        }

        default:
            break;
    }

    return false;
}

void TFullReqsLoader::Usage() const {
    Cerr << "   -h host to connect to" << Endl
         << "   -p port" << Endl
         << "   -q queries per second" << Endl
         << "   -d distribution type(poisson(default), or uniform)" << Endl;
}

static inline bool ReadRequest(IInputStream* input, TString& request) {
    request.clear();
    TStringOutput out(request);

    size_t numConsequentEmptyLines = 0;

    for (;;) {
        TString line;
        if (!input->ReadLine(line))
            return false;

        if (line.empty()) {
            ++numConsequentEmptyLines;

            if (numConsequentEmptyLines == 2)
                return true;
        } else {
            numConsequentEmptyLines = 0;
        }

        out << line << TStringBuf("\n");
    }

    return false;
}

void TFullReqsLoader::Process(TParams* params) {
    TString request;

    IDistributionRef distr = TDistribFactory::Instance(Qps).Find(DistrType);

    while (ReadRequest(params->Input(), request)) {
        try {
            params->Add(TDevastateItem(TDuration::MicroSeconds(distr->Delta()), Host, Port, request, 0));
        } catch (...) {
            Cerr << CurrentExceptionMessage() << Endl;
        }
    }
}
