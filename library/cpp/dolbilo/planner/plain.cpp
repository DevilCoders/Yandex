#include "plain.h"

#include <library/cpp/getopt/opt.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/string/subst.h>
#include <util/string/split.h>
#include <util/string/cast.h>
#include <util/string/util.h>
#include <util/string/vector.h>

static inline TString CorrectHeaders(TString headers) {
    static const TString HEADERS_DELIMITER = "\\n";
    static const TString HEADERS_DIV = "\n";
    SubstGlobal(headers, HEADERS_DELIMITER, HEADERS_DIV);
    return headers;
}

TPlainLoader::TPlainLoader()
    : Qps_(100)
    , Port_(80)
    , DistrType_("poisson")
{
}

TPlainLoader::~TPlainLoader() {
}

TString TPlainLoader::Opts() {
    return "q:h:p:z:d:";
}

void TPlainLoader::Usage() const {
    Cerr << "   -q queries per second" << Endl
         << "   -h host. Attention: if this option is used, this value (possibly, with \":port\") will be *placed before* the url from the input file. Modify the file accordingly" << Endl
         << "   -p port" << Endl
         << "   -z headers (delimeter: \\n)" << Endl
         << "   -d distribution type (poisson(default), or uniform)" << Endl << Endl;

    Cerr << "Plain format specification:" << Endl
         << "   The input file must consist of a number of tab-separated lines. Each line represents one network request" << Endl
         << "   and should be in one of the following formats:" << Endl << Endl
         << "   <url>" << Endl
         << "   <time-delta>\\t<url>" << Endl
         << "   <time-delta>\\t<url>\\t<headers>" << Endl
         << "   <time-delta>\\t<url>\\t<headers>\\t<http-method>" << Endl
         << "   <time-delta>\\t<url>\\t<headers>\\t<http-method>\\t<base64-encoded body>" << Endl << Endl
         << "   Where <headers> must consist of \"Name: Value\" pairs separated by \"\\\\n\" (two symbols!)" << Endl << Endl;
}

bool TPlainLoader::HandleOpt(const TOption* option) {
    switch (option->key) {
        case 'q': {
            if (!option->opt) {
                ythrow yexception() << "-q expect argument (queries per second)";
            }

            Qps_ = FromString<size_t>(option->opt);

            if (Qps_ == 0) {
                ythrow yexception() << "too small q value";
            }

            return true;
        }

        case 'h': {
            if (!option->opt) {
                ythrow yexception() << "-h expect argument (host)";
            }

            Host_ = option->opt;

            return true;
        }

        case 'p': {
            if (!option->opt) {
                ythrow yexception() << "-p expect argument (port)";
            }

            Port_ = FromString<ui16>(option->opt);

            return true;
        }

        case 'z': {
            if (!option->opt) {
                ythrow yexception() << "-z expect argument (headers)";
            }

            Headers_ = CorrectHeaders(option->opt);

            return true;
        }

        case 'd': {
            if (!option->opt) {
                ythrow yexception() << "-d expects argument(distribution type)";
            }

            DistrType_ = option->opt;

            return true;
        }

        default:
            break;
    }

    return false;
}

void TPlainLoader::Process(TParams* params) {
    TVector<TString> parsed;
    TString line;
    TString prefix;

    if (Host_) {
        prefix += "http://";
        prefix += Host_;

        if (Port_ != 80) {
            prefix += ':';
            prefix += ToString(Port_);
        }
    }

    IDistributionRef distr = TDistribFactory::Instance(Qps_).Find(DistrType_);

    while (params->Input()->ReadLine(line)) {
        /**
         * Supported (autodetected) plan formats:
         * <url>
         * <time-delta> \t <url>
         * <time-delta> \t <url> \t <headers>
         * <time-delta> \t <url> \t <headers> \t <method>
         * <time-delta> \t <url> \t <headers> \t <method> \t <base64-encoded body>
         **/
        try {
            TDevastateRequest request;
            Split(line.begin(), '\t', &parsed);
            const size_t chunks = parsed.size();
            TDuration delta = TDuration::MicroSeconds(chunks > 1 ? FromString<ui64>(parsed[0]) : distr->Delta());
            request.Url = prefix + ((chunks > 1) ? parsed[1] : parsed[0]);
            TStringStream headersStream(chunks > 2 ? CorrectHeaders(parsed[2]) : Headers_);
            request.Headers = THttpHeaders(&headersStream);
            if (chunks > 3) {
                request.Method = parsed[3];
            }
            if (chunks > 4) {
                request.Body = Base64Decode(parsed[4]);
            }

            params->Add(TDevastateItem(delta, request, 0));
        } catch (...) {
            Cerr << CurrentExceptionMessage() << Endl;
        }
    }
}
