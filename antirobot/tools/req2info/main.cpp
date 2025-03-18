#include "req2info.h"

#include <antirobot/daemon_lib/antirobot_service.h>
#include <antirobot/daemon_lib/request_params.h>

#include <library/cpp/charset/ci_string.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/http/io/headers.h>
#include <library/cpp/http/io/stream.h>
#include <library/cpp/uri/uri.h>

#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/str.h>
#include <util/string/vector.h>

namespace {
    const TCiString XFFY = "X-Forwarded-For-Y";
    const TString DEFAULT_ADDR = "127.0.0.1";
}

bool HasHeader(const TVector<TString>& list, const TCiString& header) {
    TCiString headerWithColon = header + ':';
    for(const auto& headerLine : list) {
        if (TCiString::is_prefix(headerWithColon, headerLine)) {
            return true;
        }
    }
    return false;
}

TString MakeRawRequestFromURL(const TString& url, const TVector<TString>& headers) {
    TVector<TString> req;

    using namespace NUri;

    TUri uri;
    TState::EParsed parseResult = uri.Parse(url, TFeature::FeaturesDefault | TFeature::FeatureEncodeExtendedASCII | TFeature::FeatureSchemeKnown);
    if (parseResult != TState::ParsedOK) {
        throw yexception() << "Could not parse the given URL: " << ParsedStateToString(parseResult) << Endl;;
    }

    req.push_back("GET " + uri.PrintS(NUri::TField::FlagPath | NUri::TField::FlagQuery | NUri::TField::FlagFrag) + " HTTP/1.1");
    req.push_back("Host: " + uri.PrintS(NUri::TField::FlagHost | NUri::TField::FlagPort));
    req.insert(req.end(), headers.begin(), headers.end());

    if (!HasHeader(headers, XFFY)) {
        TString xffy = XFFY + ": " + DEFAULT_ADDR;
        req.push_back(xffy);
    }

    req.push_back("");

    return JoinStrings(req, "\r\n");
}

TString ReadRawReqFromStdin() {
    TString result = Cin.ReadAll();

    TStringInput strIn(result);
    THttpInput inp(&strIn);

    const auto& headers = inp.Headers();
    auto it = headers.Begin();
    for (; it < headers.End(); ++it) {
       if (!TCiString::compare(it->Name(), XFFY)) {
           break;
       }
    }

    if (it == headers.End()) {
        throw yexception() << "The raw request doesn't contain X-Forwarded-For-Y header, so it can't be examined";
    }

    return result;
}

int main(int argc, char* argv[]) {
    try {
        using namespace NLastGetopt;

        TString studiedRequest;
        bool rawReq = false;
        TVector<TString> extraHeaders;
        TString antirobotHost;

        TOpts opts;
        opts.AddLongOption("header", "specify request header; more than one allowed").AppendTo(&extraHeaders);
        opts.AddLongOption("rawreq", "read the raw request from stdin").SetFlag(&rawReq).NoArgument();
        opts.AddLongOption("host", "host:port of an antirobot instance to ask").StoreResult(&antirobotHost).OptionalArgument("HOST_PORT");
        opts.AddHelpOption();

        opts.SetFreeArgsMin(0);
        opts.SetFreeArgsMax(1);
        opts.SetFreeArgTitle(0, "<url>", "Some URL");

        TOptsParseResult res(&opts, argc, argv);
        TVector<TString> freeArgs = res.GetFreeArgs();

        if (rawReq) {
            studiedRequest =  ReadRawReqFromStdin();
        } else {
            if (freeArgs.empty()) {
                throw yexception() << "Please specify raw request on stdin or URL as argument";
            }
            studiedRequest = MakeRawRequestFromURL(freeArgs[0], extraHeaders);
        }

        bool asFullReq = true;
        if (antirobotHost.empty()) {
            antirobotHost = NAntiRobot::ANTIROBOT_SERVICE_HOST;
            asFullReq = false;
        }

        Cout << GetReqInfo(antirobotHost, asFullReq, studiedRequest);
        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 2;
    }
}
