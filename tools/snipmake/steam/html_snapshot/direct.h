#pragma once

#include "fetch.h"

#include <yweb/webutil/url_fetcher_lib/fetcher.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <library/cpp/threading/future/legacy_future.h>

namespace NSteam
{

class TDirectFetcher: public TBaseFetcher, public IUrlIterator, public IFetchCallback
{
private:
    THolder<NThreading::TLegacyFuture<> > Runner;
    TDuration Timeout;
    TString UserAgent;
    TCheckUrl CheckUrl;
    bool Eof;

    static void Start(TDirectFetcher* host);

public:
    TDirectFetcher(TDuration timeout, const TString& userAgent = TString());

    const char* GetName() const override
    {
        return "Direct";
    }

    TDuration GetTimeout() const override
    {
        return Timeout;
    }

    bool IsEof() const override;
    const TCheckUrl* Next() override;
    bool HasNext() override;

    void OnFetchResult(const TFetchResult& result, const TCheckUrl& url, IInputStream* content) override;

    void Terminate();
};

}
