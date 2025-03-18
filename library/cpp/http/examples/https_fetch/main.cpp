#include <library/cpp/openssl/io/stream.h>
#include <library/cpp/http/io/stream.h>
#include <library/cpp/uri/http_url.h>

#include <util/network/socket.h>
#include <util/string/cast.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        return -1;
    }

    THttpURL url;

    THttpURL::TParsedState parsedState = url.Parse(argv[1], THttpURL::FeaturesDefault | NUri::TFeature::FeatureSchemeKnown, 0, 65536);

    if (THttpURL::ParsedOK != parsedState || !url.Get(THttpURL::FieldHost)) {
        ythrow yexception() << "can not parse url (" << TString(argv[1]).Quote() << ", " << HttpURLParsedStateToString(parsedState) << ")";
    }

    TString host = url.Get(THttpURL::FieldHost);
    const char* p = url.Get(THttpURL::FieldPort);
    bool secure = false;
    ui16 port = 80;

    if (url.Get(THttpURL::FieldScheme) == TStringBuf("https")) {
        port = 443;
        secure = true;
    }

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

    for (size_t i = 0; i < 1; ++i) {
        TSocket s(addr);
        TSocketInput si(s);
        TSocketOutput so(s);

        if (secure) {
            TOpenSslClientIO ssl(&si, &so, {TOpenSslClientIO::TOptions::TVerifyCert{host}, Nothing()});
            THttpOutput ho(&ssl);

            (ho << req).Finish();

            THttpInput hi(&ssl);

            TransferData(&hi, &Cout);
        } else {
            THttpOutput ho(&so);

            (ho << req).Finish();

            THttpInput hi(&si);

            TransferData(&hi, &Cout);
        }
    }
}
