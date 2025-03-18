#include <library/cpp/http/io/stream.h>
#include <library/cpp/uri/http_url.h>
#include <util/network/socket.h>
#include <util/string/cast.h>

int main(int /*argc*/, char** argv) {
    THttpURL url;

    THttpURL::TParsedState parsedState = url.Parse(argv[1], THttpURL::FeaturesDefault, 0, 65536);
    if (THttpURL::ParsedOK != parsedState || !url.Get(THttpURL::FieldHost)) {
        ythrow yexception() << "can not parse url ("
                            << TString(argv[1]).Quote()
                            << ", "
                            << HttpURLParsedStateToString(parsedState)
                            << ")";
    }

    TString host = url.Get(THttpURL::FieldHost);
    const char* p = url.Get(THttpURL::FieldPort);
    ui16 port = 80;

    if (p) {
        port = FromString<ui16>(p);
    }

    TString req;

    req = "GET ";
    req += url.PrintS(THttpURL::FlagPath | THttpURL::FlagQuery);
    req += " HTTP/1.1\r\n";
    req += "Host: ";
    req += host;
    req += "\r\n";
    req += "\r\n";

    TNetworkAddress addr(host, port);
    TSocket s(addr);
    TSocketInput si(s);
    TSocketOutput so(s);
    THttpOutput ho(&so);

    (ho << req).Finish();

    THttpInput hi(&si);

    TransferData(&hi, &Cout);
}
