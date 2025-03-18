#pragma once

#include <tools/clustermaster/libremote/fetch/httpagent.h>
#include <tools/clustermaster/libremote/fetch/sockhandler.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/http/fetch/httpfetcher.h>
#include <library/cpp/http/fetch/httpzreader.h>
#include <library/cpp/string_utils/quote/quote.h>

#include <util/system/file.h>

namespace fetch {

void SslStaticInit();

namespace NUrlFetcherPrivate {
struct TBufferWriter {
    TBuffer Buffer;

    int Write(void* data, size_t size) {
        Buffer.Append(reinterpret_cast<const char*>(data), size);
        return size;
    }
};
}

struct TUrlFetcher
    : private THttpFetcher< TFakeAlloc<>, TFakeCheck<>, NUrlFetcherPrivate::TBufferWriter, fetch::THttpsAgent<> >
{
    typedef THttpFetcher< TFakeAlloc<>, TFakeCheck<>, NUrlFetcherPrivate::TBufferWriter, fetch::THttpsAgent<> > TBase;

    TString PathQuery;

    TUrlFetcher(const TStringBuf& stringUrl);

    int Fetch(IOutputStream& out, const TVector<TString>& headers);

    static TString BasicAuthHeaderLine(const TString& user, const TString& password);
    static TString OAuthHeaderLine(const TString& token);
};

} // namespace fetch
